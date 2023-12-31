# /*******************************************************************************
#  * Copyright 2018 -- 2023 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
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
from lib.util import get_line_number_of_occurence
import lib.template_generator as template_generator
from lib.util import generate_tcl_directive

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
__match_regex_BEFORE_CC__.append('\\#include\\ \\"mpi\\.h\\"')
# __replace_hw_BEFORE_CC__.append('#include "MPI.hpp"')
# __replace_sw_BEFORE_CC__.append('#include "MPI.hpp"')
__replace_hw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')
__replace_sw_BEFORE_CC__.append('#define _ZRLMPI_APP_INCLUDED_\n#include "ZRLMPI.hpp"')

# include <mpi>
__match_regex_BEFORE_CC__.append('\\#include\\ \\<mpi\\.h\\>')
# __replace_hw_BEFORE_CC__.append('#include "MPI.hpp"')
# __replace_sw_BEFORE_CC__.append('#include "MPI.hpp"')
__replace_hw_BEFORE_CC__.append('#include "ZRLMPI.hpp"')
__replace_sw_BEFORE_CC__.append('#define _ZRLMPI_APP_INCLUDED_\n#include "ZRLMPI.hpp"')

# Main
# __match_regex__.append('int\\s*main\\(\\s*int\\ argc\\,\\s*char\\ \\*\\*argv\\s*\\)')
# __replace_hw__.append('void app_main(\n    // ----- MPI_Interface -----\n' +
#                       '    stream<MPI_Interface> *soMPIif,\n' +
#                       '    stream<MPI_Feedback> *siMPIFeB,\n' +
#                       '    stream<Axis<64> > *soMPI_data,\n' +
#                       '    stream<Axis<64> > *siMPI_data,\n' +
#                       '    // ----- DRAM -----\n' +
#                       '    ap_uint<512> boFdram[ZRLMPI_DRAM_SIZE_LINES]\n' +
#                       '    )')
__match_regex_hw_main_position__ = 0
# __match_regex__.append('void\\s*main\\(\\s*int\\ argc\\,\\s*char\\ \\*\\*argv')
__match_regex__.append('main\\(\\s*int\\ argc\\,\\s*char\\ \\*\\*argv')
__replace_hw__.append('app_main(\n    // ----- MPI_Interface -----\n' +
                      '    stream<MPI_Interface> *soMPIif,\n' +
                      '    stream<MPI_Feedback> *siMPIFeB,\n' +
                      '    stream<Axis<64> > *soMPI_data,\n' +
                      '    stream<Axis<64> > *siMPI_data\n' +
                      '    // ----- DRAM -----\n')
__replace_sw__.append('app_main(int argc, char **argv')

# MPI Init
__match_regex__.append('MPI_Init\\(\s*\\&argc\\,\s*\\&argv\s*\\)')
__replace_hw__.append('MPI_Init()')
__replace_sw__.append('MPI_Init()')

