#define _POSIX_C_SOURCE 199309L
#include "timer.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

#if 0
static uint32_t
ndigits10(uint64_t x)
{
    uint32_t result = 0;
    do {
        result++;
        x /= 10;
    } while (x);
    return result;
}
#else
static uint32_t
ndigits10(uint64_t x) {
    uint32_t ndigits = 1;
    while (1) {
        if (LIKELY(x < 10)) return ndigits;
        if (LIKELY(x < 100)) return ndigits+1;
        if (LIKELY(x < 1000)) return ndigits+2;
        if (LIKELY(x < 10000)) return ndigits+3;
        x /= 10000U;
        ndigits += 4;
    }
}
#endif

static int
sprint_int(char * s, int64_t x)
{
    int negative = 0;
    if (x < 0) {
        x *= -1;
        negative = 1;
        *s++ = '-';
    }
    int ndigits = ndigits10(x);
    for (int i = 1; i <= ndigits; i++) {
        int64_t y = x/10;
        s[ndigits-i] = (x - y*10) + '0';
        x = y;
    }
    s[ndigits] = '\0';
    return ndigits + negative;
}

#define NRUNS 800
#define NITER 20000

#ifndef DEFAULT_NUMBER
#define DEFAULT_NUMBER 123456789UL
#endif

int main(int argc, char * argv[])
{
    char s[32];
    char s2[32];

    uint64_t number = DEFAULT_NUMBER;
    if (argc >= 2)
        number = strtoll(argv[1], NULL, 0);

    int ndigits = ndigits10(number);

    declare_timer(1);

    long min_run_time = LONG_MAX;
    for (int run = 0; run < NRUNS; run++) {
        start_timer(1);
        for (int i = 0; i < NITER; i++) {
            sprint_int(s, number);
#if 0
            sprintf(s2, "%ld", number);
            if (strcmp(s, s2) != 0) {
                fprintf(stderr, "s: %s\ns2: %s\n", s, s2);
                assert(0);
            }
#endif
        }
        stop_timer(1);
        long run_time = get_elapsed_ns(1);
        //fprintf(stderr, "time: %ld\n", run_time);
        if (run_time < min_run_time)
            min_run_time = run_time;
    }
    //fprintf(stderr, "min_time: %ld\n", min_run_time);
    //fprintf(stderr, "%d digits: min avg time: %ld\n", ndigits, min_run_time/NITER);
    fprintf(stderr, "%ld\n", min_run_time/NITER);
}
