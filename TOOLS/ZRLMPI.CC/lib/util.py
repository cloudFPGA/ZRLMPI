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

