#ifndef TIMER_H
#define TIMER_H

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 199309L
#error "must define _POSIX_C_SOURCE >= 199309L"
#endif

/*
 * EXAMPLE:
 *
 *  declare_timer(1);
 *
 *  start_timer(1);
 *
 *  my_func();
 *
 *  stop_timer(1);
 *  printf("%ld\n", get_elapsed_ns(1));
 *
 */

#include <time.h>

#define declare_timer(X) \
    struct timespec start_time ## X, end_time ## X; \
    time_t dsec ## X; \
    long   dnsec ## X;

#define start_timer(X) \
    do {\
        clock_gettime(CLOCK_MONOTONIC, &start_time ## X);\
    } while(0)

#define stop_timer(X) \
    do {\
        clock_gettime(CLOCK_MONOTONIC, &end_time ## X); \
        dsec ## X  = end_time ## X.tv_sec - start_time ## X.tv_sec; \
        dnsec ## X = end_time ## X.tv_nsec - start_time ## X.tv_nsec; \
    } while (0)

#define get_elapsed_s(X)  (dsec ## X * 1000000000 + dnsec ## X)/1000000000
#define get_elapsed_ms(X) (dsec ## X * 1000000000 + dnsec ## X)/1000000
#define get_elapsed_us(X) (dsec ## X * 1000000000 + dnsec ## X)/1000
#define get_elapsed_ns(X) (dsec ## X * 1000000000 + dnsec ## X)

#define print_timer_s(X)  fprint_timer_s(stdout, X)
#define print_timer_ms(X) fprint_timer_ms(stdout, X)
#define print_timer_us(X) fprint_timer_us(stdout, X)
#define print_timer_ns(X) fprint_timer_ns(stdout, X)

#define fprint_timer_s(stream, X)  fprintf(stream, "timer " #X ": %ld s\n",  get_elapsed_s(X))
#define fprint_timer_ms(stream, X) fprintf(stream, "timer " #X ": %ld ms\n", get_elapsed_ms(X))
#define fprint_timer_us(stream, X) fprintf(stream, "timer " #X ": %ld us\n", get_elapsed_us(X))
#define fprint_timer_ns(stream, X) fprintf(stream, "timer " #X ": %ld ns\n", get_elapsed_ns(X))

#endif /* TIMER_H */
