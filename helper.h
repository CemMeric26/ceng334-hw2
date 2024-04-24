#ifndef HOMEWORK2_HELPER_H
#define HOMEWORK2_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <unistd.h>

#define PASS_DELAY 10

void sleep_milli(int milliseconds) {
    long seconds = milliseconds/1000;
    long microseconds = (milliseconds%1000)*1000;
    if ( seconds > 0 ) {
        sleep(seconds);
    }
    if ( microseconds > 0 ) {
        usleep(microseconds);
    }
}
#ifdef __cplusplus
};
#endif
#endif //HOMEWORK2_HELPER_H
