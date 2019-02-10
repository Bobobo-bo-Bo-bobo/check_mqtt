#include "check_mqtt.h"
#include "mqtt_functions.h"
#include "util.h"

#include <setjmp.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result) {
    struct configuration *cfg = (struct configuration *) userdata;

    cfg->mqtt_connect_result = result;
    if (result) {
        longjmp(state, ERROR_MQTT_CONNECT_FAILED);
    }

#ifdef DEBUG
    printf("DEBUG: mqtt_connect_callback: result=%d\n", result);
    printf("DEBUG: mqtt_connect_callback: subscribing to topic %s\n", cfg->topic);
#endif

    cfg->mqtt_error = mosquitto_subscribe(mosq, NULL, cfg->topic, cfg->qos);

#ifdef DEBUG
    printf("DEBUG: mqtt_connect_callback: subscribe returned %d (%s)\n", cfg->mqtt_error, mosquitto_strerror(cfg->mqtt_error));
#endif

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        longjmp(state, ERROR_MQTT_SUBSCRIBE_FAILED);
    }
}

void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result) {
    struct configuration *cfg = (struct configuration *) userdata;

#ifdef DEBUG
    printf("DEBUG: mqtt_disconnect_callback: result=%d (%s)\n", result, mosquitto_strerror(result));
#endif

    cfg->mqtt_error = result;
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos) {
    struct configuration *cfg = (struct configuration *) userdata;

#ifdef DEBUG
    printf("DEBUG: mqtt_subscribe_callback: subscribed to topic\n");
    printf("DEBUG: mqtt_subscribe_callback: Publishing payload %s\n", cfg->payload);
#endif

    cfg->mqtt_error = mosquitto_publish(mosq, NULL, cfg->topic, (int) strlen(cfg->payload) + 1, (void *) cfg->payload, cfg->qos, false);

#ifdef DEBUG
    printf("DEBUG: mqtt_subscribe_callback: mosquitto_publish returned %d (%s)\m", cfg->mqtt_error, mosquitto_strerror(mqtt->cfg_error));
#endif

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        longjmp(state, ERROR_MQTT_PUBLISH_FAILED);
    }

    clock_gettime(CLOCK_MONOTONIC, &cfg->send_time);
}

void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    struct configuration *cfg = (struct configuration *) userdata;
    char *response;

    // Note: struct mosquitto_message * will be released by libmosquitto as soon as this
    //       callback finnishes
    response = (char *) malloc(msg->payloadlen + 1);
    if (!response) {
        longjmp(state, ERROR_OOM);
    }
    memset((void *) response, 0, msg->payloadlen + 1);

    memcpy((void *) response, msg->payload, msg->payloadlen);

#ifdef DEBUG
        printf("DEBUG: mqtt_message_callback: received message: %s\n", response);
#endif

    if (!strcmp(cfg->payload, response)) {
        // this is our probe payload, measure receive time and exit MQTT loop
        clock_gettime(CLOCK_MONOTONIC, &cfg->receive_time);

#ifdef DEBUG
        printf("DEBUG: mqtt_message_callback: received response matches our probe\n");
#endif

        free(response);
        cfg->payload_received = true;

#ifdef DEBUG
        printf("DEBUG: mqtt_message_callback: disconnecting\m");
#endif
        mosquitto_disconnect(mosq);
        return;
    }

#ifdef DEBUG
    printf("DEBUG: mqtt_message_callback: received response is not the payload we sent earlier (%s != %s)\n", response, cfg->payload);
#endif

    free(response);

    // keep on listening until the timeout has been reached or we received our payload
}

