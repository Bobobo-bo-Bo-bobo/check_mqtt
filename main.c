#include "check_mqtt.h"
#include "usage.h"
#include "util.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

const char *const short_opts = "hH:p:c:k:C:iQ:T:t:su:P:w:W:";
const struct option long_opts[] = {
    { "help", no_argument, NULL, 'h' },
    { "host", required_argument, NULL, 'H' },
    { "port", required_argument, NULL, 'p' },
    { "cert", required_argument, NULL, 'c' },
    { "key", required_argument, NULL, 'k' },
    { "ca", required_argument, NULL, 'C' },
    { "insecure", no_argument, NULL, 'i' },
    { "qos", required_argument, NULL, 'Q' },
    { "topic", required_argument, NULL, 'T' },
    { "timeout", required_argument, NULL, 't' },
    { "ssl", no_argument, NULL, 's' },
    { "user", required_argument, NULL, 'u' },
    { "password", required_argument, NULL, 'P' },
    { "warn", required_argument, NULL, 'w' },
    { "critical", required_argument, NULL, 'W' },
    { NULL, 0, NULL, 0 },
};
int main(int argc, char **argv) {
    struct configuration *config;

    config = (struct configuration *) malloc(sizeof(struct configuration));
    if (!config) {
        fprintf(stderr, "Failed to allocate %d bytes of memory for configuration\n", sizeof(struct configuration));
        exit(NAGIOS_UNKNOWN);
    }
    memset((void *)config, 0, sizeof(struct configuration));

    usage();

    if (config) {
        free_configuration(config);
        free(config);
    }
}

