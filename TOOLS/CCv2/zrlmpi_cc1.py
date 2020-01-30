#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Dec 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Main python script of ZRLMPIcc
#  *
#  *


import re
import os
import sys
import subprocess
import json

from pycparser import parse_file, c_parser, c_generator, c_ast
import lib.ast_processing as ast_processing


__match_regex__ = []
__replace_hw__ = []
__replace_sw__ = []
__match_regex_BEFORE_CC__ = []
__replace_hw_BEFORE_CC__ = []
__replace_sw_BEFORE_CC__ = []
__SKIP_STRING__ = "CONTINUE"

__SW_LIB_PATH__ = "../../LIB/SW/"
__TMP_DIR__ = "/cc_delme/"

# include "mpi"
__match_regex_BEFORE_CC__.append('#include "mpi.h"')
# __replace_hw_BEFORE_CC__.append('#include "MPI.hpp"')
# __replace_sw_BEFORE_CC__.append('#include "MPI.hpp"')
__replace_hw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')
__replace_sw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')

# include <mpi>
__match_regex_BEFORE_CC__.append('#include <mpi.h>')
# __replace_hw_BEFORE_CC__.append('#include "MPI.hpp"')
# __replace_sw_BEFORE_CC__.append('#include "MPI.hpp"')
__replace_hw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')
__replace_sw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')

# Main
__match_regex__.append('int main( int argc, char **argv )')
__replace_hw__.append('int app_main(\n    // ----- MPI_Interface -----\n    stream<MPI_Interface> *soMPIif,\n    stream<Axis<8> > *soMPI_data,\n    stream<Axis<8> > *siMPI_data\n    )')
__replace_sw__.append('int app_main()')

#MPI Init
__match_regex__.append('MPI_Init( &argc, &argv )')
__replace_hw__.append('MPI_Init()')
__replace_sw__.append('MPI_Init()')

