#!/usr/bin/env python3

"""
Script to replace timing_start/end calls in all NEMO files
with perf_region_start/end calls.

Author: Tim Graham, Martin Schreiber
"""

import glob
import re
import os
from typing import List, Optional


class PerfRegions:
    def __init__(
        self,
        list_dirs: List[str],
        excluded_files_list: List[str] = [],
        output_directory: Optional[str] = None,  # output file of header
    ):
        self.list_dirs: List[str] = list_dirs
        self.excluded_files_list: List[str] = excluded_files_list
        self.output_directory: str = output_directory

        # Accumulator of all found region names
        self.region_name_list_acc: List[str] = []

        self.fortran_language_ext = ["F90", "f90", "F", "f"]
        self.fortran_region_pattern_match = {
            "init": "^.*!\$perf_regions init(| .*)$",  # initialization of timing
            "init_mpi": "^.*!\$perf_regions init_mpi(| .*)$",  # initialization of timing when using MPI
            "finalize": "^.*!\$perf_regions finalize(| .*)$",  # shutdown of timing
            "include": "^.*!\$perf_regions include(| .*)$",  # include part
            "start": "^.*!\$perf_regions start(| .*)$",  # start of timing
            "stop": "^.*!\$perf_regions stop(| .*)$",  # end of timing
            "reset": "^.*!\$perf_regions reset(| .*)$",  # reset timing
        }

        self.c_language_ext = ["c", "cpp", "cxx"]
        self.c_region_pattern_match = {
            "init": "^.*#pragma perf_regions init(| .*)$",  # initialization of timing
            "init_mpi": "^.*#pragma perf_regions init_mpi(| .*)$",  # initialization of timing when using MPI
            "finalize": "^.*#pragma perf_regions finalize(| .*)$",  # shutdown of timing
            "include": "^.*#pragma perf_regions include(| .*)$",  # include part
            "start": "^.*#pragma perf_regions start(| .*)$",  # start of timing
            "stop": "^.*#pragma perf_regions stop(| .*)$",  # end of timing
            "reset": "^.*#pragma perf_regions reset(| .*)$",  # reset timing
        }

        if not isinstance(self.list_dirs, List):
            assert isinstance(self.list_dirs, str)
            self.list_dirs = [self.list_dirs]

        self.fortran_original_comment: str = "!PERF_REGION_ORIGINAL"
        self.fortran_new_code_comment: str = "!PERF_REGION_CODE"
        self.fortran_comment_prefix: str = "!"

        self.c_original_comment: str = "//PERF_REGION_ORIGINAL"
        self.c_new_code_comment: str = "//PERF_REGION_CODE"
        self.c_comment_prefix: str = "//"

        self.perf_regions_init_found: bool = False
        self.perf_regions_finalize_found: bool = False

    def find_and_replace(self, filename_in: str, filename_out: Optional[str] = None, cleanup: bool = False):
        """
        Replace timing_start lines and add region names to region_name_list
        """

        if filename_in in self.excluded_files_list:
            return

        if filename_out is None:
            filename_out = filename_in

        in_content: List[str] = open(filename_in).read().splitlines()
        out_content: List[str] = []

        num_lines = len(in_content)

        file_ext = filename_in.split(".")[-1]
        if file_ext in self.fortran_language_ext:
            language = "fortran"
            original_comment = self.fortran_original_comment
            new_code_comment = self.fortran_new_code_comment
            prog_match_dict = self.fortran_region_pattern_match

        elif file_ext in self.c_language_ext:
            language = "c"
            original_comment = self.c_original_comment
            new_code_comment = self.c_new_code_comment
            prog_match_dict = self.c_region_pattern_match

        else:
            raise Exception(f"Unknown file extension '{file_ext}'")
        
        depth = 1

        line_id = 0
        while line_id < num_lines:
            line: str = in_content[line_id]

            prefix_str: str = "  "*depth

            # Search for exising preprocessed code
            if line.startswith(original_comment):
                if not cleanup:
                    raise Exception(f"Found existing PERF_REGION_ORIGINAL in file '{filename_in}', cleanup first!")

                print(f"{prefix_str}- Found original line in code: '{line}'")
                # next line is the original code
                line_id = line_id + 1
                line_backup = in_content[line_id]
                print(f"{prefix_str}  - original code: '{line_backup}'")

                # next line is instrumented code tag
                line_id = line_id + 1
                line = in_content[line_id]

                if line != new_code_comment:
                    raise UserWarning("PERF_REGION_CODE NOT DETECTED!")

                print(f"{prefix_str}  - found new code line: '{line}'")
                print(f"{prefix_str}  - new code: '{in_content[1]}'")

                # next line is instrumented code -> overwrite
                line_id = line_id + 1

                if language == "fortran":
                    if line_backup[1:] == "[PERF_REGION_DUMMY]":
                        line_id = line_id + 1
                        continue
                    assert line_backup[0] == "!"
                    in_content[line_id] = line_backup[1:]  # remove first comment symbol '#'

                elif language == "c":
                    assert line_backup[0:2] == "//"
                    in_content[line_id] = line_backup[2:]  # remove  comment symbol '//'

                continue

            # iterate over regular expressions
            line_processed = False
            if not cleanup:
                for (p, m_pattern) in prog_match_dict.items():

                    match = re.compile(m_pattern).match(line)
                    if not match:
                        continue

                    match_tag = match.group(1)
                    # Remove leading space
                    if len(match_tag) > 0:
                        assert match_tag[0] == " "
                        match_tag = match_tag[1:]

                    line_processed = True
                    if p == "init":  # initialization without mpi
                        print(f"{prefix_str}- Found initialization statement")
                        # Increase depth only once
                        if not self.perf_regions_init_found:
                            depth += 1
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "CALL perf_regions_init()", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "perf_regions_init();", language=language)

                        self.perf_regions_init_found = True
                        break

                    if p == "init_mpi":  # initialization with mpi
                        communicator = match_tag
                        print(f"{prefix_str}- Found initialization statement for communicator {communicator}")
                        # Increase depth only once
                        if not self.perf_regions_init_found:
                            depth += 1
                        if language == "fortran":
                            self._append_source_lines(
                                out_content, line, f"CALL perf_regions_init_mpi_fortran({communicator})", language=language
                            )
                        elif language == "c":
                            self._append_source_lines(
                                out_content, line, f"perf_regions_init_mpi({communicator});", language=language
                            )
                        self.perf_regions_init_found = True
                        break

                    elif p == "finalize":  # finalize
                        print(f"{prefix_str}- Found finalize statement")
                        depth -= 1
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "CALL perf_regions_finalize()", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "perf_regions_finalize();", language=language)
                        self.perf_regions_finalize_found = True
                        break

                    elif p == "include":  # use/include
                        print(f"{prefix_str}- Found include/use statement")
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "USE perf_regions_fortran", language=language)
                            self._append_source_lines(
                                out_content, "[PERF_REGION_DUMMY]", '#include "perf_regions_defines.h"', language=language
                            )
                        elif language == "c":
                            self._append_source_lines(out_content, line, "#include <perf_regions.h>", language=language)
                        break

                    elif p == "start":  # start timing
                        name = match_tag
                        name = name.upper()

                        if name in self.region_name_list_acc:
                            print("WARNING: " + name + " already exists in the region name list")
                        else:
                            self.region_name_list_acc.append(name)

                        id = self.region_name_list_acc.index(name)

                        print(f"{prefix_str}- Found region start '{name}'")
                        depth += 1
                        if language == "fortran":
                            self._append_source_lines(
                                out_content, line, f"""CALL perf_region_start({id}, "{name}"//achar(0))""", language=language
                            )
                        elif language == "c":
                            self._append_source_lines(
                                out_content, line, f"""perf_region_start({id}, "{name}");""", language=language
                            )
                        break

                    elif p == "stop":   # timing stop
                        name = match_tag
                        name = name.upper()
                        print(f"{prefix_str}- Found region end '{name}'")
                        depth -= 1

                        id = self.region_name_list_acc.index(name)
                        if language == "fortran":
                            self._append_source_lines(out_content, line, f"CALL perf_region_stop({id}) !" + name, language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, f"perf_region_stop({id}); //" + name, language=language)

                    elif p == "reset":  # timing_reset
                        print("{prefix_str}- Found timing_reset call")
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "! No perf_region equivalent", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "// No perf_region equivalent", language=language)

                    else:
                        raise Exception(f"Unknown perf_regions tag '{p}'")

            if line_processed == False:
                out_content.append(line)

            line_id = line_id + 1

        open(filename_out, "w").write("\n".join(out_content))


    def _append_source_lines(self, content: List[str], old_line, new_line, language: str):

        if language == "fortran":
            comment_prefix = self.fortran_comment_prefix
            original_comment = self.fortran_original_comment
            new_code_comment = self.fortran_new_code_comment

        elif language == "c":
            comment_prefix = self.c_comment_prefix
            original_comment = self.c_original_comment
            new_code_comment = self.c_new_code_comment

        content.append(original_comment)
        if old_line != "":
            content.append(comment_prefix + old_line)
        content.append(new_code_comment)
        content.append(new_line)

    def _find_source_files(self):
        """
        Return a list of all files matching the file extensions for further processing
        """

        files_to_process: List[str] = []
        for src_dir in self.list_dirs:
            src_dir: str

            for file in glob.glob(src_dir):
                file_ext = file.split(".")[-1]
                if file_ext not in self.fortran_language_ext + self.c_language_ext:
                    continue
                files_to_process.append(file)

        return files_to_process

    def remove_perf_regions_annotations(self):
        file_list = self._find_source_files()
        for filename_input in file_list:
            filename_output: str = filename_input
            self.find_and_replace(filename_input, filename_output, cleanup=True)

        print("FINISHED without errors")


    def run_preprocessor(self):
        """
        Preprocess all files specified by the pattern in the constructor
        """
        file_list = self._find_source_files()

        if self.output_directory is not None:
            os.makedirs(self.output_directory, exist_ok=True)

        for filename_input in file_list:
            if self.output_directory is not None:
                filename_output: str = os.path.join(self.output_directory, os.path.split(filename_input)[-1])
            else:
                filename_output: str = filename_input

            print(f"- Processing file '{filename_input}' -> '{filename_output}'")
            self.find_and_replace(filename_input, filename_output, cleanup=False)

        print("-" * 80)
        print(f"Region name list: {self.region_name_list_acc}")

        if not self.perf_regions_init_found:
            raise Exception("No pref_regions initialization found!")
        
        if not self.perf_regions_finalize_found:
            raise Exception("No pref_regions finalization found!")

        print("FINISHED without errors")
