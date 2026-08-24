#include "../Platform.h"
#include <time.h>

long GetTimeNano() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000L + now.tv_nsec;
}
