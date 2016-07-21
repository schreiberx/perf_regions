#!/usr/bin/env python2.7
'''
Script to replace timing_start/end calls in all nemo files
with perf_region_start/end calls.

Author: Tim Graham, Martin Schreiber
'''

from glob import glob
import re
import os



class perf_regions:

	def find_and_replace(self, filename, cleanup = False):
		'''Replace timing_start lines and add region names to region_name_list'''
		#TODO: This function could do with a bit of tidying/refactoring/error checking

		if filename in self.excluded_files_list:
			return

		in_content = open(filename).read().splitlines()
		out_content = []

		length = len(in_content)

		i = 0
		while i < length:
			line = in_content[i]

			# Search for exising preprocessed code
			if line == "!PERF_REGION_ORIGINAL":
				print("Found original line in code")
				# next line is the original code
				i = i+1
				line_backup = in_content[i]

				# next line is instrumented code tag
				i = i+1
				line = in_content[i]

				if line != "!PERF_REGION_CODE":
					raise UserWarning("PERF_REGION_CODE NOT DETECTED!")

				# next line is instrumented code -> overwrite
				i = i+1

				in_content[i] = line_backup[1:]
				continue

			# iterate over regular expressions
			line_processed = False
			if not cleanup:
				for p in range(0, 5):
					match = self.prog_match_list[p].match(line)
					if match:
						line_processed = True
						if p == 0:	# initialization
							print("Found initialization statement")
							if self.language == 'fortran':
								out_content.append("!PERF_REGION_ORIGINAL")
								out_content.append("!"+line)
								out_content.append("!PERF_REGION_CODE")
								out_content.append("CALL timing_init(...)")
							break

						elif p == 1:	# shutdown
							print("Found shutdown statement")
							if self.language == 'fortran':
								out_content.append("!PERF_REGION_ORIGINAL")
								out_content.append("!"+line)
								out_content.append("!PERF_REGION_CODE")
								out_content.append("CALL timing_init(...)")
							break

						elif p == 2:	# use/include
							print("Found include/use statement")
							if self.language == 'fortran':
								out_content.append("!PERF_REGION_ORIGINAL")
								out_content.append("!"+line)
								out_content.append("!PERF_REGION_CODE")
								out_content.append("USE perf_regions")
							break

						elif p == 3:	# start timing
							name = match.group(1)
							name = name.upper()
							print("Found start of region "+name)

							if self.language == 'fortran':
								out_content.append("!PERF_REGION_ORIGINAL")
								out_content.append("!"+line)
								out_content.append("!PERF_REGION_CODE")
								out_content.append("CALL perf_region_start(PERF_REGION_"+name+", IOR(PERF_FLAG_TIMINGS, PERF_FLAG_COUNTERS))")
							break

						elif p == 4:	# end timing
							name = match.group(1)
							name = name.upper()
							print("Found end of region "+name)

							if self.language == 'fortran':
								out_content.append("!PERF_REGION_ORIGINAL")
								out_content.append("!"+line)
								out_content.append("!PERF_REGION_CODE")
								out_content.append("CALL perf_region_end(PERF_REGION_"+name+"")
							break
						else:
							raise UserWarning("Unknown match id")

			if line_processed == False:
				out_content.append(line)
				#print line

			i = i+1

		open(filename, 'w').write("\n".join(out_content))
		return



	def find_files(self):
		'''
		Return a list of all files to be modified
		'''
		if not isinstance(self.list_dirs, list):
			src_dirs=[self.list_dirs]

                if self.language == 'fortran':
			file_ext = '*[fF]90'

		files = []
		for src_dir in self.list_dirs:
                        these_files = [y for x in os.walk(src_dir) for y in glob(os.path.join(x[0], file_ext))]

			if len(these_files)==0: 
				raise UserWarning('One or more src_dirs contain no recognised files')
			files.extend(these_files)

		if len(files)==0: raise RuntimeError('No recognised files found in '+src_dir.join(','))

		return files


	region_name_list = []



	def __init__(
		self,
		i_list_dirs,
		i_pattern_match_list,		# 0: init, 1: shutdown, 2: use/include, 3: start region, 4: stop region
		i_language = 'fortran',		# Fortran language
		i_excluded_files_list = []
	):
		self.region_name_list = []

		self.excluded_files_list = i_excluded_files_list
		self.list_dirs = i_list_dirs
		self.pattern_match_list = i_pattern_match_list
		self.prog_match_list = [re.compile(p) for p in self.pattern_match_list]
		self.language = i_language

		if self.language != 'fortran':
			raise UserWarning('Unsupported lanugage')

		pass


	def cleanup(self):
		file_list = self.find_files()
		for filename in file_list:
			self.find_and_replace(filename, True)
		

	def preprocessor(self):
		file_list = self.find_files()
		for filename in file_list:
			self.find_and_replace(filename)

