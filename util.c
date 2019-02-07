#include "check_mqtt.h"
#include "util.h"

#include <stdlib.h>
#include <uuid.h>

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

    if (cfg->topic) {
        free(cfg->topic);
    }

    if (cfg->user) {
        free(cfg->user);
    }

    if (cfg->password) {
        free(cfg->password);
    }

    memset((void *) cfg, 0, sizeof(struct configuration));
}

