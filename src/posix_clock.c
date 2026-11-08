#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <time.h>
#include <stdio.h>

#ifdef _POSIX_TIMERS
static clockid_t clock_type;
static int clock_type_initialized = 0;
#endif

/** Checks whether we have support for POSIX timers on this system.
Sets the clock type and queries the resolution (in seconds). Returns 1 if
POSIX timers are supported and 0 otherwise */
int posix_clock_init(double *res){
  struct timespec ts;

#ifdef _POSIX_TIMERS

#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
  clock_type = CLOCK_MONOTONIC;
#else
  clock_type = CLOCK_REALTIME;
#endif

  if (clock_getres(clock_type, &ts) != 0)
  {
    fprintf(stderr, "TIMING: ERROR: clock_getres() failed.\n");
    *res = 0.0;
    return 0;
  }

  *res = (double)(ts.tv_sec) + 1.0e-9*ts.tv_nsec;
  clock_type_initialized = 1;
  return 1;
#else
  /* This system doesn't have support for POSIX timers */
  fprintf(stderr, "TIMING: ERROR: POSIX TIMERS not available.\n");
  *res = 0.0;
  return 0;
#endif

}

/** Returns the current time (in seconds) as measured by the clock selected
    in posix_clock_init() */
double posix_clock(void){
  struct timespec ts;
  double tnow;

#ifdef _POSIX_TIMERS
  if (!clock_type_initialized || clock_gettime(clock_type, &ts) != 0)
    return 0.0;

  tnow = (double)(ts.tv_sec) + 1.0e-9*ts.tv_nsec;
#else
  tnow = 0.0;
#endif

  return tnow;
}
