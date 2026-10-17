/* Regression: a signal inside Darwin getenv must neither park with its lock
 * held nor lose the next quantum's preemption request. */
#include "platform/vcpu.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint64_t progress[2];
static int64_t deadline;

static int64_t now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static void *guest(void *opaque)
{
    intptr_t i = (intptr_t)opaque;
    vcpu_attach(0);
    while (now_ms() < deadline) {
        const char *value = getenv("VCPU_TEST");
        if (value && *value) progress[i]++;
    }
    vcpu_detach();
    return NULL;
}

int main(void)
{
    pthread_t threads[2];
    setenv("VCPU_TEST", "1", 1);
    deadline = now_ms() + 10000;
    for (intptr_t i = 0; i < 2; i++)
        if (pthread_create(&threads[i], NULL, guest, (void *)i) != 0)
            return 1;
    for (int i = 0; i < 2; i++)
        if (pthread_join(threads[i], NULL) != 0)
            return 1;
    printf("guest progress: %llu, %llu\n",
           (unsigned long long)progress[0], (unsigned long long)progress[1]);
    return !(progress[0] > 1000 && progress[1] > 1000);
}
