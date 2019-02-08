#include "check_mqtt.h"
#include "mqtt_functions.h"
#include "util.h"

#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result) {
    struct configuration *cfg = (struct configuration *) userdata;

    cfg->mqtt_connect_result = result;
    if (result) {
        return;
    }

    cfg->mqtt_error = mosquitto_subscribe(mosq, NULL, cfg->topic, cfg->qos);

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        // XXX: How to handle errors ?
        return;
    }
}

void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result) {
    struct configuration *cfg = (struct configuration *) userdata;

    cfg->mqtt_error = result;
}

void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos) {
    struct configuration *cfg = (struct configuration *) userdata;

    cfg->mqtt_error = mosquitto_publish(mosq, NULL, cfg->topic, (int) strlen(cfg->payload) + 1, (void *) cfg->payload, cfg->qos, false);

    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        // XXX: How to handle errors ?
        return;
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
        // XXX: Is there a better way to handle OOM condition here?
        return;
    }
    memset((void *) response, 0, msg->payloadlen + 1);

    memcpy((void *) response, msg->payload, msg->payloadlen);

    if (!strcmp(cfg->payload, response)) {
        // this is our probe payload, measure receive time and exit MQTT loop
        clock_gettime(CLOCK_MONOTONIC, &cfg->receive_time);

        free(response);
        cfg->payload_received = true;

        mosquitto_disconnect(mosq);
        return;
    }

    // keep on listening until the timeout has been reached or we received our payload
}

int mqtt_connect(struct configuration *cfg) {
    struct mosquitto *mqtt = NULL;
    char *mqttid;
    char *mqtt_uuid;
    size_t mqttid_len = strlen(MQTT_UID_PREFIX) + 36 + 1;

    mqttid = (char *) malloc(mqttid_len);
    if (!mqttid) {
        fprintf(stderr, "Unable to allocate %d bytes of memory for MQTT ID\n", mqttid_len);
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
    mqtt = mosquitto_new(mqttid, true, (void *) cfg);
    if (!mqtt) {
        fprintf(stderr, "Unable to initialise MQTT structure\n");
        free(mqttid);
        return -1;
    }

    // we are not threaded
    mosquitto_threaded_set(mqtt, false);

    // configure basic SSL
    if (cfg->ssl) {
        if (cfg->insecure) {
            cfg->mqtt_error = mosquitto_tls_opts_set(mqtt, SSL_VERIFY_NONE, NULL, NULL);
        } else {
            cfg->mqtt_error = mosquitto_tls_opts_set(mqtt, SSL_VERIFY_PEER, NULL, NULL);
        }

        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    }

    // XXX: There is a third option, "pre-shared key over TLS" - mosquitto_tls_psk_set
    if (cfg->user) {
        cfg->mqtt_error = mosquitto_username_pw_set(mqtt, cfg->user, cfg->password);
        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    } else if (cfg->cert) {
        cfg->mqtt_error = mosquitto_tls_set(mqtt, cfg->ca, cfg->cadir, cfg->cert, cfg->key, NULL);
        if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
            free(mqttid);
            mosquitto_lib_cleanup();
            return -1;
        }
    }

    //set callback handlers
    mosquitto_connect_callback_set(mqtt, mqtt_connect_callback);
    mosquitto_disconnect_callback_set(mqtt, mqtt_disconnect_callback);
    mosquitto_subscribe_callback_set(mqtt, mqtt_subscribe_callback);
    mosquitto_message_callback_set(mqtt, mqtt_message_callback);

    cfg->mqtt_error = mosquitto_connect(mqtt, cfg->host, cfg->port, cfg->keep_alive);
    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        free(mqttid);
        mosquitto_lib_cleanup();
        return -1;
    }

    cfg->mqtt_error = mosquitto_loop_forever(mqtt, cfg->timeout, 1);
    if (cfg->mqtt_error != MOSQ_ERR_SUCCESS) {
        free(mqttid);
        mosquitto_lib_cleanup();
        return -1;
    }

    free(mqttid);
    mosquitto_lib_cleanup();

    return 0;
}

