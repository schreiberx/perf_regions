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

		num_lines = len(in_content)

		i = 0
		while i < num_lines:
			line = in_content[i]
			#print("SOURCE: "+line)

			# Search for exising preprocessed code
			if line == self.original_comment:
				print("Found original line in code ("+line+")")
				# next line is the original code
				i = i+1
				line_backup = in_content[i]
				print("         original code ("+line_backup+")")

				# next line is instrumented code tag
				i = i+1
				line = in_content[i]

				if line != self.new_code_comment:
					raise UserWarning("PERF_REGION_CODE NOT DETECTED!")

				print("         found new code line ("+line+")")
				print("         new code ("+in_content[1]+")")

				# next line is instrumented code -> overwrite
				i = i+1

				if self.language == 'fortran':
					print line_backup[1:]
					if line_backup[1:] == '[PERF_REGION_DUMMY]':
						i = i+1
						continue
					in_content[i] = line_backup[1:]	# remove first comment symbol '#'
				elif self.language == 'c':
					in_content[i] = line_backup[2:]	# remove  comment symbol '//'

				continue

			# iterate over regular expressions
			line_processed = False
			if not cleanup:
				for p in range(0, len(self.prog_match_list)):
					match = self.prog_match_list[p].match(line)
					if match:
						line_processed = True
						if p == 0:	# initialization
							print("Found initialization statement")
							if self.language == 'fortran':
								self.append_content(out_content, line, "CALL perf_regions_init()")
							elif self.language == 'c':
								self.append_content(out_content, line, "perf_regions_init();")
							break

						elif p == 1:	# shutdown
							print("Found shutdown statement")
							if self.language == 'fortran':
								self.append_content(out_content, line, "CALL perf_regions_finalize()")
							elif self.language == 'c':
								self.append_content(out_content, line, "perf_regions_finalize();")
							break

						elif p == 2:	# use/include
							print("Found include/use statement")
							if self.language == 'fortran':
								self.append_content(out_content, line, 'USE perf_regions_fortran')
								self.append_content(out_content, '[PERF_REGION_DUMMY]', '#include "perf_region_defines.h"')
							elif self.language == 'c':
								self.append_content(out_content, line, "#include <perf_regions.h>")
							break

						elif p == 3:	# start timing
							name = match.group(1)
							name = name.upper()

							if name in self.region_name_list:
								print("WARNING: "+name+" already exists in the region name list")
							else:
								self.region_name_list.append(name)

							id = self.region_name_list.index(name);

							print("Found start of region "+name)
							if self.language == 'fortran':
								self.append_content(out_content, line, "CALL perf_region_start("+str(id)+", IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !"+name)
							elif self.language == 'c':
								self.append_content(out_content, line, "perf_region_start("+str(id)+", (PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS)); //"+name)
							break

						elif p == 4:	# end timing
							name = match.group(1)
							name = name.upper()
							print("Found end of region "+name)

							id = self.region_name_list.index(name);
							if self.language == 'fortran':
								self.append_content(out_content, line, "CALL perf_region_stop("+str(id)+") !"+name)
							elif self.language == 'c':
								self.append_content(out_content, line, "perf_region_stop("+str(id)+"); //"+name)
						elif p == 5:   # timing_reset
							print("Found timing_reset call")
							if self.language == 'fortran':
								self.append_content(out_content, line, "! No perf_region equivalent" )
							elif self.language == 'c':
								self.append_content(out_content, line, "// No perf_region equivalent" )
						else:
							raise UserWarning("Unknown match id")

			if line_processed == False:
				out_content.append(line)
				#print line

			i = i+1

		open(filename, 'w').write("\n".join(out_content))
		return


	def append_content(self, content, old_line, new_line):
		content.append(self.original_comment)
		if old_line != '':
			content.append(self.comment_prefix + old_line)
		content.append(self.new_code_comment)
		content.append(new_line)


	def find_files(self):
		'''
		Return a list of all files to be modified
		'''
		if not isinstance(self.list_dirs, list):
			src_dirs=[self.list_dirs]

		if self.language == 'fortran':
			file_ext = '*[fF]90'
		elif self.language == 'c':
			file_ext = '*c'

		files = []
		for src_dir in self.list_dirs:
			these_files = [y for x in os.walk(src_dir) for y in glob(os.path.join(x[0], file_ext))]

			if len(these_files)==0: 
				raise UserWarning('One or more src_dirs contain no recognised files')
			files.extend(these_files)

		if len(files)==0:
			raise RuntimeError('No recognised files found in '+src_dirs.join(','))

		return files


	region_name_list = []



	def __init__(
		self,
		i_list_dirs,
		i_pattern_match_list,		# 0: init, 1: shutdown, 2: use/include, 3: start region, 4: stop region
#		i_perfregion_root_dir,		# root directory of perf regions
		i_header_output_file,		# output file of header
		i_language = 'fortran',		# Fortran language
		i_excluded_files_list = []
	):
		self.region_name_list = []

		self.excluded_files_list = i_excluded_files_list
		self.list_dirs = i_list_dirs
		self.pattern_match_list = i_pattern_match_list
		self.prog_match_list = [re.compile(p) for p in self.pattern_match_list]
		self.language = i_language

#		self.perfregion_root_dir = i_perfregion_root_dir
		self.header_output_file = i_header_output_file

		if self.language not in ['fortran', 'c']:
			raise UserWarning('Unsupported lanugage')

		if self.language == 'fortran':
			self.original_comment = '!PERF_REGION_ORIGINAL'
			self.new_code_comment = '!PERF_REGION_CODE'
			self.comment_prefix = '!'
			
		elif self.language == 'c':
			self.original_comment = '//PERF_REGION_ORIGINAL'
			self.new_code_comment = '//PERF_REGION_CODE'
			self.comment_prefix = '//'
		pass


	def cleanup(self):
		file_list = self.find_files()
		for filename in file_list:
			self.find_and_replace(filename, True)

		print
		print("*"*80)
		print("* FINISHED WITHOUT ERRORS")
		print("*"*80)


	def preprocessor(self):
		file_list = self.find_files()
		print("Processing files "+str(file_list))
		for filename in file_list:
			self.find_and_replace(filename)

		print
		print("*"*80)
		print("Region name list:")
		print("*"*80)
		print(self.region_name_list)

		print
		print("*"*80)
		print("Creating header files")
		print("*"*80)

		if not os.path.exists(self.header_output_file):
		    os.makedirs(self.header_output_file)
		self.p_write_header(self.header_output_file+'/perf_region_list.txt')

		print
		print("*"*80)
		print("* FINISHED WITHOUT ERRORS")
		print("*"*80)


	def p_write_header(self, filepath):
		"""Simply write a text file with all the region names"""

		with open(filepath, 'w') as file_handler:
		    for item in self.region_name_list:
			file_handler.write(item+"\n")
