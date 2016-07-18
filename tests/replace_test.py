#!/usr/bin/env python2.7


import sys
sys.path.append('../scripts')

import perf_regions

source_dirs = ["./src_replace"]

pf = perf_regions.perf_regions(
		source_dirs,	# list with source directories
		[
			".*CALL timing_init.*",			# initialization of timing
			".*CALL timing_shutdown.*",		# shutdown of timing

			".*USE timing.*",			# include part
			".*CALL timing_start\('(.*)'\).*",	# start of timing
			".*CALL timing_stop\('(.*)'\).*",	# end of timing
		],
		'fortran',
		['timing.F90']	# excluded files
	)


if len(sys.argv) > 1:
	if sys.argv[1] == 'preprocess':
		print("PREPROCESS")
		pf.preprocessor()

	elif sys.argv[1] == 'cleanup':
		print("CLEANUP")
		pf.cleanup()

	else:
		print("Unsupported argument "+sys.argv[1])

else:
	pf.preprocessor()
