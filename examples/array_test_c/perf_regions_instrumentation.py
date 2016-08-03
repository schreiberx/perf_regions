#!/usr/bin/env python2.7


import sys
sys.path.append('../../scripts')

import perf_regions

pf = perf_regions.perf_regions(
		["./"],	# list with source directories
		[
			".*#pragma perf_region init.*",		# initialization of timing
			".*#pragma perf_region finalize.*",	# shutdown of timing

			".*#pragma perf_region include.*",	# include part

			".*#pragma perf_region start (.*).*",	# start of timing
			".*#pragma perf_region stop (.*).*",	# end of timing
		],
#		'../../',	# perf region root directory
		'./',		# output directory of perf region tools
		'c'
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
