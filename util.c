#include "check_mqtt.h"
#include "util.h"

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

