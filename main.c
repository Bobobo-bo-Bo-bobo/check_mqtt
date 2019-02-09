#include "check_mqtt.h"
#include "usage.h"
#include "util.h"
#include "mqtt_functions.h"
#include "sig_handler.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

const char *const short_opts = "hH:p:c:k:C:iQ:T:t:su:P:w:W:K:f:";
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
    { "password-file", required_argument, NULL, 'f' },
    { NULL, 0, NULL, 0 },
};

static sigjmp_buf state;

int main(int argc, char **argv) {
    struct configuration *config;
    int exit_code;
    int opt_idx;
    int opt_rc;
    long temp_long;
    int rc;
    struct timespec delay;
    double rtt;
#ifdef HAVE_SIGACTION
    struct sigaction action;
    sigset_t mask;
#endif

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
                          if (config->host) {
                              free(config->host);
                          }
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
                          if (config->cert) {
                              free(config->cert);
                          }
                          config->cert = strdup(optarg);
                          if (!config->cert) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL certificate file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'k': {
                          if (config->key) {
                              free(config->key);
                          }
                          config->key = strdup(optarg);
                          if (!config->key) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL private key file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'C': {
                          if (config->ca) {
                              free(config->ca);
                          }
                          config->ca = strdup(optarg);
                          if (!config->ca) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for SSL CA certificate file\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'D': {
                          if (config->cadir) {
                              free(config->cadir);
                          }
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
                          if (config->topic) {
                              free(config->topic);
                          }
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
                          if (config->user) {
                              free(config->user);
                          }
                          config->user = strdup(optarg);
                          if (!config->user) {
                              fprintf(stderr, "Unable to allocate %d bytes of memory for user name\n", strlen(optarg) + 1);
                              goto leave;
                          }
                          break;
                      }
            case 'P': {
                          if (config->password) {
                              free(config->password);
                          }
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
            case 'f': {
                          rc = read_password_from_file(optarg, config);
                          if (rc != 0) {
                              fprintf(stderr, "Can't read password from %s, errno=%d (%s)\n", optarg, rc, strerror(rc));
                              goto leave;
                          }
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

    // set missing defaults
    if (!config->topic) {
        config->topic = strdup(DEFAULT_TOPIC);
        if (!config->topic) {
            fprintf(stderr, "Unable to allocate %d bytes of memory for MQTT topic\n", strlen(DEFAULT_TOPIC) + 1);
            goto leave;
        }
    }
    if (!config->cadir) {
        config->cadir = strdup(DEFAULT_CADIR);
        if (!config->cadir) {
            fprintf(stderr, "Unable to allocate %d bytes of memory for CA directory\n", strlen(DEFAULT_CADIR) + 1);
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

#ifdef DEBUG
    print_configuration(config);
#endif

#ifdef HAVE_SIGACTION
    sigemptyset(&mask);
    action.sa_handler = (void *) alarm_handler;
    action.sa_sigaction = NULL;
    action.sa_mask = mask;
    action.sa_flags = SA_RESETHAND;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        fprintf(stderr, "Can't install signal handler for SIGALRM, errno=%d (%s)\n", errno, strerror(errno));
        goto leave;
    }
#else
    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
        fprintf(stderr, "Can't install signal handler for SIGALRM, errno=%d (%s)\n", errno, strerror(errno));
        goto leave;
    }
#endif

    alarm(config->timeout);

    switch (setjmp(state)) {
        case 0: {
                    if (mqtt_connect(config) == -1) {
                        exit_code = NAGIOS_CRITICAL;

                        if (config->mqtt_connect_result) {
                            // XXX: Print connect errors
                            switch (config->mqtt_connect_result) {
                                case 1: {
                                            fprintf(stdout, "connection refused (unacceptable protocol version)\n");
                                            break;
                                        }
                                case 2: {
                                            fprintf(stdout, "connection refused (identifier rejected)\n");
                                            break;
                                        }
                                case 3: {
                                            fprintf(stdout, "connection refused (broker unavailable)\n");
                                            break;
                                        }
                                default: {
                                             fprintf(stdout, "reserved return code %d\n", config->mqtt_connect_result);
                                             break;
                                         }
                            }
                        } else {
                            fprintf(stdout, "%s | mqtt_rtt=U;%d;%d;0\n", mosquitto_strerror(config->mqtt_error), config->warn, config->critical);
                        }
                    } else {
                        if (config->payload_received) {
                            delay = get_delay(config->send_time, config->receive_time);
                            rtt = timespec2double_ms(delay);
                            fprintf(stdout, "Response received after %.1fms | mqtt_rtt=%.3fms;%d;%d;0\n", rtt, rtt, config->warn, config->critical);

                            if (rtt >= (double) config->critical) {
                                exit_code = NAGIOS_CRITICAL;
                            } else if (rtt >= (double) config->warn) {
                                exit_code = NAGIOS_WARNING;
                            } else {
                                exit_code = NAGIOS_OK;
                            }
                        } else {
                            fprintf(stdout, "No response received | mqtt_rtt=U;%d;%d;0\n", config->warn, config->critical);
                            exit_code = NAGIOS_CRITICAL;
                        }
                    };
                    break;
                }
        case ERROR_TIMEOUT: {
                                fprintf(stdout, "Timeout after %d seconds | mqtt_rtt=U;%d;%d;0\n", config->warn, config->critical);
                                exit_code = NAGIOS_CRITICAL;
                                break;
                            }
        case ERROR_MQTT_CONNECT_FAILED:
        case ERROR_MQTT_SUBSCRIBE_FAILED:
        case ERROR_MQTT_PUBLISH_FAILED: {
                                            fprintf(stdout, "%s | mqtt_rtt=U;%d;%d;0\n", mosquitto_strerror(config->mqtt_error), config->warn, config->critical);
                                            exit_code = NAGIOS_CRITICAL;
                                            break;
                                        }
        case ERROR_OOM: {
                            fprintf(stdout, "Memory allocation failed | mqtt_rtt=U;%d;%d;0\n", config->warn, config->critical);
                            exit_code = NAGIOS_CRITICAL;
                            break;
                        }
        default: {
                     printf("Unknown error condition | mqtt_rtt=U;%d;%d;0\n", config->warn, config->critical);
                     exit_code = NAGIOS_UNKNOWN;
                     break;
                 }
    }

leave:
    if (config) {
        free_configuration(config);
        free(config);
    }
    exit(exit_code);
}

