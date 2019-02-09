#include "check_mqtt.h"

#include <setjmp.h>

void alarm_handler(int signo) {
    longjmp(state, ERROR_TIMEOUT);
};

