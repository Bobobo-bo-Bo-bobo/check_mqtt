#ifndef __CHECK_MQTT_UTIL_H__
#define __CHECK_MQTT_UTIL_H__

char *uuidgen(void);
void free_configuration(struct configuration *);
long str2long(const char *);

#ifndef HAVE_MEMSET
#include <stddef.h>
void *memset(void *, int, size_t);
#else /* HAVE_MEMSET */
#include <string.h>
#endif /* HAVE_MEMSET */

#ifndef HAVE_MEMCPY
#include <stddef.h>
void *memcpy(void *, const void *, size_t);
#else /* HAVE_MEMCPY */
#include <string.h>
#endif /* HAVE_MEMCPY */

#include <time.h>
struct timespec get_delay(const struct timespec, const struct timespec);
double timespec2double_ms(const struct timespec);

#ifdef DEBUG
void print_configuration(const struct configuration *);
#endif

int read_password_from_file(const char *, struct configuration *);

#endif /* __CHECK_MQTT_UTIL_H__ */

