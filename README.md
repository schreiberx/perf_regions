**************************************
* Performance regions library
**************************************

* PAPI

Check for availability of events:

```
$ papi_avail
```

In case that performance events are not available, add the line

```
kernel.perf_event_paranoid = -1
```

in ```/etc/sysctl.conf```


* How to use perf_regions

Measure performance of different regions

To compile in debug mode, use:
	make MODE=debug
Always test your implementation first with the debug mode!

Performance can be either measured via
	- the wallclock time or
	- the performance counters


src/		Source code of perf regions
examples/	Example files
scripts/	Scripts to instrument code

examples/array_test_c	Test example with C-code
examples/nemo_test_f	Test example with Fortran code
