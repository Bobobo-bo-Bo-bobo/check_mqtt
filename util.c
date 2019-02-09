#include "check_mqtt.h"
#include "util.h"

#include <mosquitto.h>
#include <errno.h>
#include <stdlib.h>
#include <uuid.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#ifndef HAVE_MEMSET
#include <stddef.h>
void *memset(void *s, int c, size_t n) {
    unsigned char* p=s;
    while(n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}
#else /* HAVE_MEMSET */
#include <string.h>
#endif /* HAVE_MEMSET */

#ifndef HAVE_MEMCPY
void *memcpy(void *d, const void *s, size_t n) {
    char *dest = d;
    const char *src = s;

    for (size_t i=0; i < n; i++) {
        *dest++ = *src++;
    }
    return d;
}
#else /* HAVE_MEMCPY */
#include <string.h>
#endif /* HAVE_MEMCPY */

char *uuidgen(void) {
    char *result;
    uuid_t uuid;

    // UUID string is always 36 byte long + terminating \0
    result = (char *) malloc(37);
    if (!result) {
        // XXX: it is critical if we don't have enough memory to allocate 37 byte
        // OTOH mosquitto_new will generate a ugly random id
        return NULL;
    }

    uuid_generate(uuid);
    uuid_unparse(uuid, result);
    return result;
}

void free_configuration(struct configuration *cfg) {
    if (!cfg) {
        return;
    }

    if (cfg->host) {
        free(cfg->host);
    }

    if (cfg->cert) {
        free(cfg->cert);
    }

    if (cfg->key) {
        free(cfg->key);
    }

    if (cfg->ca) {
        free(cfg->ca);
    }

    if (cfg->cadir) {
        free(cfg->cadir);
    }

    if (cfg->topic) {
        free(cfg->topic);
    }

    if (cfg->user) {
        free(cfg->user);
    }

    if (cfg->password) {
        free(cfg->password);
    }

    if (cfg->payload) {
        free(cfg->payload);
    }

    if (cfg->mqtt_handle) {
        mosquitto_destroy(cfg->mqtt_handle);
    }

    memset((void *) cfg, 0, sizeof(struct configuration));
}

long str2long(const char *str) {
    char *remain;
    long result;

    result = strtol(str, &remain, 10);
    if ((errno == ERANGE && (result == LONG_MAX || result == LONG_MIN)) || (errno != 0 && result == 0)) {
        fprintf(stderr, "ERROR: Can't convert %s to long\n", str);
        return LONG_MIN;
    }
    if (str == remain) {
        fprintf(stderr, "ERROR: Can't convert %s to long\n", str);
        return LONG_MIN;
    }
    if (*remain != 0) {
        fprintf(stderr, "ERROR: Can't convert %s to long\n", str);
        return LONG_MIN;
    }
    return result;
}

struct timespec get_delay(const struct timespec begin, const struct timespec end) {
    struct timespec delta;

    // did the nano seconds counters wrap?
    if (end.tv_nsec - begin.tv_nsec < 0) {
        delta.tv_sec = end.tv_sec - begin.tv_sec - 1;
        delta.tv_nsec = 1e+09 + (end.tv_nsec - begin.tv_nsec);
    } else {
        delta.tv_sec = end.tv_sec - begin.tv_sec;
        delta.tv_nsec = end.tv_nsec - begin.tv_nsec;
    }

    return delta;
}

double timespec2double_ms(const struct timespec ts) {
    return (double) (1.0e+09 * ts.tv_sec + ts.tv_nsec)*1.0e-06;
}

#ifdef DEBUG
void print_configuration(const struct configuration *cfg) {
    if (cfg->host) {
        printf("host (0x%0x): %s\n", (void *) cfg->host, cfg->host);
    } else {
        printf("host (NULL)\n");
    }

    printf("port: %d\n", cfg->port);

    if (cfg->cert) {
        printf("cert (0x%0x): %s\n", (void *) cfg->cert, cfg->cert);
    } else {
        printf("cert (NULL)\n");
    }

    if (cfg->key) {
        printf("key (0x%0x): %s\n", (void *) cfg->key, cfg->key);
    } else {
        printf("key (NULL)\n");
    }

    if (cfg->ca) {
        printf("ca (0x%0x): %s\n", (void *) cfg->ca, cfg->ca);
    } else {
        printf("ca (NULL)\n");
    }

    if (cfg->cadir) {
        printf("cadir (0x%0x): %s\n", (void *) cfg->cadir, cfg->cadir);
    } else {
        printf("cadir (NULL)\n");
    }

    printf("insecure: %s (%d)\n", cfg->insecure?"true":"false", cfg->insecure);

    printf("qos: %d\n", cfg->qos);

    if (cfg->topic) {
        printf("topic (0x%0x): %s\n", (void *) cfg->topic, cfg->topic);
    } else {
        printf("topic (NULL)\n");
    }

    printf("timeout: %d\n", cfg->timeout);

    printf("ssl: %s (%d)\n", cfg->ssl?"true":"false", cfg->ssl);

    if (cfg->user) {
        printf("user (0x%0x): %s\n", (void *) cfg->user, cfg->user);
    } else {
        printf("user (NULL)\n");
    }

    if (cfg->password) {
        printf("password (0x%0x): %s\n", (void *) cfg->password, cfg->password);
    } else {
        printf("password (NULL)\n");
    }

    printf("warn: %d\n", cfg->warn);

    printf("critical: %d\n", cfg->critical);

    if (cfg->payload) {
        printf("payload (0x%0x): %s\n", (void *) cfg->payload, cfg->payload);
    } else {
        printf("payload (NULL)\n");
    }

    printf("mqtt_connect_result: %d\n", cfg->mqtt_connect_result);

    printf("mqtt_error: %d\n", cfg->mqtt_error);

    printf("payload_received: %s (%d)\n", cfg->payload_received?"true":"false", cfg->payload_received);

    printf("keepalive: %d\n", cfg->keep_alive);

    printf("send_time.tv_sec: %d\n", cfg->send_time.tv_sec);
    printf("send_time.tv_nsec: %d\n", cfg->send_time.tv_nsec);

    printf("receive_time.tv_sec: %d\n", cfg->receive_time.tv_sec);
    printf("receive_time.tv_nsec: %d\n", cfg->receive_time.tv_nsec);

    printf("debug: %s (%d)\n", cfg->debug?"true":"false", cfg->debug);
};
#endif

int read_password_from_file(const char *file, struct configuration *cfg) {
    char *buffer;
    FILE *fd;
    char *token;

    buffer = (char *) malloc(READ_BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate read buffer\n");
        return ENOMEM;
    }
    memset((void *) buffer, 0, READ_BUFFER_SIZE);

    fd = fopen(file, "r");
    if (!fd) {
        free(buffer);
        return errno;
    }

    // XXX; In theory fread may return less than READ_BUFFER_SIZE bytes.
    //      Although unlikely it may be neccessary to handle such corner case
    fread((void *) buffer, sizeof(char), READ_BUFFER_SIZE, fd);
    if (ferror(fd)) {
        fprintf(stderr, "Can't read from %s\n", file);
        free(buffer);
        fclose(fd);
        return EIO;
    }

    // XXX: fclose mail fail too
    fclose(fd);

    // we only read the first line and don't care about the rest (if any)
    token = strtok(buffer, "\r\n");

    // token is NULL if the file is empty or contains only the delimeter
    if (token) {
        // if password has already set, free old string
        if (cfg->password) {
            free(cfg->password);
        }
        cfg->password = strdup(token);
        if (!cfg->password) {
            free(buffer);
            return ENOMEM;
        }
    }
    free(buffer);

    return 0;
}

