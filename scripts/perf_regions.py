#!/usr/bin/env python3

"""
Instrument source files with perf_region_start/perf_region_stop calls.

Supports native perf_regions annotations and NEMO-style timing_start/stop
calls.

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
            "init": r"^.*!(\$|pragma\s+)perf_regions\s+init(\s*)$",  # initialization of timing
            "init_mpi": r"^\s*!(\$|pragma\s+)perf_regions\s+init_mpi(\s*)$",  # initialization of timing when using MPI
            "finalize": r"^\s*!(\$|pragma\s+)perf_regions\s+finalize(\s*)$",  # shutdown of timing
            "include": r"^\s*!(\$|pragma\s+)perf_regions\s+include(\s*)$",  # include part
            "start": r"^\s*!(\$|pragma\s+)perf_regions\s+start\s+(.*)$",  # start of timing
            "stop": r"^\s*!(\$|pragma\s+)perf_regions\s+stop\s+(.*)$",  # end of timing
            "reset": r"^\s*!(\$|pragma\s+)perf_regions\s+reset(\s*)$",  # reset timing
            # Compatibility with NEMO-style timing_* calls
            "timing_use": r"^\s*use\s+timing\s*$",
            "timing_init": r"^\s*call\s+timing_init\s*\(\s*\)\s*$",
            "timing_finalize": r"^\s*call\s+timing_finalize\s*\(\s*\)\s*$",
            "timing_start": r"^\s*call\s+timing_start\s*\(\s*(['\"])(.*?)\1\s*\)\s*;?\s*$",
            "timing_stop": r"^\s*call\s+timing_stop\s*\(\s*(['\"])(.*?)\1\s*\)\s*;?\s*$",
        }
        self.fortran_ignore_case = True

        self.c_language_ext = ["c", "cpp", "cxx"]
        self.c_region_pattern_match = {
            "init": r"^\s*#pragma\s+perf_regions\s+init\s*$",  # initialization of timing
            "init_mpi": r"^\s*#pragma\s+perf_regions\s+init_mpi\s*$",  # initialization of timing when using MPI
            "finalize": r"^\s*#pragma\s+perf_regions\s+finalize\s*$",  # shutdown of timing
            "include": r"^\s*#pragma\s+perf_regions\s+include\s*$",  # include part
            "start": r"^\s*#pragma\s+perf_regions\s+start\s+(.*)$",  # start of timing
            "stop": r"^\s*#pragma\s+perf_regions\s+stop\s+(.*)$",  # end of timing
            "reset": r"^\s*#pragma\s+perf_regions\s+reset\s*$",  # reset timing
        }
        self.c_ignore_case = False

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

    def _cleanup_name_tag(self, name: str) -> str:
        # Remove leading/trailing spaces
        name = name.strip()

        # Replace spaces with underscores
        name = name.replace(" ", "_")

        if name.startswith("'") and name.endswith("'"):
            name = name[1:-1]

        if name.startswith('"') and name.endswith('"'):
            name = name[1:-1]

        return name

    @staticmethod
    def _escape_code_string(name: str, language: str) -> str:
        if language == "c":
            return name.replace("\\", "\\\\").replace('"', '\\"')
        # Fortran escapes a double quote by doubling it.
        return name.replace('"', '""')

    @staticmethod
    def _comment_safe_name(name: str) -> str:
        return name.replace('"', "'")

    def _region_id_for_start(self, name: str) -> int:
        if name in self.region_name_list_acc:
            print("WARNING: " + name + " already exists in the region name list")
            return self.region_name_list_acc.index(name)

        self.region_name_list_acc.append(name)
        return len(self.region_name_list_acc) - 1

    def _region_id_for_stop(self, name: str, filename: str, line_no: int) -> int:
        try:
            return self.region_name_list_acc.index(name)
        except ValueError:
            raise Exception(
                f"Stop without matching start for region '{name}' in "
                f"'{filename}' at line {line_no}"
            )

    def find_and_replace(self, filename_in: str, filename_out: Optional[str] = None, cleanup: bool = False):
        """
        Replace timing_start lines and add region names to region_name_list
        """

        if filename_in in self.excluded_files_list:
            return

        if filename_out is None:
            filename_out = filename_in

        in_content_raw: str = open(filename_in).read()
        in_content: List[str] = in_content_raw.splitlines()
        out_content: List[str] = []

        num_lines = len(in_content)

        file_ext = filename_in.split(".")[-1]
        if file_ext in self.fortran_language_ext:
            language = "fortran"
            original_comment = self.fortran_original_comment
            new_code_comment = self.fortran_new_code_comment
            prog_match_dict = self.fortran_region_pattern_match
            ignore_case = self.fortran_ignore_case

        elif file_ext in self.c_language_ext:
            language = "c"
            original_comment = self.c_original_comment
            new_code_comment = self.c_new_code_comment
            prog_match_dict = self.c_region_pattern_match
            ignore_case = self.c_ignore_case

        else:
            raise Exception(f"Unknown file extension '{file_ext}'")

        depth = 1

        line_id = 0
        while line_id < num_lines:
            line: str = in_content[line_id]

            prefix_str: str = "  " * depth

            # Search for exising preprocessed code
            if line.startswith(original_comment):
                if not cleanup:
                    raise Exception(f"Found existing PERF_REGION_ORIGINAL in file '{filename_in}', cleanup first!")

                print(f"{prefix_str}- Found original line in code: '{line}'")
                # next line is the original code
                line_id = line_id + 1
                line_original = in_content[line_id]
                # out_content.append(line_backup)
                print(f"{prefix_str}  - original code (to be restored): '{line_original}'")

                # next line is instrumented code tag
                line_id = line_id + 1
                line = in_content[line_id]

                if line != new_code_comment:
                    raise UserWarning("PERF_REGION_CODE NOT DETECTED!")

                # next line is instrumented code -> overwrite
                line_id = line_id + 1

                print(f"{prefix_str}  - found injected code (to be removed): '{in_content[line_id]}'")

                if language == "fortran":
                    assert line_original[0] == "!"
                    in_content[line_id] = line_original[1:]  # remove first comment symbol '#'

                elif language == "c":
                    assert line_original[0:2] == "//"
                    in_content[line_id] = line_original[2:]  # remove  comment symbol '//'

                continue

            # iterate over regular expressions
            line_processed = False

            if not cleanup:
                for p, m_pattern in prog_match_dict.items():

                    if ignore_case:
                        match = re.compile(m_pattern, re.IGNORECASE).match(line)
                    else:
                        match = re.compile(m_pattern).match(line)

                    if not match:
                        continue

                    if match.lastindex is not None:
                        match_tag = match.group(match.lastindex)

                        # Remove leading space
                        while len(match_tag) > 0:
                            if match_tag[0] != " ":
                                break
                            match_tag = match_tag[1:]

                    else:
                        match_tag = None

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

                    if p == "finalize":  # finalize
                        print(f"{prefix_str}- Found finalize statement")
                        depth -= 1
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "CALL perf_regions_finalize()", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "perf_regions_finalize();", language=language)
                        self.perf_regions_finalize_found = True
                        break

                    if p == "include":  # use/include
                        print(f"{prefix_str}- Found include/use statement")
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "USE perf_regions_fortran", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "#include <perf_regions.h>", language=language)
                        break

                    if p == "start":  # start timing
                        name = self._cleanup_name_tag(match_tag)
                        id = self._region_id_for_start(name)

                        print(f"{prefix_str}- Found region start '{name}'")
                        depth += 1
                        escaped_name = self._escape_code_string(name, language)
                        if language == "fortran":
                            self._append_source_lines(
                                out_content, line, f"""CALL perf_region_start({id}, "{escaped_name}")""", language=language
                            )
                        elif language == "c":
                            self._append_source_lines(
                                out_content, line, f"""perf_region_start({id}, "{escaped_name}");""", language=language
                            )
                        break

                    if p == "stop":  # timing stop
                        name = self._cleanup_name_tag(match_tag)

                        print(f"{prefix_str}- Found region end '{name}'")
                        depth -= 1

                        id = self._region_id_for_stop(name, filename_in, line_id + 1)
                        comment_name = self._comment_safe_name(name)
                        if language == "fortran":
                            self._append_source_lines(
                                out_content, line, f"CALL perf_region_stop({id})   ! name=\"{comment_name}\"", language=language
                            )
                        elif language == "c":
                            self._append_source_lines(
                                out_content, line, f"perf_region_stop({id});   // name=\"{comment_name}\"", language=language
                            )
                        break

                    if p == "timing_use":
                        print(f"{prefix_str}- Found NEMO 'use timing' statement")
                        self._append_source_lines(out_content, line, "USE perf_regions_fortran", language=language)
                        break

                    if p == "timing_init":
                        print(f"{prefix_str}- Found NEMO timing_init statement")
                        if not self.perf_regions_init_found:
                            depth += 1
                        self._append_source_lines(out_content, line, "CALL perf_regions_init()", language=language)
                        self.perf_regions_init_found = True
                        break

                    if p == "timing_finalize":
                        print(f"{prefix_str}- Found NEMO timing_finalize statement")
                        depth -= 1
                        self._append_source_lines(out_content, line, "CALL perf_regions_finalize()", language=language)
                        self.perf_regions_finalize_found = True
                        break

                    if p == "timing_start":
                        name = self._cleanup_name_tag(match_tag)
                        id = self._region_id_for_start(name)
                        print(f"{prefix_str}- Found NEMO timing_start '{name}'")
                        depth += 1
                        escaped_name = self._escape_code_string(name, language)
                        self._append_source_lines(
                            out_content, line, f"""CALL perf_region_start({id}, "{escaped_name}")""", language=language
                        )
                        break

                    if p == "timing_stop":
                        name = self._cleanup_name_tag(match_tag)
                        id = self._region_id_for_stop(name, filename_in, line_id + 1)
                        print(f"{prefix_str}- Found NEMO timing_stop '{name}'")
                        depth -= 1
                        comment_name = self._comment_safe_name(name)
                        self._append_source_lines(
                            out_content, line, f"CALL perf_region_stop({id})   ! name=\"{comment_name}\"", language=language
                        )
                        break

                    if p == "reset":  # timing_reset
                        print(f"{prefix_str}- Found timing_reset call")
                        if language == "fortran":
                            self._append_source_lines(out_content, line, "! No perf_region equivalent", language=language)
                        elif language == "c":
                            self._append_source_lines(out_content, line, "// No perf_region equivalent", language=language)
                        break

                    raise Exception(f"Unknown perf_regions tag '{p}'")

            if line_processed is False:
                out_content.append(line)

            line_id = line_id + 1

        out_content_raw = "\n".join(out_content)
        if len(in_content_raw) > 0:
            if in_content_raw[-1] == "\n":
                out_content_raw += "\n"
        print(f"- Writing output to file '{filename_out}'")
        open(filename_out, "w").write(out_content_raw)

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


def main(argv: Optional[List[str]] = None) -> int:
    import argparse

    parser = argparse.ArgumentParser(
        description="Instrument or restore perf_regions annotations."
    )
    parser.add_argument(
        "mode",
        choices=("preprocess", "cleanup"),
        help="Instrument files or restore the original source.",
    )
    parser.add_argument(
        "targets",
        nargs="*",
        help="Files or glob patterns. Defaults to ./build/*.",
    )
    parser.add_argument(
        "--output-directory",
        default=None,
        help="Write instrumented files to this directory.",
    )
    args = parser.parse_args(argv)

    targets = args.targets or ["./build/*"]
    expanded: List[str] = []
    for target in targets:
        matches = glob.glob(target)
        expanded.extend(matches or [target])

    preprocessor = PerfRegions(
        expanded,
        output_directory=args.output_directory,
    )

    if args.mode == "preprocess":
        preprocessor.run_preprocessor()
    else:
        preprocessor.remove_perf_regions_annotations()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
