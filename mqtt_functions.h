#ifndef __CHECK_MQTT_MQTT_FUNCTIONS_H__
#define __CHECK_MQTT_MQTT_FUNCTIONS_H__

#include <mosquitto.h>

#define SSL_VERIFY_NONE 0
#define SSL_VERIFY_PEER 1

void mqtt_connect_callback(struct mosquitto *, void *, int);
void mqtt_disconnect_callback(struct mosquitto *, void *, int);
void mqtt_subscribe_callback(struct mosquitto *, void *, int, int, const int*);
void mqtt_message_callback(struct mosquitto *, void *, const struct mosquitto_message *);

int mqtt_connect(struct configuration *);

#endif /* __CHECK_MQTT_MQTT_FUNCTIONS_H__ */

