#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <x86intrin.h>

#ifndef NRUNS
#define NRUNS 100000
#endif

#ifndef N
#define N 10000
#endif

static int64_t
sum(int * array, size_t n)
{
    int64_t total = 0;
    for (size_t i = 0; i < n; i++) {
        total += array[i];
    }
    return total;
}

int main()
{
    int array[N];
    for (size_t i = 0; i < N; i++) {
        array[i] = i;
    }

    int64_t min_time = LONG_MAX;
    for (size_t run = 0; run < NRUNS; run++) {
        int64_t startTime = __rdtsc();
        int64_t total = sum(array, N);
        int64_t endTime = __rdtsc();
        int64_t duration = endTime - startTime;
        if (duration < min_time)
            min_time = duration;
        if (run == 0)
            printf("%" PRId64 "\n", total);
    }
    fprintf(stderr, "%ld\n", min_time);
    return 0;
}