# MPI Send
__match_regex__.append('MPI_Send\\(')
__replace_hw__.append('MPI_Send(soMPIif, siMPIFeB, soMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

# MPI Recv
__match_regex__.append('MPI_Recv\\(')
__replace_hw__.append('MPI_Recv(soMPIif, siMPIFeB, siMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

# MPI Send
__match_regex__.append('MPI_Send_DRAM\\(')
__replace_hw__.append('MPI_Send_DRAM(soMPIif, siMPIFeB, soMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

# MPI Recv
__match_regex__.append('MPI_Recv_DRAM\\(')
__replace_hw__.append('MPI_Recv_DRAM(soMPIif, siMPIFeB, siMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

__wrapper_pattern_buffer__ = 'ZRLMPI_BUFFER_DECLS'
__wrapper_pattern_call__ = 'ZRLMPI_APP_MAIN_CALL'
__app_main_call_start__ = '    app_main(soMPIif, siMPIFeB, soMPI_data, siMPI_data'


def copy_and_update_ZRLMPI_wrapper(template_path, target_path, buffer_decls, buffer_names):
    generated_wrapper = None
    buffer_line = get_line_number_of_occurence(__wrapper_pattern_buffer__, template_path)
    call_line = get_line_number_of_occurence(__wrapper_pattern_call__, template_path)
    with open(template_path, 'r') as tfile:
        generated_wrapper = tfile.read().splitlines()
    generated_wrapper[buffer_line] = buffer_decls
    new_call_line = __app_main_call_start__
    for n in buffer_names:
        new_call_line += ', ' + n
    new_call_line += ');'
    generated_wrapper[call_line] = new_call_line
    generated_c = "\n".join(generated_wrapper)
    generated_c += "\n"
    with open(target_path, 'w+') as target_file:
        target_file.write(generated_c)


__vhdl_component_tmpl__ = "; {} : IN STD_LOGIC_VECTOR (63 downto 0)"
__vhdl_port_map_tmpl__ = ', {}     => x"{}" '
__vhdl_component_pattern__ = "ZRLMPI_COMPONENT_INSERT"
__vhdl_port_map_pattern__ = "ZRLMPI_PORT_MAP_INSERT"
__boFdram_default_size_lines__ = 2


def copy_and_update_Role_vhdl(template_path, target_path, buffer_names, buffer_sizes):
    generated_role = None
    component_line = get_line_number_of_occurence(__vhdl_component_pattern__, template_path)
    port_map_line = get_line_number_of_occurence(__vhdl_port_map_pattern__, template_path)
    with open(template_path, 'r') as tfile:
        generated_role = tfile.read().splitlines()
    new_component_lines = []
    for n in buffer_names:
        new_component_lines.append(__vhdl_component_tmpl__.format(n))
    if len(new_component_lines) > 0:
        current_offset = __boFdram_default_size_lines__ * 64  # bytes per line
        new_comp_l = "\n".join(new_component_lines)
        generated_role[component_line] = new_comp_l
        assert len(buffer_sizes) == len(buffer_names)
        new_port_map_lines = []
        for i in range(0, len(buffer_names)):
            offset = "{:016x}".format(current_offset)
            new_port_map = __vhdl_port_map_tmpl__.format(buffer_names[i],offset)
            current_offset += buffer_sizes[i]
            new_port_map_lines.append(new_port_map)
        new_port_l = "\n".join(new_port_map_lines)
        generated_role[port_map_line] = new_port_l
    generated_vhdl = "\n".join(generated_role)
    with open(target_path, 'w+') as target_file:
        target_file.write(generated_vhdl)


def zrlmpi_cc_v0(inputSWOnly, inputHW, hw_out_file, sw_out_file):
    hw_out = inputHW
    sw_out = inputSWOnly

    for i in range(0, len(__match_regex__)):
        if __replace_hw__[i] != __SKIP_STRING__:
            hw_out = re.sub(re.compile(__match_regex__[i], re.IGNORECASE), __replace_hw__[i], hw_out)
        if __replace_sw__[i] != __SKIP_STRING__:
            sw_out = re.sub(re.compile(__match_regex__[i], re.IGNORECASE), __replace_sw__[i], sw_out)

    hw_out_file.write(hw_out)
    sw_out_file.write(sw_out)


def zrlmpi_regex_before_cc(inputSWOnly, inputHW, hw_out_file, sw_out_file):
    hw_out = inputHW
    sw_out = inputSWOnly

    for i in range(0, len(__match_regex_BEFORE_CC__)):
        if __replace_hw_BEFORE_CC__[i] != __SKIP_STRING__:
            hw_out = re.sub(re.compile(__match_regex_BEFORE_CC__[i], re.IGNORECASE), __replace_hw_BEFORE_CC__[i],
                            hw_out)
        if __replace_sw_BEFORE_CC__[i] != __SKIP_STRING__:
            sw_out = re.sub(re.compile(__match_regex_BEFORE_CC__[i], re.IGNORECASE), __replace_sw_BEFORE_CC__[i],
                            sw_out)

    hw_out_file.write(hw_out)
    sw_out_file.write(sw_out)


def gcc_file_parsing(own_dir, tmp_dir, hw_input_file, orig_header, hw_header_file, hw_out_file):
    working_dir = own_dir + "/" + tmp_dir
    os.popen(patch_cmd).read()
    # link correct header
    ln_command = "ln -s {}/{} {}/{}".format(working_dir, os.path.basename(hw_header_file), working_dir,
                                            os.path.basename(orig_header))
    # print(ln_command)
    os.popen("rm -f {}/{}".format(working_dir, os.path.basename(orig_header))).read()
    os.popen(ln_command).read()
    # TODO: error handling
    # parsed_file = own_dir+"/app_parsed.cc"
    # print("parsed file will be "+parsed_file)
    gcc_command = "gcc -std=c99 -pedantic -Wall -Wextra -Wconversion -Werror  -nostdinc -E -D'__attribute__(x)=' " + \
                  "-I{}/pycparser/utils/fake_libc_include  -I{}/{} {} > {}".format(own_dir, own_dir, __SW_LIB_PATH__,
                                                                                   hw_input_file, hw_out_file)
    # print(gcc_command)
    os.popen(gcc_command).read()
    # return parsed_file
    return hw_out_file


def unifdef_split(own_dir, input):
    sw_pre = subprocess.run(
        ["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(input), "-DZRLMPI_SW_ONLY", "-UDEBUG"],
        stdout=subprocess.PIPE, cwd=os.getcwd())
    hw_pre = subprocess.run(
        ["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(input), "-UZRLMPI_SW_ONLY", "-UDEBUG"],
        stdout=subprocess.PIPE, cwd=os.getcwd())
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


def add_header_line_after(pattern_escaped, line_to_add, filename_old, filename_new):
    include_after_line = get_line_number_of_occurence(pattern_escaped, filename_old)
    # print(include_after_line)
    old_header = ""
    with open(filename_old, 'r') as in_file:
        old_header = in_file.readlines()

    old_header.insert(include_after_line, line_to_add)

    new_header_str = ""
    for e in old_header:
        new_header_str += str(e)

    with open(filename_new, 'w+') as target_file:
        target_file.write(new_header_str)
    return include_after_line


if __name__ == '__main__':
    if len(sys.argv) != 14:
        print(
            "USAGE: {0} mpi_input.c mpi_input.h hw_output_file.c hw_output_file.h hw_directives_output.tcl sw_output_file.c " +
            "sw_output_file.h cluster.json cFp.json ZRLMPI_template.cpp ZRLMPI_out_file.cpp Role_template.vhdl Role_out_file.vhdl".format(
                sys.argv[0]))
        exit(1)

    args_mpi_input_c = sys.argv[1]
    args_mpi_input_h = sys.argv[2]
    args_hw_outfile_c = sys.argv[3]
    args_hw_outfile_h = sys.argv[4]
    args_hw_directives_out = sys.argv[5]
    args_sw_outfile_c = sys.argv[6]
    args_sw_outfile_h = sys.argv[7]
    args_cluster_json = sys.argv[8]
    args_cfp_json = sys.argv[9]
    args_zrlmpicc_template = sys.argv[10]
    args_zrlmpicc_target = sys.argv[11]
    args_role_vhdl_template = sys.argv[12]
    args_role_vhdl_target = sys.argv[13]

    own_dir = os.path.dirname(os.path.realpath(__file__))
    tmp_hw_file_c = own_dir + __TMP_DIR__ + "/tmp_hw1.c"
    tmp_hw_file_h = own_dir + __TMP_DIR__ + "/tmp_hw1.h"
    tmp_sw_file_c = own_dir + __TMP_DIR__ + "/tmp_sw1.c"
    tmp_sw_file_h = own_dir + __TMP_DIR__ + "/tmp_sw1.h"

    print("\nStarting cross-compelation...\n")

    # clean tmp dir
    # .read() as way to wait until done
    os.popen("rm -rf {}/{}".format(own_dir, __TMP_DIR__)).read()
    os.popen("mkdir {}/{}".format(own_dir, __TMP_DIR__)).read()

    hw_pre_c, sw_pre_c = unifdef_split(own_dir, args_mpi_input_c)
    hw_pre_h, sw_pre_h = unifdef_split(own_dir, args_mpi_input_h)

    with open(tmp_hw_file_c, 'w+') as out_hw, open(tmp_sw_file_c, 'w+') as out_sw:
        zrlmpi_regex_before_cc(sw_pre_c.stdout.decode('utf-8'), hw_pre_c.stdout.decode('utf-8'), out_hw, out_sw)
    with open(tmp_hw_file_h, 'w+') as out_hw, open(tmp_sw_file_h, 'w+') as out_sw:
        zrlmpi_regex_before_cc(sw_pre_h.stdout.decode('utf-8'), hw_pre_h.stdout.decode('utf-8'), out_hw, out_sw)

    tmp2_hw_file_c = own_dir + __TMP_DIR__ + "/tmp_hw2.c"
    tmp2_hw_file_h = own_dir + __TMP_DIR__ + "/tmp_hw2.h"

    # here, hw and sw files are the same
    parsed_file = gcc_file_parsing(own_dir, __TMP_DIR__, tmp_hw_file_c, sys.argv[2], tmp_hw_file_h, tmp2_hw_file_c)
    cluster_description = {}
    with open(args_cluster_json, 'r') as in_config:
        cluster_description = json.load(in_config)
    # TODO: support range definitions in JSON (convert to array here)

    cFp_description = {}
    with open(args_cfp_json, 'r') as in_config:
        cFp_description = json.load(in_config)

    tmp3_hw_file_c = own_dir + __TMP_DIR__ + "/tmp_hw3.c"
    tmp3_sw_file_c = own_dir + __TMP_DIR__ + "/tmp_sw3.c"
    # tmp3_hw_file_h = own_dir + "/tmp_hw3.h" # header will not change

    # replicator nodes must be the same for all versions
    replicator_nodes = template_generator.calculate_replicator_nodes(cluster_description)
    main_ast = get_main_ast(parsed_file)
    zrlmpi_max_buffer_size_bytes, tcl_lines, buffer_init_src, new_signature_line, buffer_names, buffer_sizes_bytes = \
        ast_processing.process_ast(main_ast, cluster_description, cFp_description, tmp_hw_file_c, tmp3_hw_file_c,
                                   replicator_nodes=replicator_nodes)

    # now, process template for SW file
    main_ast = get_main_ast(parsed_file)
    ignore_return = ast_processing.process_ast(main_ast, cluster_description, cFp_description, tmp_sw_file_c,
                                               tmp3_sw_file_c,
                                               template_only=True, replicator_nodes=replicator_nodes)

    max_buffer_string = "#define ZRLMPI_MAX_DETECTED_BUFFER_SIZE ({})  //in BYTES!\n\n".format(
        zrlmpi_max_buffer_size_bytes)
    add_header_line_after('\#include\ \"ZRLMPI\.hpp\"', max_buffer_string, tmp_hw_file_h, tmp2_hw_file_h)

    # substitute #include statements
    file_name = os.path.basename(sys.argv[1])[:-4]
    __match_regex__.append(re.escape('#include "{0}.hpp"'.format(file_name)))
    __replace_hw__.append('#include "app_hw.hpp"')
    __replace_sw__.append('#include "app_sw.hpp"')


    # c
    with open(args_hw_outfile_c, 'w+') as hw_out_file, open(args_sw_outfile_c, 'w+') as sw_out_file, \
            open(tmp3_hw_file_c) as hw_in_file, open(tmp3_sw_file_c) as sw_in_file:
        zrlmpi_cc_v0(sw_in_file.read(), hw_in_file.read(), hw_out_file, sw_out_file)
    # h
    #shortened_signature_line = new_signature_line[0:9] + new_signature_line[31:-1]  #to remove the last bracket and argc etc...
    #shortened_signature_line = "void " + header_signature_line[4:-2]
    #__replace_hw__[__match_regex_hw_main_position__] = shortened_signature_line
    __replace_hw__[__match_regex_hw_main_position__] += buffer_init_src
    with open(args_hw_outfile_h, 'w+') as hw_out_file, open(args_sw_outfile_h, 'w+') as sw_out_file, \
            open(tmp2_hw_file_h) as hw_in_file, open(tmp_sw_file_h) as sw_in_file:
        zrlmpi_cc_v0(sw_in_file.read(), hw_in_file.read(), hw_out_file, sw_out_file)

    # add "static" tcl directives
    tcl_lines.append("set_directive_inline -region -recursive app_main")

    tcl_file = generate_tcl_directive(tcl_lines)
    with open(args_hw_directives_out, 'w+') as tcl_out_file:
        tcl_out_file.write(tcl_file)

    # take care of ZRLMPI generation
    copy_and_update_ZRLMPI_wrapper(args_zrlmpicc_template, args_zrlmpicc_target, buffer_init_src, buffer_names)

    # take care of Role vhdl
    copy_and_update_Role_vhdl(args_role_vhdl_template, args_role_vhdl_target, buffer_names, buffer_sizes_bytes)

    print("\n...finished cross-compelation.\n")