int mqtt_connect(struct configuration *cfg) {
    char *mqttid;
    char *mqtt_uuid;
    size_t mqttid_len = strlen(MQTT_UID_PREFIX) + 36 + 1;

    mqttid = (char *) malloc(mqttid_len);
    if (!mqttid) {
        fprintf(stderr, "Unable to allocate %ld bytes of memory for MQTT ID\n", mqttid_len);
        return -1;
    }
    memset((void *) mqttid, 0, mqttid_len);

    mqtt_uuid = uuidgen();
    if (!mqtt_uuid) {
        fprintf(stderr, "Unable to allocate 37 bytes of memory for UUID\n");
        free(mqttid);
        return -1;
    }

    snprintf(mqttid, mqttid_len, "%s%s", MQTT_UID_PREFIX, mqtt_uuid);
    free(mqtt_uuid);

    mosquitto_lib_init(); // always return MOSQ_ERR_SUCCESS

    // initialize MQTT structure, clean messages and subscriptions on disconnect
    cfg->mqtt_handle = mosquitto_new(mqttid, true, (void *) cfg);

#ifdef DEBUG
    printf("DEBUG: mqtt_connect: mosquitto_new returned new MQTT connection structure at 0x%0x\n", cfg->mqtt_handle);
#endif

    if (!cfg->mqtt_handle) {
        fprintf(stderr, "Unable to initialise MQTT structure\n");
        free(mqttid);
        return -1;
    }

    // we are not threaded
    mosquitto_threaded_set(cfg->mqtt_handle, false);

    // configure basic SSL
    if (cfg->ssl) {
        if (cfg->insecure) {
            cfg->mqtt_error = mosquitto_tls_opts_set(cfg->mqtt_handle, SSL_VERIFY_NONE, NULL, NULL);
        } else {
            cfg->mqtt_error = mosquitto_tls_opts_set(cfg->mqtt_handle, SSL_VERIFY_PEER, NULL, NULL);
        }
        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }

        cfg->mqtt_error = mosquitto_tls_set(cfg->mqtt_handle, cfg->ca, cfg->cadir, cfg->cert, cfg->key, NULL);
        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    }

    // XXX: There is a third option, "pre-shared key over TLS" - mosquitto_tls_psk_set
    if (cfg->user) {

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: setting up username/password authentication\n");
#endif

        cfg->mqtt_error = mosquitto_username_pw_set(cfg->mqtt_handle, cfg->user, cfg->password);

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: mosquitto_username_pw_set returned %d (%s)\n", cfg->mqtt_error, mosquitto_strerror(cfg->mqtt_error));
#endif

        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    } else if (cfg->cert) {

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: setting up SSL certificate authentication\n");
#endif

        cfg->mqtt_error = mosquitto_tls_set(cfg->mqtt_handle, cfg->ca, cfg->cadir, cfg->cert, cfg->key, NULL);

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: mosquitto_tls_set returned %d (%s)\n", cfg->mqtt_error, mosquitto_strerror(cfg->mqtt_error));
#endif

        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    }

#ifdef DEBUGG
    printf("DEBUG: mqtt_connect: installing callback functions\n");
#endif

    //set callback handlers
    mosquitto_connect_callback_set(cfg->mqtt_handle, mqtt_connect_callback);
    mosquitto_disconnect_callback_set(cfg->mqtt_handle, mqtt_disconnect_callback);
    mosquitto_subscribe_callback_set(cfg->mqtt_handle, mqtt_subscribe_callback);
    mosquitto_message_callback_set(cfg->mqtt_handle, mqtt_message_callback);

    cfg->mqtt_error = mosquitto_connect(cfg->mqtt_handle, cfg->host, cfg->port, cfg->keep_alive);

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: mosquitto_connect returned %d (%s)\n", cfg->mqtt_error, mosquitto_strerror(cfg->mqtt_error));
#endif

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        free(mqttid);
        mosquitto_lib_cleanup();
        return -1;
    }

    cfg->mqtt_error = mosquitto_loop_forever(cfg->mqtt_handle, cfg->timeout, 1);

#ifdef DEBUG
        printf("DEBUG: mqtt_connect: mosquitto_loop_forever returned %d (%s)\n", cfg->mqtt_error, mosquitto_strerror(cfg->mqtt_error));
#endif

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        free(mqttid);
        mosquitto_lib_cleanup();
        return -1;
    }

    free(mqttid);
    mosquitto_lib_cleanup();

    return 0;
}