#MPI Send
__match_regex__.append('MPI_Send(')
__replace_hw__.append('MPI_Send(soMPIif, soMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

#MPI Recv
__match_regex__.append('MPI_Recv(')
__replace_hw__.append('MPI_Recv(soMPIif, siMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)


def zrlmpi_cc_v0(inputSWOnly, inputHW, hw_out_file, sw_out_file):
    hw_out = inputHW
    sw_out = inputSWOnly

    for i in range(0, len(__match_regex__)):
        if __replace_hw__[i] != __SKIP_STRING__:
            hw_out = re.sub(re.escape(__match_regex__[i]), __replace_hw__[i], hw_out)
        if __replace_sw__[i] != __SKIP_STRING__:
            sw_out = re.sub(re.escape(__match_regex__[i]), __replace_sw__[i], sw_out)

    hw_out_file.write(hw_out)
    sw_out_file.write(sw_out)


def zrlmpi_regex_before_cc(inputSWOnly, inputHW, hw_out_file, sw_out_file):
    hw_out = inputHW
    sw_out = inputSWOnly

    for i in range(0, len(__match_regex_BEFORE_CC__)):
        if __replace_hw_BEFORE_CC__[i] != __SKIP_STRING__:
            hw_out = re.sub(re.escape(__match_regex_BEFORE_CC__[i]), __replace_hw_BEFORE_CC__[i], hw_out)
        if __replace_sw_BEFORE_CC__[i] != __SKIP_STRING__:
            sw_out = re.sub(re.escape(__match_regex_BEFORE_CC__[i]), __replace_sw_BEFORE_CC__[i], sw_out)

    hw_out_file.write(hw_out)
    sw_out_file.write(sw_out)


def gcc_file_parsing(own_dir, tmp_dir, hw_input_file, orig_header, hw_header_file, hw_out_file):
    working_dir = own_dir + "/" + tmp_dir
    # link correct header
    ln_command = "ln -s {}/{} {}/{}".format(working_dir, os.path.basename(hw_header_file), working_dir, os.path.basename(orig_header))
    print(ln_command)
    os.popen("rm -f {}/{}".format(working_dir, os.path.basename(orig_header))).read()
    os.popen(ln_command).read()
    # TODO: error handling
    # parsed_file = own_dir+"/app_parsed.cc"
    # print("parsed file will be "+parsed_file)
    gcc_command = "gcc -std=c99 -pedantic -Wall -Wextra -Wconversion -Werror  -nostdinc -E -D'__attribute__(x)=' " +\
                  "-I{}/pycparser/utils/fake_libc_include  -I{}/{} {} > {}".format(own_dir, own_dir, __SW_LIB_PATH__, hw_input_file, hw_out_file)
    print(gcc_command)
    os.popen(gcc_command).read()
    # return parsed_file
    return hw_out_file


def unifdef_split(own_dir, input):
    sw_pre = subprocess.run(["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(input), "-DZRLMPI_SW_ONLY", "-UDEBUG"], stdout=subprocess.PIPE, cwd=os.getcwd())
    hw_pre = subprocess.run(["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(input), "-UZRLMPI_SW_ONLY", "-UDEBUG"], stdout=subprocess.PIPE, cwd=os.getcwd())
    return hw_pre, sw_pre


def get_main_ast(parsed_c_file):
    ast = parse_file(parsed_c_file, use_cpp=True)
    # for e in ast.ext[-1].body.block_items:
    #     if type(e) == c_ast.Decl:
    #         print(e.name)
    #     elif type(e) == c_ast.FuncCall:
    #         print(e.name.name)
    ast_main_function = ast.ext[-1]
    # TODO: is this enough or should we work with the complete ast?
    # return ast
    return ast_main_function


if __name__ == '__main__':
    if len(sys.argv) != 8:
        print("USAGE: {0} mpi_input.c mpi_input.h hw_output_file.c hw_output_file.h sw_output_file.c sw_output_file.h cluster.json".format(sys.argv[0]))
        exit(1)

    own_dir = os.path.dirname(os.path.realpath(__file__))
    tmp_hw_file_c = own_dir + __TMP_DIR__+ "/tmp_hw1.c"
    tmp_hw_file_h = own_dir + __TMP_DIR__+ "/tmp_hw1.h"
    tmp_sw_file_c = own_dir + __TMP_DIR__+ "/tmp_sw1.c"
    tmp_sw_file_h = own_dir + __TMP_DIR__+ "/tmp_sw1.h"

    # clean tmp dir
    # .read() as way to wait until done
    os.popen("rm -rf {}/{}".format(own_dir, __TMP_DIR__)).read()
    os.popen("mkdir {}/{}".format(own_dir, __TMP_DIR__)).read()

    hw_pre_c, sw_pre_c = unifdef_split(own_dir, sys.argv[1])
    hw_pre_h, sw_pre_h = unifdef_split(own_dir, sys.argv[2])

    with open(tmp_hw_file_c, 'w+') as out_hw, open(tmp_sw_file_c, 'w+') as out_sw:
        zrlmpi_regex_before_cc(sw_pre_c.stdout.decode('utf-8'), hw_pre_c.stdout.decode('utf-8'), out_hw, out_sw)
    with open(tmp_hw_file_h, 'w+') as out_hw, open(tmp_sw_file_h, 'w+') as out_sw:
        zrlmpi_regex_before_cc(sw_pre_h.stdout.decode('utf-8'), hw_pre_h.stdout.decode('utf-8'), out_hw, out_sw)

    tmp2_hw_file_c = own_dir + __TMP_DIR__ + "/tmp_hw2.c"
    # tmp2_hw_file_h = own_dir + "/tmp_hw2.h"

    parsed_file = gcc_file_parsing(own_dir, __TMP_DIR__, tmp_hw_file_c, sys.argv[2], tmp_hw_file_h, tmp2_hw_file_c)
    cluster_description = {}
    with open(sys.argv[7], 'r') as in_config:
        cluster_description = json.load(in_config)
    # TODO: do pycparser transfromations and store results in tmp3

    tmp3_hw_file_c = own_dir + __TMP_DIR__ + "/tmp_hw3.c"
    # tmp3_hw_file_h = own_dir + "/tmp_hw3.h" # header will not change

    main_ast = get_main_ast(parsed_file)
    zrlmpi_max_buffer_size = ast_processing.process_ast(main_ast, cluster_description, tmp_hw_file_c, tmp3_hw_file_c)

    # substitute #include statements
    file_name = os.path.basename(sys.argv[1])[:-4]
    __match_regex__.append('#include "{0}.hpp"'.format(file_name))
    __replace_hw__.append('#include "app_hw.hpp"')
    __replace_sw__.append('#include "app_sw.hpp"')


    # c
    with open(sys.argv[3], 'w+') as hw_out_file, open(sys.argv[5], 'w+') as sw_out_file, \
            open(tmp3_hw_file_c) as hw_in_file, open(tmp_sw_file_c) as sw_in_file:
        zrlmpi_cc_v0(sw_in_file.read(), hw_in_file.read(), hw_out_file, sw_out_file)
    # h
    # TODO: add ZRLMPI_MAX_DETECTED_BUFFER_SIZE to HW header
    with open(sys.argv[4], 'w+') as hw_out_file, open(sys.argv[6], 'w+') as sw_out_file, \
            open(tmp_hw_file_h) as hw_in_file, open(tmp_sw_file_h) as sw_in_file:
        zrlmpi_cc_v0(sw_in_file.read(), hw_in_file.read(), hw_out_file, sw_out_file)
