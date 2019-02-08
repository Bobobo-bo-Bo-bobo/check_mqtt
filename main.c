#include "check_mqtt.h"
#include "usage.h"
#include "util.h"
#include "mqtt_functions.h"

#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

const char *const short_opts = "hH:p:c:k:C:iQ:T:t:su:P:w:W:K:";
const struct option long_opts[] = {
    { "help", no_argument, NULL, 'h' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    { "cert", required_argument, NULL, 'c' },
    { "key", required_argument, NULL, 'k' },
    { "ca", required_argument, NULL, 'C' },
    { "cadir", required_argument, NULL, 'D' },
    { "insecure", no_argument, NULL, 'i' },
    { "qos", required_argument, NULL, 'Q' },
    { "topic", required_argument, NULL, 'T' },
    { "timeout", required_argument, NULL, 't' },
    { "ssl", no_argument, NULL, 's' },
    { "user", required_argument, NULL, 'u' },
    { "password", required_argument, NULL, 'P' },
    { "warn", required_argument, NULL, 'w' },
    { "critical", required_argument, NULL, 'W' },
    { "keepalive", required_argument, NULL, 'K' },
    { NULL, 0, NULL, 0 },
};

int main(int argc, char **argv) {
    struct configuration *config;
    int exit_code;
    int opt_idx;
    int opt_rc;
    long temp_long;
    int rc;
    struct timespec delay;
    double rtt;

    exit_code = NAGIOS_UNKNOWN;
    config = (struct configuration *) malloc(sizeof(struct configuration));
    if (!config) {
        fprintf(stderr, "Failed to allocate %d bytes of memory for configuration\n", sizeof(struct configuration));
        exit(NAGIOS_UNKNOWN);
    }
    memset((void *)config, 0, sizeof(struct configuration));

    // set defaults
    config->port = DEFAULT_PORT;
    config->insecure = false;
    config->qos = DEFAULT_QOS;
    config->timeout = DEFAULT_TIMEOUT;
    config->ssl = false;
    config->warn = DEFAULT_WARN_MS;
    config->critical = DEFAULT_CRITICAL_MS;
    config->keep_alive = DEFAULT_KEEP_ALIVE;

    for (;;) {
        opt_rc = getopt_long(argc, argv, short_opts, long_opts, &opt_idx);
        if (opt_rc == -1) {
            break;
        }
        switch(opt_rc) {
            case 'h': {
                          usage();
                          exit(NAGIOS_OK);
                      }
            case 'H': {
                          config->host = strdup(optarg);
                          if (!config->host) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for hostname\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'p': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }

                          if ((temp_long <= 0) || (temp_long > 65535)) {
                              fprintf(stderr, "Invalid port %d\n", temp_long);
                              goto leave;
                          }
                          config->port = (unsigned int) temp_long;
                          break;
                      }
            case 'c': {
                          config->cert = strdup(optarg);
                          if (!config->cert) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL certificate file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'k': {
                          config->key = strdup(optarg);
                          if (!config->key) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL private key file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'C': {
                          config->ca = strdup(optarg);
                          if (!config->ca) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL CA certificate file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'D': {
                          config->cadir = strdup(optarg);
                          if (!config->cadir) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL CA certificate directory\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'i': {
                          config->insecure = true;
                          break;
                      }
            case 'Q': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }

                          // only 0, 1 and 2 are valid QoS values
                          if ((temp_long < 0) || (temp_long > 2)) {
                              fprintf(stderr, "Invalid QoS value %d (valid values are 0, 1 or 2)\n", temp_long);
                              goto leave;
                          }
                          config->qos = (int) temp_long;
                      }
            case 'T': {
                          config->topic = strdup(optarg);
                          if (!config->topic) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for MQTT topic\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 't': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }
                          if (temp_long <= 0) {
                              fprintf(stderr, "Invalid timeout %d (must be > 0)\n", temp_long);
                              goto leave;
                          }
                          config->timeout = (unsigned int) temp_long;
                          break;
                      }
            case 's': {
                          config->ssl = true;
                          break;
                      }
            case 'u': {
                          config->user = strdup(optarg);
                          if (!config->user) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for user name\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'P': {
                          config->password = strdup(optarg);
                          if (!config->password) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for password\n", strlen(optarg) + 1);
                              goto leave;
                          };
                          break;
                      }
            case 'w': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }
                          if (temp_long <= 0) {
                              fprintf(stderr, "Invalid timeout value %d (must be > 0)\n", temp_long);
                              goto leave;
                          }
                          config->warn = (unsigned int) temp_long;
                          break;
                      }
            case 'W': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }
                          if (temp_long <= 0) {
                              fprintf(stderr, "Invalid timeout value %d (must be > 0)\n", temp_long);
                              goto leave;
                          }
                          config->critical = (unsigned int) temp_long;
                          break;
                      }
            case 'K': {
                          temp_long = str2long(optarg);
                          if (temp_long == LONG_MIN) {
                              goto leave;
                          }

                          if (temp_long <= 0) {
                              fprintf(stderr, "Invalid keep alive value %s (must be > 0)\n", temp_long);
                              goto leave;
                          }
                          config->keep_alive = (int) temp_long;
                          break;
                      }
            default: {
                         fprintf(stderr, "Unknown argument\n");
                         goto leave;
                     }

        }
    }

    // host name is mandatory
    if (!config->host) {
        fprintf(stderr, "Host option is mandatory\n\n");
        usage();
        goto leave;
    }

    if (!config->topic) {
        config->topic = strdup(DEFAULT_TOPIC);
        if (!config->topic) {
            fprintf(stderr, "Unable to allocate %d bytes of memory for MQTT topic\n", strlen(DEFAULT_TOPIC) + 1);
            goto leave;
        }
    }

    // sanity checks
    if (config->warn > config->critical) {
        fprintf(stderr, "Critical threshold must be greater or equal than warning threshold\n");
        goto leave;
    }

    // User/password authentication and authentication with SSL client certificate are mutually exclusive
    if ((config->user || config->password) && (config->cert || config->key)) {
        fprintf(stderr, "User/password authentication and authentication using SSL certificate are mutually exclusive\n");
        goto leave;
    }

    // Note: The library allows for username and no password to send only username
    if (!config->user) {
        // SSL authentication requires certificate and key file
        if ((!config->cert) || (!config->key)) {
            fprintf(stderr, "SSL authentication requires certificate and key file\n");
            goto leave;
        }
    }
    config->payload = uuidgen();
    if (!config->payload) {
        fprintf(stderr, "Unable to allocate 37 bytes of memory for MQTT payload\n");
        goto leave;
    }

    if (mqtt_connect(config) == -1) {
        exit_code = NAGIOS_CRITICAL;

        if (config->mqtt_connect_result) {
            // XXX: Print connect errors
        } else {
            fprintf(stdout, "%s\n", mosquitto_strerror(config->mqtt_error));
        }
    } else {
        delay = get_delay(config->send_time, config->receive_time);
        rtt = timespec2double_ms(delay);
    };

leave:
    if (config) {
        free_configuration(config);
        free(config);
    }
    exit(exit_code);
}

