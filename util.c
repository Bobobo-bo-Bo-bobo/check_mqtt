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

