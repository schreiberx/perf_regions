/** Define interfaces to POSIX-related timing routines */

/** Checks whether we have support for POSIX timers on this system.
Sets the clock type and queries the resolution (in seconds). Returns 1 if
POSIX timers are supported and 0 otherwise */
int posix_clock_init(double *res);

/** Returns the current time (in seconds) as measured by the clock selected
    in posix_clock_init() */
double posix_clock(void);
