#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Jan 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Various tools for ZRLMPIcc
#  *
#  *

import os
import subprocess

__directives_file_template__ = "######################################################\n" \
                               "# THIS FILE is GENERATED AUTOMATICALLY by ZRLMPICC   #\n" \
                               "# It contains Vivado HLS tcl directives for app_hw   #\n" \
                               "# It is read by 'run_hls.tcl' during synthesis       #\n" \
                               "######################################################\n\n\n"

delete_line_pattern ="ZRLMPI_THIS_LINE_MUST_BE_DELETED"
clib_funcs_to_mark_for_delete = ['malloc']


def get_line_number_of_occurence(pattern_escaped, filename):
    awk_command = "awk /'" + str(pattern_escaped) + "/{ print NR; exit }'  " + str(filename)
    # print(awk_command)
    awk_out = subprocess.check_output(awk_command, shell=True)
    bytes_to_convert = []
    for e in awk_out:
        if e != 0xa:
            bytes_to_convert.append(e)
    # line_number = int(chr(awk_out[0]))
    line_number = 0
    r = 0
    for n in reversed(bytes_to_convert):
        line_number += int(chr(n))*(10**r)
        r += 1
    # print(line_number)
    return line_number


def generate_tcl_directive(directives_to_add):
    ret = __directives_file_template__
    for d in directives_to_add:
        ret += d + "\n"
    ret += "\n"
    return ret


def delete_marked_lines(lines):
    filtered = []
    for l in lines.splitlines():
        if not (delete_line_pattern in l):
            filtered.append(l)
    ret = "\n".join(filtered)
    return ret

