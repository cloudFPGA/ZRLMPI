#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Jan 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Script that manages all the processing of the C AST
#  *
#  *

import os
import subprocess

from pycparser import c_ast, c_generator
import lib.mpi_signature_name_visitor as name_visitor


def process_ast(cast_orig, cluster_description, hw_file_pre_parsing, target_file_name):
    # TODO:
    # 1. find buffer names
    # 2. find rank names
    find_name_visitor = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor.visit(cast_orig)
    buffer_functions_names, buffer_functions_obj = find_name_visitor.get_results_buffers()
    rank_variable_names, rank_variable_obj = find_name_visitor.get_results_ranks()
    print(buffer_functions_names)
    print(rank_variable_names)
    # 3. determine buffer size (and return them)
    # 4. detect FPGA parts based on rank (from cluster description)
    # 5. modify AST with constant values (i.e. copy into new)
    new_ast = cast_orig
    # 5. generate C code again
    # 6. append original header lines and save in file

    generator = c_generator.CGenerator()
    generated_c = str(generator.visit(new_ast))

    awk_command = "awk '/int.*main\(/{ print NR; exit }'  " + str(hw_file_pre_parsing)
    print(awk_command)
    awk_out = subprocess.check_output(awk_command, shell=True)
    line_number = int(chr(awk_out[0]))
    # print(line_number)

    head = ""
    with open(hw_file_pre_parsing, 'r') as in_file:
         head = [next(in_file) for x in range(line_number - 1)]

    head_str = ""
    for e in head:
        head_str += str(e)

    concatenated_file = head_str + "\n" + generated_c

    with open(target_file_name, 'w+') as target_file:
        target_file.write(concatenated_file)
    return   # ZRLMPI_MAX_DETECTED_BUFFER_SIZE

