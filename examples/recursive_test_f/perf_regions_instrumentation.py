#!/usr/bin/env python2.7


import sys
sys.path.append('../../scripts')

import perf_regions

pf = perf_regions.perf_regions(
		["./"],	# list with source directories
		[
			".*timing_init.*",		# initialization of timing
			".*timing_finalize.*",	# shutdown of timing

			".*!pragma perf_region include.*",	# include part

			".*timing_start\(\'(.*)\'\)",	# start of timing
			".*timing_stop\(\'(.*)\'\)",	# end of timing
			".*timing_reset.*"		# reset of timing
		],
#		'../../',	# perf region root directory
		'./',		# output directory of perf region tools
		'fortran'
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
