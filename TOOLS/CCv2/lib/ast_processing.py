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

from pycparser import c_ast


def process_ast(cast_orig, cluster_description, target_file_name):
    # TODO:
    # 1. find buffer names
    # 2. find rank names
    # 3. determine buffer size (and return them)
    # 4. detect FPGA parts based on rank (from cluster description)
    # 5. modify AST with constant values (i.e. copy into new)
    # 5. generate C code again and save in file
    return   # ZRLMPI_MAX_DETECTED_BUFFER_SIZE

