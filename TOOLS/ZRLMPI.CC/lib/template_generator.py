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
#  *     Created: July 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Helper functions to generate partial ASTs for collective routines
#  *
#  *

from pycparser import c_ast
import random
import string
from lib.util import delete_line_pattern
from lib.util import clib_funcs_to_mark_for_delete
from lib.resource_checker import maximum_bram_buffer_size_bytes

__scatter_tag__ = c_ast.Constant("int", "0")  # or smth else?
__gather_tag__ = c_ast.Constant("int", "0")  # or smth else?
__bcast_tag__ = c_ast.Constant("int", "0")  # or smth else?
__reduce_tag__ = c_ast.Constant("int", "0")  # or smth else?
__status_ignore__ = c_ast.Constant("int", "0")  # from zrlmpi_common.hpp
__loop_variable__name__ = "template_variable"
__buffer_variable__name__ = "template_buffer"
__new_start_variable_name__ = "new_start"
__new_count_variable_name__ = "new_count"
__random_name_suffix_length__ = 5

# __array_map_directive_string__ = "set_directive_array_map -instance boFdram -mode horizontal {} {}"
# __array_map_directive_string__ = "set_directive_interface -bundle boAPP_DRAM -offset direct -latency 52 -mode m_axi {} {}"
__array_map_directive_string__ = "set_directive_interface -bundle boAPP_DRAM -offset direct -mode m_axi -latency 52 -num_write_outstanding 16 -num_read_outstanding 16 -max_write_burst_length 256 -max_read_burst_length 256 {} {}"
# __default_directive_location__ = "app_main"
__default_directive_location__ = "mpi_wrapper"

# __max_packet_length__ = "(1024/sizeof({})"
# __max_packet_length__ = 256
# __max_packet_length__ = 352
# __max_packet_length__ = 346  # to apply with VXLAN in ZYC2
# __max_packet_length__ = 65536  # for counters in FPGA, and to reduce cost of packet loss
__max_packet_length__ = 262160  # (2^20/4) for counters in FPGA, and to reduce cost of packet loss

__NO_OPTIMIZATION_MSG__ = "NO-Optimization-Possible"

__mpi_op_defines__ = {'0': 'MPI_SUM'}

__loop_optim_split_size__ = 2000
__loop_split_size_constant__ = c_ast.Constant('int', str(__loop_optim_split_size__))


def get_type_string_from_int(datatype_int):
    if datatype_int == 0:
        return "int"
    elif datatype_int == 1:
        return "float"
    else:
        return "void"


def get_reduce_function_from_name_and_type(func_name, datatype_int):
    suffix = ""
    if datatype_int == 0:
        suffix = "INTEGER"
    elif datatype_int == 1:
        suffix = "FLOAT"
    else:
        suffix = "UNKNOWN"
    ret = func_name + '_' + suffix
    return ret


def get_random_name_extension():
    alphabet = string.ascii_lowercase + string.ascii_uppercase
    ret = ''.join(random.choice(alphabet) for i in range(__random_name_suffix_length__))
    return ret


def get_local_copy_buffer_name(orig_name):
    new_buffer_name = "local_copy_{}_{}_L".format(orig_name, get_random_name_extension())
    return new_buffer_name


def scatter_replacement(scatter_call, cluster_size_constant, rank_obj):
    loop_variable_name = "{}_{}_0".format(__loop_variable__name__, get_random_name_extension())
    new_start_variable_name = "{}_{}_0".format(__new_start_variable_name__, get_random_name_extension())
    send_datatype = scatter_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(send_datatype.value))
    chunk_size = scatter_call.args.exprs[1]
    root_rank = scatter_call.args.exprs[6]
    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), cluster_size_constant)
    for_next = c_ast.UnaryOp('p++', c_ast.ID(loop_variable_name))
    loop_stmts = []
    buffer_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                 type=c_ast.PtrDecl([],
                                                    c_ast.TypeDecl(new_start_variable_name, [], c_ast.IdentifierType([datatype_string]))),
                               init=c_ast.BinaryOp("+", scatter_call.args.exprs[0],
                                                   c_ast.BinaryOp("*", c_ast.ID(loop_variable_name), chunk_size)),
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    skip_stmts = []
    src_dst_equal = False
    try:
        if scatter_call.args.exprs[0].expr.name.name.name == scatter_call.args.exprs[3].expr.name.name.name:
            src_dst_equal = True
    except:
        # so we are not sure...
        src_dst_equal = False
    if not src_dst_equal:
        memcpy_args = []
        memcpy_args.append(scatter_call.args.exprs[3])
        memcpy_args.append(c_ast.ID(new_start_variable_name))
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args))
        skip_stmts.append(memcpy)
    # in all cases:
    skip_stmts.append(c_ast.Continue())
    skip_if = c_ast.If(c_ast.BinaryOp("==", c_ast.ID(loop_variable_name), root_rank), c_ast.Compound(skip_stmts), None)
    loop_stmts.append(skip_if)
    send_args = []
    send_args.append(c_ast.ID(new_start_variable_name))
    send_args.append(chunk_size)
    send_args.append(scatter_call.args.exprs[2])
    send_args.append(c_ast.ID(loop_variable_name))
    send_args.append(__scatter_tag__)
    send_args.append(scatter_call.args.exprs[7])
    send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
    loop_stmts.append(send_call)
    for_stmt = c_ast.Compound(loop_stmts)
    then_stmt = []
    then_stmt.append(c_ast.For(loop_variable, for_condition, for_next, for_stmt))
    then_part = c_ast.Compound(then_stmt)
    else_args = []
    else_args.append(scatter_call.args.exprs[3])
    else_args.append(scatter_call.args.exprs[4])
    else_args.append(scatter_call.args.exprs[5])
    else_args.append(root_rank)
    else_args.append(__scatter_tag__)
    else_args.append(scatter_call.args.exprs[7])
    else_args.append(__status_ignore__)
    else_stmts = []
    else_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(else_args)))
    else_part = c_ast.Compound(else_stmts)
    condition = c_ast.BinaryOp("==", rank_obj, root_rank)
    pAST = c_ast.If(condition, then_part, else_part)
    return pAST


def gather_replacement(gather_call, cluster_size_constant, rank_obj):
    loop_variable_name = "{}_{}_1".format(__loop_variable__name__, get_random_name_extension())
    new_start_variable_name = "{}_{}_1".format(__new_start_variable_name__, get_random_name_extension())
    recv_datatype = gather_call.args.exprs[5]
    datatype_string = get_type_string_from_int(int(recv_datatype.value))
    chunk_size = gather_call.args.exprs[4]
    root_rank = gather_call.args.exprs[6]
    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), cluster_size_constant)
    for_next = c_ast.UnaryOp('p++', c_ast.ID(loop_variable_name))
    loop_stmts = []
    buffer_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                 type=c_ast.PtrDecl([],
                                                    c_ast.TypeDecl(new_start_variable_name, [], c_ast.IdentifierType([datatype_string]))),
                                 init=c_ast.BinaryOp("+", gather_call.args.exprs[3],
                                                     c_ast.BinaryOp("*", c_ast.ID(loop_variable_name), chunk_size)),
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    skip_stmts = []
    src_dst_equal = False
    try:
        if gather_call.args.exprs[0].expr.name.name.name == gather_call.args.exprs[3].expr.name.name.name:
            src_dst_equal = True
    except:
        # so we are not sure...
        src_dst_equal = False
    if not src_dst_equal:
        memcpy_args = []
        memcpy_args.append(c_ast.ID(new_start_variable_name))
        memcpy_args.append(gather_call.args.exprs[0])
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args))
        skip_stmts.append(memcpy)
    # in all cases:
    skip_stmts.append(c_ast.Continue())
    skip_if = c_ast.If(c_ast.BinaryOp("==", c_ast.ID(loop_variable_name), root_rank), c_ast.Compound(skip_stmts), None)
    loop_stmts.append(skip_if)
    recv_args = []
    recv_args.append(c_ast.ID(new_start_variable_name))
    recv_args.append(chunk_size)
    recv_args.append(recv_datatype)
    recv_args.append(c_ast.ID(loop_variable_name))
    recv_args.append(__gather_tag__)
    recv_args.append(gather_call.args.exprs[7])
    recv_args.append(__status_ignore__)
    recv_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args))
    loop_stmts.append(recv_call)
    for_stmt = c_ast.Compound(loop_stmts)
    then_stmt = []
    then_stmt.append(c_ast.For(loop_variable, for_condition, for_next, for_stmt))
    then_part = c_ast.Compound(then_stmt)
    send_args = []
    send_args.append(gather_call.args.exprs[0])
    send_args.append(gather_call.args.exprs[1])
    send_args.append(gather_call.args.exprs[2])
    send_args.append(root_rank)
    send_args.append(__gather_tag__)
    send_args.append(gather_call.args.exprs[7])
    send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
    else_stmts = []
    else_stmts.append(send_call)
    else_part = c_ast.Compound(else_stmts)
    condition = c_ast.BinaryOp("==", rank_obj, root_rank)
    pAST = c_ast.If(condition, then_part, else_part)
    return pAST


def bcast_replacement(bcast_call, cluster_size_constant, rank_obj):
    loop_variable_name = "{}_{}_5".format(__loop_variable__name__, get_random_name_extension())
    send_datatype = bcast_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(send_datatype.value))
    chunk_size = bcast_call.args.exprs[1]
    root_rank = bcast_call.args.exprs[3]
    communicator = bcast_call.args.exprs[4]
    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), cluster_size_constant)
    for_next = c_ast.UnaryOp('p++', c_ast.ID(loop_variable_name))
    buffer_variable = bcast_call.args.exprs[0]
    loop_stmts = []
    skip_stmts = []
    skip_stmts.append(c_ast.Continue())
    skip_if = c_ast.If(c_ast.BinaryOp("==", c_ast.ID(loop_variable_name), root_rank), c_ast.Compound(skip_stmts), None)
    loop_stmts.append(skip_if)
    send_args = []
    send_args.append(buffer_variable)
    send_args.append(chunk_size)
    send_args.append(send_datatype)
    send_args.append(c_ast.ID(loop_variable_name))
    send_args.append(__bcast_tag__)
    send_args.append(communicator)
    send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
    loop_stmts.append(send_call)
    for_stmt = c_ast.Compound(loop_stmts)
    then_stmt = []
    then_stmt.append(c_ast.For(loop_variable, for_condition, for_next, for_stmt))
    then_part = c_ast.Compound(then_stmt)
    else_args = []
    else_args.append(bcast_call.args.exprs[0])
    else_args.append(chunk_size)
    else_args.append(send_datatype)
    else_args.append(root_rank)
    else_args.append(__bcast_tag__)
    else_args.append(communicator)
    else_args.append(__status_ignore__)
    else_stmts = []
    else_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(else_args)))
    else_part = c_ast.Compound(else_stmts)
    condition = c_ast.BinaryOp("==", rank_obj, root_rank)
    pAST = c_ast.If(condition, then_part, else_part)
    return pAST


def reduce_replacement(reduce_call, cluster_size_constant, rank_obj):
    loop_variable_name = "{}_{}_7".format(__loop_variable__name__, get_random_name_extension())
    reduce_datatype = reduce_call.args.exprs[3]
    datatype_string = get_type_string_from_int(int(reduce_datatype.value))
    reduce_func = get_reduce_function_from_name_and_type(reduce_call.args.exprs[4].name, int(reduce_datatype.value))
    chunk_size = reduce_call.args.exprs[2]
    root_rank = reduce_call.args.exprs[5]
    orig_communicator = reduce_call.args.exprs[6]
    target_buffer = reduce_call.args.exprs[1]
    source_buffer = reduce_call.args.exprs[0]
    # 1. global inits
    accum_inter_buffer_size = chunk_size
    accum_buffer_variable_name = "{}_{}_7".format(__buffer_variable__name__, get_random_name_extension())
    accum_buffer_variable_decl = c_ast.Decl(name=accum_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                          type=c_ast.ArrayDecl(type=c_ast.TypeDecl(accum_buffer_variable_name, [],
                                                                                   c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                               #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                               dim=accum_inter_buffer_size
                                                               ),
                                          init=None,
                                          bitsize=None)
    accum_buffer_variable_id = c_ast.ID(accum_buffer_variable_name)

    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), cluster_size_constant)
    for_next = c_ast.UnaryOp('p++', c_ast.ID(loop_variable_name))
    loop_stmts = []
    buffer_variable = accum_buffer_variable_decl
    root_stmts = []
    memcpy_args = []
    # memcpy_args.append(accum_buffer_variable_id)
    memcpy_args.append(target_buffer)
    memcpy_args.append(source_buffer)
    sizeof_args = []
    sizeof_args.append(c_ast.Constant('string', datatype_string))
    memcpy_args.append(c_ast.BinaryOp("*", chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
    memcpy = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args))
    root_stmts.append(memcpy)
    # else part
    else_stmts = []
    recv_args = []
    recv_args.append(accum_buffer_variable_id)
    recv_args.append(chunk_size)
    recv_args.append(reduce_datatype)
    recv_args.append(c_ast.ID(loop_variable_name))
    recv_args.append(__reduce_tag__)
    recv_args.append(orig_communicator)
    recv_args.append(__status_ignore__)
    recv_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args))
    else_stmts.append(recv_call)
    # add reduce function
    red_args = []
    red_args.append(target_buffer)
    red_args.append(accum_buffer_variable_id)
    red_args.append(chunk_size)
    red_func = c_ast.FuncCall(c_ast.ID(reduce_func), c_ast.ExprList(red_args))
    else_stmts.append(red_func)
    root_if = c_ast.If(c_ast.BinaryOp("==", c_ast.ID(loop_variable_name), root_rank), c_ast.Compound(root_stmts),
                       c_ast.Compound(else_stmts))
    loop_stmts.append(root_if)
    # loop_stmts.append(red_func)
    for_stmt = c_ast.Compound(loop_stmts)
    then_stmt = []
    then_stmt.append(c_ast.For(loop_variable, for_condition, for_next, for_stmt))
    then_part = c_ast.Compound(then_stmt)
    send_args = []
    send_args.append(source_buffer)
    send_args.append(chunk_size)
    send_args.append(reduce_datatype)
    send_args.append(root_rank)
    send_args.append(__reduce_tag__)
    send_args.append(orig_communicator)
    send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
    else_stmts = []
    else_stmts.append(send_call)
    else_part = c_ast.Compound(else_stmts)
    condition = c_ast.BinaryOp("==", rank_obj, root_rank)
    if_else_tree = c_ast.If(condition, then_part, else_part)
    past_stmts = []
    past_stmts.append(buffer_variable)
    past_stmts.append(if_else_tree)
    pAST = c_ast.Compound(past_stmts)
    return pAST


def calculate_replicator_nodes(cluster_description):
    # the FPGAs have an end2end speedup of 1/3 currently, so we group it to three
    # TODO optimize
    group_size = 4
    fpga_cnt = len(cluster_description['nodes']['fpga'])
    if fpga_cnt < 2:
        return {"msg": __NO_OPTIMIZATION_MSG__}
    group_numbers = int((fpga_cnt + (group_size-1)) / group_size)
    groups = {"msg": "ok", "cnt": group_numbers, "group_size": group_size}
    group_fill_cnt = 0
    current_group = 0
    replicator_nodes = []
    for f in cluster_description['nodes']['fpga']:
        if group_fill_cnt == 0:
            current_group = f
            groups[current_group] = []
            group_fill_cnt += 1
            replicator_nodes.append(f)
        else:
            groups[current_group].append(f)
            group_fill_cnt += 1
        if group_fill_cnt >= group_size:
            group_fill_cnt = 0
    groups["replicators"] = replicator_nodes
    print("found optimized tree structure:" + str(groups))
    return groups


def disassemble_chunk_sizes(n):
    current_obj = n
    found_parts = []
    # found_ints = []
    obj_to_visit = []
    while True:
        if type(current_obj) is c_ast.Constant:
            # found_ints.append(int(current_obj.value))
            # found_parts.append(current_obj)
            found_parts.append(int(current_obj.value))
        elif type(current_obj) is c_ast.ID:
            # found_parts.append(current_obj)
            found_parts.append(current_obj.name)
        elif type(current_obj) is c_ast.BinaryOp:
            found_parts.append(current_obj.op)
            obj_to_visit.append(current_obj.left)
            obj_to_visit.append(current_obj.right)
        else:
            print("  (while disassembling cunk sizes, found unexpected type {})\n".format(type(current_obj)))

        if len(obj_to_visit) > 0:
            current_obj = obj_to_visit[0]
            del obj_to_visit[0]
        else:
            break
    return found_parts


def create_available_buffer_structure():
    ret = {'list': [], 'decls': [], 'ids': [], 'tcls': []}
    return ret


def optimized_scatter_replacement(scatter_call, replicator_nodes, rank_obj, available_buffer_list, template_only):
    """

    :param available_buffer_list: available internal optimization buffers
    :param scatter_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    orig_datatype = scatter_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(orig_datatype.value))
    orig_chunk_size = scatter_call.args.exprs[1]
    real_chunk_size, err = _extract_int_size_of_malloc(orig_chunk_size)
    if err is None:
        orig_chunk_size = c_ast.Constant('int', str(real_chunk_size))
    orig_src_buffer = scatter_call.args.exprs[0]
    orig_root_rank = scatter_call.args.exprs[6]
    orig_communicator = scatter_call.args.exprs[7]
    orig_local_buffer = scatter_call.args.exprs[3]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # for first replicator node, create buffer
    # 1. global inits
    return_array_decl = False
    tcl_list_return = []
    all_inter_buffer_size = c_ast.BinaryOp('*', c_ast.Constant('int', str(replicator_nodes["group_size"])), orig_chunk_size)
    real_all_inter_buffer_size, err = _extract_int_size_of_malloc(all_inter_buffer_size)
    if err is None:
        all_inter_buffer_size = c_ast.Constant('int', str(real_all_inter_buffer_size))
    all_inter_buffer_signature = disassemble_chunk_sizes(all_inter_buffer_size)
    found_buffer = False
    all_buffer_variable_id = None
    all_buffer_variable_decl = None
    if available_buffer_list is not None and len(available_buffer_list['list']) > 0:
        position = 0
        for l in available_buffer_list['list']:
            if l == all_inter_buffer_signature:
                # found one
                all_buffer_variable_decl = available_buffer_list['decls'][position]
                all_buffer_variable_id = available_buffer_list['ids'][position]
                found_buffer = True
                break
            else:
                position += 1
    if not found_buffer:
        all_buffer_variable_name = "{}_{}_4".format(__buffer_variable__name__, get_random_name_extension())
        all_buffer_variable_decl = c_ast.Decl(name=all_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                 type=c_ast.ArrayDecl(type=c_ast.TypeDecl(all_buffer_variable_name, [],
                                                      c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                      #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                      dim=all_inter_buffer_size
                                                      ),
                                 init=None,
                                 bitsize=None)
        all_buffer_variable_id = c_ast.ID(all_buffer_variable_name)
        buffer_real_size, err = _extract_int_size_of_malloc(all_inter_buffer_size)
        tcl_directive = __array_map_directive_string__.format(__default_directive_location__, all_buffer_variable_name)
        if available_buffer_list is not None:
            available_buffer_list['list'].append(all_inter_buffer_signature)
            available_buffer_list['decls'].append(all_buffer_variable_decl)
            available_buffer_list['ids'].append(all_buffer_variable_id)
            # if err is none, there might be some variable in thre
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                available_buffer_list['tcls'].append(tcl_directive)
                return_array_decl = True
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
        else:
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                return_array_decl = True
                tcl_list_return.append(tcl_directive)
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
    # 2. take care of replicator nodes
    intermediate_parts = {}
    intermediate_part_cnts = replicator_nodes['cnt']
    rns = replicator_nodes["replicators"]
    reversed_rns = list(reversed(rns))
    last_processed_rn = 0
    for rr in reversed_rns:
        group_rcv_nodes = replicator_nodes[rr]
        size_of_this_group = len(group_rcv_nodes)
        # 1. receive large chung
        inter_then_part_stmts = []
        if template_only:
            inter_then_part_stmts.append(all_buffer_variable_decl)
        # if rr == rns[0]:
        #     # first node, declare buffer
        #     inter_then_part_stmts.append(all_buffer_variable_decl)
        func_call_args = []
        func_call_args.append(all_buffer_variable_id)
        func_call_args.append(c_ast.BinaryOp('*', c_ast.Constant('int', str(size_of_this_group+1)), orig_chunk_size))
        func_call_args.append(orig_datatype)
        func_call_args.append(orig_root_rank)
        func_call_args.append(__scatter_tag__)
        func_call_args.append(orig_communicator)
        func_call_args.append(__status_ignore__)
        inter_then_part_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(func_call_args)))
        # 2. memcpy own part
        memcpy_args = []
        memcpy_args.append(orig_local_buffer)
        memcpy_args.append(all_buffer_variable_id)
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args))
        inter_then_part_stmts.append(memcpy)
        # 3. distribute to groups
        send_cnt = 1  # start with 1, 0 is the node itself
        for rcvi in group_rcv_nodes:
            send_args = []
            send_args.append(c_ast.BinaryOp('+', all_buffer_variable_id,
                                            c_ast.BinaryOp('*', c_ast.Constant('int', str(send_cnt)),
                                                           orig_chunk_size)))
            send_cnt += 1
            send_args.append(orig_chunk_size)
            send_args.append(orig_datatype)
            send_args.append(c_ast.Constant('int', str(rcvi)))
            send_args.append(__scatter_tag__)
            send_args.append(orig_communicator)
            send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
            inter_then_part_stmts.append(send_call)
        inter_then_part = c_ast.Compound(inter_then_part_stmts)
        # 4. receive in groups
        inter_else_part_plain_stmts = []
        recv_args = []
        recv_args.append(orig_local_buffer)
        recv_args.append(orig_chunk_size)
        recv_args.append(orig_datatype)
        recv_args.append(c_ast.Constant('int', str(rr)))
        recv_args.append(__scatter_tag__)
        recv_args.append(orig_communicator)
        recv_args.append(__status_ignore__)
        inter_else_part_plain_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args)))
        inter_else_part_plain = c_ast.Compound(inter_else_part_plain_stmts)
        # is also if again
        # if -> then receive -> else, next group
        # if last group, no if/else
        if rr == rns[-1]:
            inter_else_part = inter_else_part_plain
        else:
            rcv_compares = []
            for rcvn in group_rcv_nodes:
                new_compare = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rcvn)))
                rcv_compares.append(new_compare)
            inter_else_condition = rcv_compares[0]
            if len(rcv_compares) > 1:
                for i in range(1, len(rcv_compares)):
                    inter_else_condition = c_ast.BinaryOp("||", inter_else_condition, rcv_compares[i])
            inter_else_then_part = inter_else_part_plain
            inter_else_else_part = intermediate_parts[last_processed_rn]
            inter_else_part = c_ast.If(inter_else_condition, inter_else_then_part, inter_else_else_part)
        # 3. assemble
        inter_condition = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rr)))
        inter_part = c_ast.If(inter_condition, inter_then_part, inter_else_part)
        intermediate_parts[rr] = inter_part
        last_processed_rn = rr
    # 3. take care of root node
    root_stmts = []
    src_dst_equal = False
    try:
        if scatter_call.args.exprs[0].expr.name.name.name == scatter_call.args.exprs[3].expr.name.name.name:
            src_dst_equal = True
    except:
        # so we are not sure...
        src_dst_equal = False
    if not src_dst_equal:
        memcpy_args2 = []
        memcpy_args2.append(orig_local_buffer)
        memcpy_args2.append(orig_src_buffer)
        sizeof_args2 = []
        sizeof_args2.append(c_ast.Constant('string', datatype_string))
        memcpy_args2.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args2))))
        memcpy2 = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args2))
        root_stmts.append(memcpy2)
    send_cnt = 1  # start with 1, 0 is the node itself
    for rn in rns:
        group_rcv_nodes = replicator_nodes[rn]
        size_of_this_group = len(group_rcv_nodes)
        send_args = []
        send_args.append(c_ast.BinaryOp('+', orig_src_buffer,
                                    c_ast.BinaryOp('*', c_ast.Constant('int', str(send_cnt)),
                                                   orig_chunk_size)))
        send_cnt += size_of_this_group + 1
        send_args.append(c_ast.BinaryOp('*', c_ast.Constant('int', str(size_of_this_group+1)), orig_chunk_size))
        send_args.append(orig_datatype)
        send_args.append(c_ast.Constant('int', str(rn)))
        send_args.append(__scatter_tag__)
        send_args.append(orig_communicator)
        send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
        root_stmts.append(send_call)
    root_part = c_ast.Compound(root_stmts)
    # 4. create pAst
    condition = c_ast.BinaryOp("==", rank_obj, orig_root_rank)
    if_else_tree = c_ast.If(condition, root_part, intermediate_parts[last_processed_rn])
    if not template_only and (not found_buffer and not return_array_decl):
        # TODO: put this on global level?
        past_stmts = []
        past_stmts.append(all_buffer_variable_decl)
        past_stmts.append(if_else_tree)
        pAST = c_ast.Compound(past_stmts)
    else:
        pAST = if_else_tree
    array_decl = None
    tcl_ret = None
    array_name = None
    if return_array_decl and not template_only:
        array_decl = all_buffer_variable_decl
        tcl_ret = tcl_list_return
        array_name = all_buffer_variable_name
    return pAST, available_buffer_list, array_decl, tcl_ret, array_name


def optimized_gather_replacement(gather_call, replicator_nodes, rank_obj, available_buffer_list, template_only):
    """

    :param gather_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    orig_datatype = gather_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(orig_datatype.value))
    orig_chunk_size = gather_call.args.exprs[1]
    real_chunk_size, err = _extract_int_size_of_malloc(orig_chunk_size)
    if err is None:
        orig_chunk_size = c_ast.Constant('int', str(real_chunk_size))
    orig_src_buffer = gather_call.args.exprs[0]
    orig_root_rank = gather_call.args.exprs[6]
    orig_communicator = gather_call.args.exprs[7]
    orig_dst_buffer = gather_call.args.exprs[3]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # for first replicator node, create buffer
    # 1. global inits
    return_array_decl = False
    tcl_list_return = []
    all_inter_buffer_size = c_ast.BinaryOp('*', c_ast.Constant('int', str(replicator_nodes["group_size"])), orig_chunk_size)
    real_all_inter_buffer_size, err = _extract_int_size_of_malloc(all_inter_buffer_size)
    if err is None:
        all_inter_buffer_size = c_ast.Constant('int', str(real_all_inter_buffer_size))
    all_inter_buffer_signature = disassemble_chunk_sizes(all_inter_buffer_size)
    found_buffer = False
    all_buffer_variable_id = None
    all_buffer_variable_decl = None
    if available_buffer_list is not None and len(available_buffer_list['list']) > 0:
        position = 0
        for l in available_buffer_list['list']:
            if l == all_inter_buffer_signature:
                # found one
                all_buffer_variable_decl = available_buffer_list['decls'][position]
                all_buffer_variable_id = available_buffer_list['ids'][position]
                found_buffer = True
                break
            else:
                position += 1
    if not found_buffer:
        all_buffer_variable_name = "{}_{}_4".format(__buffer_variable__name__, get_random_name_extension())
        all_buffer_variable_decl = c_ast.Decl(name=all_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                              type=c_ast.ArrayDecl(type=c_ast.TypeDecl(all_buffer_variable_name, [],
                                                                                       c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                                   #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                                   dim=all_inter_buffer_size
                                                                   ),
                                              init=None,
                                              bitsize=None)
        all_buffer_variable_id = c_ast.ID(all_buffer_variable_name)
        buffer_real_size, err = _extract_int_size_of_malloc(all_inter_buffer_size)
        tcl_directive = __array_map_directive_string__.format(__default_directive_location__, all_buffer_variable_name)
        if available_buffer_list is not None:
            available_buffer_list['list'].append(all_inter_buffer_signature)
            available_buffer_list['decls'].append(all_buffer_variable_decl)
            available_buffer_list['ids'].append(all_buffer_variable_id)
            # if err is none, there might be some variable in thre
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                available_buffer_list['tcls'].append(tcl_directive)
                return_array_decl = True
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
        else:
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                return_array_decl = True
                tcl_list_return.append(tcl_directive)
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
    # 2. take care of replicator nodes
    intermediate_parts = {}
    intermediate_part_cnts = replicator_nodes['cnt']
    rns = replicator_nodes["replicators"]
    reversed_rns = list(reversed(rns))
    last_processed_rn = 0
    for rr in reversed_rns:
        group_rcv_nodes = replicator_nodes[rr]
        size_of_this_group = len(group_rcv_nodes)
        inter_then_part_stmts = []
        if template_only:
            inter_then_part_stmts.append(all_buffer_variable_decl)
        # 1. collect from groups
        recv_cnt = 1  # start with 1, 0 is the node itself
        for rcvi in group_rcv_nodes:
            recv_args = []
            recv_args.append(c_ast.BinaryOp('+', all_buffer_variable_id,
                                            c_ast.BinaryOp('*', c_ast.Constant('int', str(recv_cnt)),
                                                           orig_chunk_size)))
            recv_cnt += 1
            recv_args.append(orig_chunk_size)
            recv_args.append(orig_datatype)
            recv_args.append(c_ast.Constant('int', str(rcvi)))
            recv_args.append(__scatter_tag__)
            recv_args.append(orig_communicator)
            recv_args.append(__status_ignore__)
            recv_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args))
            inter_then_part_stmts.append(recv_call)
        # 2. memcpy own part
        memcpy_args = []
        memcpy_args.append(all_buffer_variable_id)
        memcpy_args.append(orig_src_buffer)
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args))
        inter_then_part_stmts.append(memcpy)
        # 3. send large chunk
        send_call_args = []
        send_call_args.append(all_buffer_variable_id)
        send_call_args.append(c_ast.BinaryOp('*', c_ast.Constant('int', str(size_of_this_group+1)), orig_chunk_size))
        send_call_args.append(orig_datatype)
        send_call_args.append(orig_root_rank)
        send_call_args.append(__scatter_tag__)
        send_call_args.append(orig_communicator)
        inter_then_part_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_call_args)))
        inter_then_part = c_ast.Compound(inter_then_part_stmts)
        # 4. send in groups
        inter_else_part_plain_stmts = []
        send2_args = []
        send2_args.append(orig_src_buffer)
        send2_args.append(orig_chunk_size)
        send2_args.append(orig_datatype)
        send2_args.append(c_ast.Constant('int', str(rr)))
        send2_args.append(__scatter_tag__)
        send2_args.append(orig_communicator)
        inter_else_part_plain_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send2_args)))
        inter_else_part_plain = c_ast.Compound(inter_else_part_plain_stmts)
        # is also if again
        # if -> then receive -> else, next group
        # if last group, no if/else
        if rr == rns[-1]:
            inter_else_part = inter_else_part_plain
        else:
            rcv_compares = []
            for rcvn in group_rcv_nodes:
                new_compare = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rcvn)))
                rcv_compares.append(new_compare)
            inter_else_condition = rcv_compares[0]
            if len(rcv_compares) > 1:
                for i in range(1, len(rcv_compares)):
                    inter_else_condition = c_ast.BinaryOp("||", inter_else_condition, rcv_compares[i])
            inter_else_then_part = inter_else_part_plain
            inter_else_else_part = intermediate_parts[last_processed_rn]
            inter_else_part = c_ast.If(inter_else_condition, inter_else_then_part, inter_else_else_part)
        # 3. assemble
        inter_condition = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rr)))
        inter_part = c_ast.If(inter_condition, inter_then_part, inter_else_part)
        intermediate_parts[rr] = inter_part
        last_processed_rn = rr
    # 3. take care of root node
    root_stmts = []
    src_dst_equal = False
    try:
        if gather_call.args.exprs[0].expr.name.name.name == gather_call.args.exprs[3].expr.name.name.name:
            src_dst_equal = True
    except:
        # so we are not sure...
        src_dst_equal = False
    if not src_dst_equal:
        memcpy_args2 = []
        memcpy_args2.append(orig_dst_buffer)
        memcpy_args2.append(orig_src_buffer)
        sizeof_args2 = []
        sizeof_args2.append(c_ast.Constant('string', datatype_string))
        memcpy_args2.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args2))))
        memcpy2 = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args2))
        root_stmts.append(memcpy2)
    recv2_cnt = 1  # start with 1, 0 is the node itself
    for rn in rns:
        group_rcv_nodes = replicator_nodes[rn]
        size_of_this_group = len(group_rcv_nodes)
        recv2_args = []
        recv2_args.append(c_ast.BinaryOp('+', orig_dst_buffer,
                                         c_ast.BinaryOp('*', c_ast.Constant('int', str(recv2_cnt)),
                                                        orig_chunk_size)))
        recv2_cnt += size_of_this_group + 1
        recv2_args.append(c_ast.BinaryOp('*', c_ast.Constant('int', str(size_of_this_group+1)), orig_chunk_size))
        recv2_args.append(orig_datatype)
        recv2_args.append(c_ast.Constant('int', str(rn)))
        recv2_args.append(__scatter_tag__)
        recv2_args.append(orig_communicator)
        recv2_args.append(__status_ignore__)
        recv2_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv2_args))
        root_stmts.append(recv2_call)
    root_part = c_ast.Compound(root_stmts)
    # 4. create pAst
    condition = c_ast.BinaryOp("==", rank_obj, orig_root_rank)
    if_else_tree = c_ast.If(condition, root_part, intermediate_parts[last_processed_rn])
    if not template_only and (not found_buffer and not return_array_decl):
        past_stmts = []
        past_stmts.append(all_buffer_variable_decl)
        past_stmts.append(if_else_tree)
        pAST = c_ast.Compound(past_stmts)
    else:
        pAST = if_else_tree
    array_decl = None
    tcl_ret = None
    array_name = None
    if return_array_decl and not template_only:
        array_decl = all_buffer_variable_decl
        tcl_ret = tcl_list_return
        array_name = all_buffer_variable_name
    return pAST, available_buffer_list, array_decl, tcl_ret, array_name


def optimized_bcast_replacement(bcast_call, replicator_nodes, rank_obj, available_buffer_list):
    """

    :param available_buffer_list: available internal optimization buffers (will be ignored here)
    :param bcast_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    orig_datatype = bcast_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(orig_datatype.value))
    orig_chunk_size = bcast_call.args.exprs[1]
    orig_buffer = bcast_call.args.exprs[0]
    orig_root_rank = bcast_call.args.exprs[3]
    orig_communicator = bcast_call.args.exprs[4]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # 1. take care of replicator nodes
    intermediate_parts = {}
    intermediate_part_cnts = replicator_nodes['cnt']
    rns = replicator_nodes["replicators"]
    reversed_rns = list(reversed(rns))
    last_processed_rn = 0
    for rr in reversed_rns:
        group_rcv_nodes = replicator_nodes[rr]
        size_of_this_group = len(group_rcv_nodes)
        # 1. receive large chunk
        inter_then_part_stmts = []
        # if rr == rns[0]:
        #     # first node, declare buffer
        #     inter_then_part_stmts.append(all_buffer_variable_decl)
        func_call_args = []
        func_call_args.append(orig_buffer)
        func_call_args.append(orig_chunk_size)
        func_call_args.append(orig_datatype)
        func_call_args.append(orig_root_rank)
        func_call_args.append(__bcast_tag__)
        func_call_args.append(orig_communicator)
        func_call_args.append(__status_ignore__)
        inter_then_part_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(func_call_args)))
        # 2. distribute to groups
        for rcvi in group_rcv_nodes:
            send_args = []
            send_args.append(orig_buffer)
            send_args.append(orig_chunk_size)
            send_args.append(orig_datatype)
            send_args.append(c_ast.Constant('int', str(rcvi)))
            send_args.append(__bcast_tag__)
            send_args.append(orig_communicator)
            send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
            inter_then_part_stmts.append(send_call)
        inter_then_part = c_ast.Compound(inter_then_part_stmts)
        # 3. receive in groups
        inter_else_part_plain_stmts = []
        recv_args = []
        recv_args.append(orig_buffer)
        recv_args.append(orig_chunk_size)
        recv_args.append(orig_datatype)
        recv_args.append(c_ast.Constant('int', str(rr)))
        recv_args.append(__bcast_tag__)
        recv_args.append(orig_communicator)
        recv_args.append(__status_ignore__)
        inter_else_part_plain_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args)))
        inter_else_part_plain = c_ast.Compound(inter_else_part_plain_stmts)
        # is also if again
        # if -> then receive -> else, next group
        # if last group, no if/else
        if rr == rns[-1]:
            inter_else_part = inter_else_part_plain
        else:
            rcv_compares = []
            for rcvn in group_rcv_nodes:
                new_compare = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rcvn)))
                rcv_compares.append(new_compare)
            inter_else_condition = rcv_compares[0]
            if len(rcv_compares) > 1:
                for i in range(1, len(rcv_compares)):
                    inter_else_condition = c_ast.BinaryOp("||", inter_else_condition, rcv_compares[i])
            inter_else_then_part = inter_else_part_plain
            inter_else_else_part = intermediate_parts[last_processed_rn]
            inter_else_part = c_ast.If(inter_else_condition, inter_else_then_part, inter_else_else_part)
        # 3. assemble
        inter_condition = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rr)))
        inter_part = c_ast.If(inter_condition, inter_then_part, inter_else_part)
        intermediate_parts[rr] = inter_part
        last_processed_rn = rr
    # 2. take care of root node
    root_stmts = []
    for rn in rns:
        group_rcv_nodes = replicator_nodes[rn]
        size_of_this_group = len(group_rcv_nodes)
        send_args = []
        #send_args.append(c_ast.BinaryOp('+', orig_src_buffer,
        #                                c_ast.BinaryOp('*', c_ast.Constant('int', str(send_cnt)),
        #                                               orig_chunk_size)))
        send_args.append(orig_buffer)
        send_args.append(orig_chunk_size)
        send_args.append(orig_datatype)
        send_args.append(c_ast.Constant('int', str(rn)))
        send_args.append(__bcast_tag__)
        send_args.append(orig_communicator)
        send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
        root_stmts.append(send_call)
    root_part = c_ast.Compound(root_stmts)
    # 4. create pAst
    condition = c_ast.BinaryOp("==", rank_obj, orig_root_rank)
    if_else_tree = c_ast.If(condition, root_part, intermediate_parts[last_processed_rn])
    pAST = if_else_tree
    return pAST, available_buffer_list


def optimized_reduce_replacement(reduce_call, replicator_nodes, rank_obj, available_buffer_list, template_only):
    """

    :param reduce_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    reduce_datatype = reduce_call.args.exprs[3]
    datatype_string = get_type_string_from_int(int(reduce_datatype.value))
    reduce_func_arg = reduce_call.args.exprs[4]
    reduce_func_name = None
    if type(reduce_func_arg) is c_ast.Constant:
        reduce_func_name = __mpi_op_defines__[reduce_func_arg.value]
    else:
        reduce_func_name = reduce_func_arg.name
    reduce_func = get_reduce_function_from_name_and_type(reduce_func_name, int(reduce_datatype.value))
    orig_chunk_size = reduce_call.args.exprs[2]
    real_chunk_size, err = _extract_int_size_of_malloc(orig_chunk_size)
    if err is None:
        orig_chunk_size = c_ast.Constant('int', str(real_chunk_size))
    root_rank = reduce_call.args.exprs[5]
    orig_communicator = reduce_call.args.exprs[6]
    target_buffer = reduce_call.args.exprs[1]
    source_buffer = reduce_call.args.exprs[0]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # for first replicator node, create buffer
    # 1. global inits
    return_array_decl = False
    tcl_list_return = []
    accum_inter_buffer_size = orig_chunk_size
    real_all_inter_buffer_size, err = _extract_int_size_of_malloc(accum_inter_buffer_size)
    if err is None:
        accum_inter_buffer_size = c_ast.Constant('int', str(real_all_inter_buffer_size))
    all_inter_buffer_signature = disassemble_chunk_sizes(accum_inter_buffer_size)
    found_buffer = False
    all_buffer_variable_id = None
    all_buffer_variable_decl = None
    if available_buffer_list is not None and len(available_buffer_list['list']) > 0:
        position = 0
        for l in available_buffer_list['list']:
            if l == all_inter_buffer_signature:
                # found one
                all_buffer_variable_decl = available_buffer_list['decls'][position]
                all_buffer_variable_id = available_buffer_list['ids'][position]
                found_buffer = True
                break
            else:
                position += 1
    if not found_buffer:
        all_buffer_variable_name = "{}_{}_8".format(__buffer_variable__name__, get_random_name_extension())
        all_buffer_variable_decl = c_ast.Decl(name=all_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                              type=c_ast.ArrayDecl(type=c_ast.TypeDecl(all_buffer_variable_name, [],
                                                                                       c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                                   #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                                   dim=accum_inter_buffer_size
                                                                   ),
                                              init=None,
                                              bitsize=None)
        all_buffer_variable_id = c_ast.ID(all_buffer_variable_name)
        buffer_real_size, err = _extract_int_size_of_malloc(accum_inter_buffer_size)
        tcl_directive = __array_map_directive_string__.format(__default_directive_location__, all_buffer_variable_name)
        if available_buffer_list is not None:
            available_buffer_list['list'].append(all_inter_buffer_signature)
            available_buffer_list['decls'].append(all_buffer_variable_decl)
            available_buffer_list['ids'].append(all_buffer_variable_id)
            # if err is none, there might be some variable in thre
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                available_buffer_list['tcls'].append(tcl_directive)
                return_array_decl = True
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
        else:
            if err is None and buffer_real_size > maximum_bram_buffer_size_bytes:
                return_array_decl = True
                tcl_list_return.append(tcl_directive)
                print("[INFO] tree operation buffer is to large, will be mapped to DRAM.")
    # 2. take care of replicator nodes
    # we need a new buffer as temporary accumulator
    replicator_accum_buffer_name = "{}_{}_82".format(__buffer_variable__name__, get_random_name_extension())
    replicator_accum_buffer_decl = c_ast.Decl(name=replicator_accum_buffer_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                              type=c_ast.ArrayDecl(type=c_ast.TypeDecl(replicator_accum_buffer_name, [],
                                                                                       c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                                   #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                                   dim=accum_inter_buffer_size # same size
                                                                   ),
                                              init=None,
                                              bitsize=None)
    replicator_accum_buffer_id = c_ast.ID(replicator_accum_buffer_name)
    tcl_directive = __array_map_directive_string__.format(__default_directive_location__, replicator_accum_buffer_name)
    intermediate_parts = {}
    intermediate_part_cnts = replicator_nodes['cnt']
    rns = replicator_nodes["replicators"]
    reversed_rns = list(reversed(rns))
    last_processed_rn = 0
    for rr in reversed_rns:
        group_rcv_nodes = replicator_nodes[rr]
        size_of_this_group = len(group_rcv_nodes)
        inter_then_part_stmts = []
        if template_only:
            inter_then_part_stmts.append(all_buffer_variable_decl)
        # 0. add buffer decl
        inter_then_part_stmts.append(replicator_accum_buffer_decl)
        # 1. accum own part
        memcpy_args0 = []
        memcpy_args0.append(replicator_accum_buffer_id)
        memcpy_args0.append(source_buffer)
        sizeof_args0 = []
        sizeof_args0.append(c_ast.Constant('string', datatype_string))
        memcpy_args0.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args0))))
        memcpy0 = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args0))
        inter_then_part_stmts.append(memcpy0)
        # 2. collect from groups
        for rcvi in group_rcv_nodes:
            recv_args = []
            recv_args.append(all_buffer_variable_id)
            recv_args.append(orig_chunk_size)
            recv_args.append(reduce_datatype)
            recv_args.append(c_ast.Constant('int', str(rcvi)))
            recv_args.append(__reduce_tag__)
            recv_args.append(orig_communicator)
            recv_args.append(__status_ignore__)
            recv_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args))
            inter_then_part_stmts.append(recv_call)
            red_args = []
            red_args.append(replicator_accum_buffer_id)
            red_args.append(all_buffer_variable_id)
            red_args.append(orig_chunk_size)
            red_func = c_ast.FuncCall(c_ast.ID(reduce_func), c_ast.ExprList(red_args))
            inter_then_part_stmts.append(red_func)
        # 3. forward accum data
        send_call_args = []
        send_call_args.append(replicator_accum_buffer_id)
        send_call_args.append(orig_chunk_size)
        send_call_args.append(reduce_datatype)
        send_call_args.append(root_rank)
        send_call_args.append(__reduce_tag__)
        send_call_args.append(orig_communicator)
        inter_then_part_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_call_args)))
        inter_then_part = c_ast.Compound(inter_then_part_stmts)
        # 4. send in groups
        inter_else_part_plain_stmts = []
        send2_args = []
        send2_args.append(source_buffer)
        send2_args.append(orig_chunk_size)
        send2_args.append(reduce_datatype)
        send2_args.append(c_ast.Constant('int', str(rr)))
        send2_args.append(__reduce_tag__)
        send2_args.append(orig_communicator)
        inter_else_part_plain_stmts.append(c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send2_args)))
        inter_else_part_plain = c_ast.Compound(inter_else_part_plain_stmts)
        # is also if again
        # if -> then receive -> else, next group
        # if last group, no if/else
        if rr == rns[-1]:
            inter_else_part = inter_else_part_plain
        else:
            rcv_compares = []
            for rcvn in group_rcv_nodes:
                new_compare = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rcvn)))
                rcv_compares.append(new_compare)
            inter_else_condition = rcv_compares[0]
            if len(rcv_compares) > 1:
                for i in range(1, len(rcv_compares)):
                    inter_else_condition = c_ast.BinaryOp("||", inter_else_condition, rcv_compares[i])
            inter_else_then_part = inter_else_part_plain
            inter_else_else_part = intermediate_parts[last_processed_rn]
            inter_else_part = c_ast.If(inter_else_condition, inter_else_then_part, inter_else_else_part)
        # 3. assemble
        inter_condition = c_ast.BinaryOp("==", rank_obj, c_ast.Constant('int', str(rr)))
        inter_part = c_ast.If(inter_condition, inter_then_part, inter_else_part)
        intermediate_parts[rr] = inter_part
        last_processed_rn = rr
    # 3. take care of root node
    root_stmts = []
    if template_only:
        root_stmts.append(all_buffer_variable_decl)
    memcpy_args2 = []
    memcpy_args2.append(target_buffer)
    memcpy_args2.append(source_buffer)
    sizeof_args = []
    sizeof_args.append(c_ast.Constant('string', datatype_string))
    memcpy_args2.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
    memcpy2 = c_ast.FuncCall(c_ast.ID('my_memcpy'), c_ast.ExprList(memcpy_args2))
    root_stmts.append(memcpy2)
    for rn in rns:
        recv2_args = []
        recv2_args.append(all_buffer_variable_id)
        recv2_args.append(orig_chunk_size)
        recv2_args.append(reduce_datatype)
        recv2_args.append(c_ast.Constant('int', str(rn)))
        recv2_args.append(__reduce_tag__)
        recv2_args.append(orig_communicator)
        recv2_args.append(__status_ignore__)
        recv2_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv2_args))
        root_stmts.append(recv2_call)
        red_args2 = []
        red_args2.append(target_buffer)
        red_args2.append(all_buffer_variable_id)
        red_args2.append(orig_chunk_size)
        red_func2 = c_ast.FuncCall(c_ast.ID(reduce_func), c_ast.ExprList(red_args2))
        root_stmts.append(red_func2)
    root_part = c_ast.Compound(root_stmts)
    # 4. create pAst
    condition = c_ast.BinaryOp("==", rank_obj, root_rank)
    if_else_tree = c_ast.If(condition, root_part, intermediate_parts[last_processed_rn])
    if not template_only and (not found_buffer and not return_array_decl):
        past_stmts = []
        past_stmts.append(all_buffer_variable_decl)
        past_stmts.append(if_else_tree)
        pAST = c_ast.Compound(past_stmts)
    else:
        pAST = if_else_tree
    array_decl = None
    tcl_ret = None
    array_name = None
    if return_array_decl and not template_only:
        array_decl = [all_buffer_variable_decl, replicator_accum_buffer_decl]
        tcl_ret = tcl_list_return
        array_name = [all_buffer_variable_name, replicator_accum_buffer_name]
    return pAST, available_buffer_list, array_decl, tcl_ret, array_name


def send_replacemet(send_call):
    print("ERROR: htis method is deprecated, do not use! (and no longer necessary)")
    exit(-1)
    if type(send_call.args.exprs[1]) == c_ast.Constant:
        orig_count = int(send_call.args.exprs[1].value)
        if orig_count <= __max_packet_length__:
            # nothing to do
            return send_call
    orig_count = send_call.args.exprs[1]
    loop_variable_name = "{}_{}_2".format(__loop_variable__name__, get_random_name_extension())
    new_start_variable_name = "{}_{}_2".format(__new_start_variable_name__, get_random_name_extension())
    new_count_variable_name = "{}_{}_2".format(__new_count_variable_name__, get_random_name_extension())
    send_datatype = send_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(send_datatype.value))
    # iterations = c_ast.BinaryOp("/", c_ast.BinaryOp("+", orig_count, c_ast.Constant('int', str(__max_packet_length__-1))),
    #                            c_ast.Constant('int', str(__max_packet_length__)))
    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    #for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), iterations)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), orig_count)
    # for_next = c_ast.UnaryOp('p++', c_ast.ID(loop_variable_name))
    for_next = c_ast.Assignment('+=', c_ast.ID(loop_variable_name),
                                #c_ast.BinaryOp('+', c_ast.ID(loop_variable_name),
                                c_ast.Constant('int', str(__max_packet_length__)))#)
    loop_stmts = []
    buffer_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                 type=c_ast.PtrDecl([],
                                                    c_ast.TypeDecl(new_start_variable_name, [], c_ast.IdentifierType([datatype_string]))),
                                 init=c_ast.BinaryOp("+", send_call.args.exprs[0],
                                                     #c_ast.BinaryOp("*",
                                                                    c_ast.ID(loop_variable_name),
                                                     #c_ast.Constant('int', str(__max_packet_length__)))
                                                     )
                                                     ,
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    count_variable = c_ast.Decl(name=new_count_variable_name, quals=[], storage=[], funcspec=[],
                                type=c_ast.TypeDecl(new_count_variable_name, [], c_ast.IdentifierType(['int'])),
                                init=c_ast.BinaryOp("-",orig_count,c_ast.ID(loop_variable_name)),
                                #c_ast.Constant('int', str(__max_packet_length__)),
                                bitsize=None)
    loop_stmts.append(count_variable)
    if_stmts = []
    #corr_stmt = c_ast.Assignment("=", c_ast.ID(new_count_variable_name),
    #                             c_ast.BinaryOp("-", orig_count,
    #                                            c_ast.BinaryOp("*", c_ast.BinaryOp("-", c_ast.ID(loop_variable_name), c_ast.Constant('int', '1'))
    #                                                           , c_ast.Constant('int', str(__max_packet_length__))
    #                                                           )
    #                                            )
    #                             )
    corr_stmt = c_ast.Assignment('=', c_ast.ID(new_count_variable_name),
                                 c_ast.Constant('int', str(__max_packet_length__)))
    if_stmts.append(corr_stmt)
    #corr_if = c_ast.If(c_ast.BinaryOp(">", c_ast.BinaryOp("+", c_ast.ID(new_count_variable_name),
    #                                                      c_ast.BinaryOp("*",
    #                                                                     c_ast.BinaryOp("-", c_ast.ID(loop_variable_name), c_ast.Constant('int', '1')),
    #                                                                     c_ast.Constant('int', str(__max_packet_length__))
    #                                                                     )
    #                                                      ),
    #                                  orig_count),
    #                   c_ast.Compound(if_stmts), None)
    corr_if = c_ast.If(c_ast.BinaryOp('>', c_ast.ID(new_count_variable_name), c_ast.Constant('int', str(__max_packet_length__))),
                       c_ast.Compound(if_stmts), None)
    loop_stmts.append(corr_if)
    send_args = []
    send_args.append(c_ast.ID(new_start_variable_name))
    send_args.append(c_ast.ID(new_count_variable_name))
    send_args.append(send_call.args.exprs[2])
    send_args.append(send_call.args.exprs[3])
    send_args.append(send_call.args.exprs[4])
    send_args.append(send_call.args.exprs[5])
    send_call = c_ast.FuncCall(c_ast.ID("MPI_Send"), c_ast.ExprList(send_args))
    loop_stmts.append(send_call)
    for_stmt = c_ast.Compound(loop_stmts)
    pAST = c_ast.For(loop_variable, for_condition, for_next, for_stmt)
    return pAST


def recv_replacement(recv_call):
    print("ERROR: htis method is deprecated, do not use! (and no longer necessary)")
    exit(-1)
    if type(recv_call.args.exprs[1]) == c_ast.Constant:
        orig_count = int(recv_call.args.exprs[1].value)
        if orig_count <= __max_packet_length__:
            # nothing to do
            return recv_call
    orig_count = recv_call.args.exprs[1]
    loop_variable_name = "{}_{}_3".format(__loop_variable__name__, get_random_name_extension())
    new_start_variable_name = "{}_{}_3".format(__new_start_variable_name__, get_random_name_extension())
    new_count_variable_name = "{}_{}_3".format(__new_count_variable_name__, get_random_name_extension())
    recv_datatype = recv_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(recv_datatype.value))
    loop_variable = c_ast.Decl(name=loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
    for_condition = c_ast.BinaryOp('<', c_ast.ID(loop_variable_name), orig_count)
    for_next = c_ast.Assignment('+=', c_ast.ID(loop_variable_name),
                                c_ast.Constant('int', str(__max_packet_length__)))#)
    loop_stmts = []
    buffer_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[],
                                 type=c_ast.PtrDecl([],
                                                    c_ast.TypeDecl(new_start_variable_name, [], c_ast.IdentifierType([datatype_string]))),
                                 init=c_ast.BinaryOp("+", recv_call.args.exprs[0],
                                                     #c_ast.BinaryOp("*",
                                                                    c_ast.ID(loop_variable_name),
                                                     #c_ast.Constant('int', str(__max_packet_length__)))
                                                     )
                                                     ,
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    count_variable = c_ast.Decl(name=new_count_variable_name, quals=[], storage=[], funcspec=[],
                                type=c_ast.TypeDecl(new_count_variable_name, [], c_ast.IdentifierType(['int'])),
                                init=c_ast.BinaryOp("-", orig_count, c_ast.ID(loop_variable_name)),
                                bitsize=None)
    loop_stmts.append(count_variable)
    if_stmts = []
    corr_stmt = c_ast.Assignment('=', c_ast.ID(new_count_variable_name),
                                 c_ast.Constant('int', str(__max_packet_length__)))
    if_stmts.append(corr_stmt)
    corr_if = c_ast.If(c_ast.BinaryOp('>', c_ast.ID(new_count_variable_name), c_ast.Constant('int', str(__max_packet_length__))),
                       c_ast.Compound(if_stmts), None)
    loop_stmts.append(corr_if)
    recv_args = []
    recv_args.append(c_ast.ID(new_start_variable_name))
    recv_args.append(c_ast.ID(new_count_variable_name))
    recv_args.append(recv_call.args.exprs[2])
    recv_args.append(recv_call.args.exprs[3])
    recv_args.append(recv_call.args.exprs[4])
    recv_args.append(recv_call.args.exprs[5])
    recv_args.append(recv_call.args.exprs[6])
    recv_call = c_ast.FuncCall(c_ast.ID("MPI_Recv"), c_ast.ExprList(recv_args))
    loop_stmts.append(recv_call)
    for_stmt = c_ast.Compound(loop_stmts)
    pAST = c_ast.For(loop_variable, for_condition, for_next, for_stmt)
    return pAST


def replace_clib_libraries(clib_call):
    f_name = clib_call.name.name
    new_name = ""
    found_name = False
    for f in clib_funcs_to_mark_for_delete:
        if f in f_name:
            new_name = delete_line_pattern
            found_name = True
            break
    if not found_name:
        new_name = "my_{}".format(f_name)
    new_name_id = c_ast.ID(new_name)
    new_call = c_ast.FuncCall(new_name_id, clib_call.args)
    return new_call


def _extract_int_size_of_malloc(malloc_args):
    if type(malloc_args) == c_ast.Constant:
        return int(malloc_args.value), None
    if type(malloc_args) == c_ast.BinaryOp:
        right_value, err1 = _extract_int_size_of_malloc(malloc_args.right)
        left_value, err2 = _extract_int_size_of_malloc(malloc_args.left)
        cmd = "{} {} {}".format(left_value, malloc_args.op, right_value)
        result_value = eval(cmd)
        err = None
        if err1 is not None:
            err = err1
        elif err2 is not None:
            err = err2
        return result_value, err
    elif type(malloc_args) == c_ast.UnaryOp and malloc_args.op == 'sizeof':
        return 1, None  # for array calculation, sizeof is always 1, no matter if * or +
    else:
        # print("[WARNING] unable to determine right malloc array size")
        return 1, "ERROR"


def get_nop_decl():
    nop_name = "{}_{}_X".format(delete_line_pattern, get_random_name_extension())
    nop_op = c_ast.Decl(name=nop_name, quals=[], storage=[],
                        funcspec=[], type=c_ast.TypeDecl(declname=nop_name, quals=[], type=c_ast.IdentifierType(['char'])),
                        init=None, bitsize=None)
    return nop_op


def malloc_replacement(malloc_call, malloc_stmt):
    # TODO: read current func_name from object
    # the placement is maybe not always 'app_main'
    array_size_value, err = _extract_int_size_of_malloc(malloc_call.args.exprs[0])
    array_size = c_ast.Constant(type='int', value=str(array_size_value))
    assert type(malloc_stmt) == c_ast.Assignment
    assert type(malloc_stmt.lvalue) == c_ast.ID or type(malloc_stmt.lvalue) == c_ast.Decl
    array_name = malloc_stmt.lvalue.name
    assert type(malloc_stmt.rvalue) == c_ast.Cast
    array_prt_type = malloc_stmt.rvalue.to_type.type
    array_raw_type = malloc_stmt.rvalue.to_type.type.type
    array_type_decl = c_ast.ArrayDecl(type=c_ast.TypeDecl(declname=array_name, type=array_raw_type, quals=[]),
                                      dim_quals=[], dim=array_size)
    array_decl = c_ast.Decl(name=array_name, quals=[], storage=[], funcspec=[],
                                          type=array_type_decl,
                                          init=None,
                                          bitsize=None)
    # pAST = array_decl
    tcl_directives = __array_map_directive_string__.format(__default_directive_location__, array_name)
    decl_to_search = array_name
    if type(malloc_stmt.lvalue) == c_ast.Decl:
        decl_to_search = None
    nop_op = get_nop_decl()
    pAST = nop_op
    return pAST, tcl_directives, decl_to_search, nop_op, array_decl, array_name


def __find_subscript_factor__(buffer_name, item_list, loop_var_to_replace):
    obj_to_visit = []
    for e in item_list:
        for c in e:
            obj_to_visit.append(c)
            for cc in c:
                obj_to_visit.append(cc)
                for ccc in cc:
                    obj_to_visit.append(ccc)
                    for cccc in ccc:
                        obj_to_visit.append(cccc)
            # we go for levels...
    cur_obj = obj_to_visit[0]
    del obj_to_visit[0]
    ret = c_ast.Constant('int', '1')
    while cur_obj is not None:
        name_found = False
        if hasattr(cur_obj, 'name'):
            if type(cur_obj.name) is c_ast.ID:
                if cur_obj.name.name == buffer_name:
                    name_found = True
            elif type(cur_obj.name) is string:
                if cur_obj.name == buffer_name:
                    name_found = True
            if name_found:
                if type(cur_obj) is not c_ast.ArrayRef:
                    return ret
                if type(cur_obj.subscript) is c_ast.ID:
                    if cur_obj.subscript.name == loop_var_to_replace:
                        return ret
                    else:
                        return cur_obj.subscript
                if type(cur_obj.subscript) is c_ast.BinaryOp:
                    cur_local_obj = cur_obj.subscript
                    new_op = c_ast.BinaryOp(op=cur_local_obj.op, left=None, right=None)
                    while True:
                        if type(cur_local_obj) is c_ast.BinaryOp:
                            if type(cur_local_obj.left) is c_ast.ID and cur_local_obj.left.name == loop_var_to_replace:
                                ret = cur_local_obj.right
                                return ret
                            if type(cur_local_obj.right) is c_ast.ID and cur_local_obj.right.name == loop_var_to_replace:
                                ret = cur_local_obj.left
                                return ret
                            if type(cur_local_obj.left) is c_ast.Constant:
                                new_op.left = cur_local_obj.left
                                cur_local_obj = cur_local_obj.right
                                continue
                            if type(cur_local_obj.right) is c_ast.Constant:
                                new_op.right = cur_local_obj.right
                                cur_local_obj = cur_local_obj.left
                                continue
                        elif type(cur_local_obj) is c_ast.ID:
                            if cur_local_obj.name == loop_var_to_replace:
                                return ret
                        else:
                            return ret
        if not name_found:
            if len(obj_to_visit) > 0:
                cur_obj = obj_to_visit[0]
                del obj_to_visit[0]
            else:
                return ret


def loop_optimization_replacement(inner_loop_entry, dram_buffer_names, dram_buffer_names_replace, found_array_dims):
    loop_call = inner_loop_entry['loop']
    new_outer_loop_stmts = []
    outer_loop_variable_name = "{}_{}_10".format(__loop_variable__name__, get_random_name_extension())
    outer_loop_variable_decl = c_ast.Decl(name=outer_loop_variable_name, quals=[], storage=[], funcspec=[],
                               type=c_ast.TypeDecl(outer_loop_variable_name, [], c_ast.IdentifierType(['int'])),
                               init=c_ast.Constant('int', '0'), bitsize=None)
                                # TODO: cover other init values?
    outer_loop_variable_id = c_ast.ID(name=outer_loop_variable_name)
    new_outer_loop_init = c_ast.DeclList([outer_loop_variable_decl])
    # TODO: what if condition is reversed?
    assert type(loop_call.cond) is c_ast.BinaryOp
    assert type(loop_call.cond.left) is c_ast.ID
    new_outer_loop_cond = c_ast.BinaryOp(loop_call.cond.op, outer_loop_variable_id, loop_call.cond.right)
    new_outer_loop_next = c_ast.Assignment('+=', outer_loop_variable_id, __loop_split_size_constant__)
    old_loop_var_name = loop_call.cond.left.name
    # take care of buffers
    new_buffer_decls = []
    used_buffer_decls_names = []
    buffer_id_read_pairs = []
    buffer_id_write_pairs = []
    for b in inner_loop_entry['buffers']:
        ni = dram_buffer_names.index(b['name'])
        nn = dram_buffer_names_replace[ni]
        b_decl = c_ast.Decl(name=nn, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                              type=c_ast.ArrayDecl(type=c_ast.TypeDecl(nn, [],
                                                                                       c_ast.IdentifierType(['int'])), dim_quals=[],
                                                                   # TODO: support other types
                                                                   #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                                   dim=__loop_split_size_constant__
                                                                   ),
                                              init=None,
                                              bitsize=None)
        new_buffer_decls.append(b_decl)
        used_buffer_decls_names.append(nn)
        factor = __find_subscript_factor__(b['name'], loop_call.stmt.block_items, old_loop_var_name)
        # TODO: support other type of BinaryOp
        subscript = c_ast.BinaryOp('*', outer_loop_variable_id, factor)
        # b_id = c_ast.ArrayRef(name=nn, subscript=subscript)
        b_id = c_ast.ID(nn)
        o_id = c_ast.UnaryOp('&', c_ast.ArrayRef(name=c_ast.ID(b['name']), subscript=subscript))
        # o_id = c_ast.ID(b['name'])
        if b['write']:
            np = {}
            np['target'] = o_id
            np['source'] = b_id
            buffer_id_write_pairs.append(np)
        if b['read']:
            np = {}
            np['target'] = b_id
            np['source'] = o_id
            buffer_id_read_pairs.append(np)
    # TODO: add different size handling
    # add memcpy for reading
    for p in buffer_id_read_pairs:
        memcpy_args = []
        memcpy_args.append(p['target'])
        memcpy_args.append(p['source'])
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', 'int'))
        # TODO: support other datatypes
        memcpy_args.append(c_ast.BinaryOp("*", __loop_split_size_constant__, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
        new_outer_loop_stmts.append(memcpy)
    # handle inner loop
    new_inner_loop_cond = c_ast.BinaryOp(loop_call.cond.op, loop_call.cond.left, __loop_split_size_constant__)
    new_inner_loop = c_ast.For(init=loop_call.init, cond=new_inner_loop_cond, next=loop_call.next, stmt=loop_call.stmt)
    new_outer_loop_stmts.append(new_inner_loop)
    # add memcpy for writing
    for p in buffer_id_write_pairs:
        memcpy_args = []
        memcpy_args.append(p['target'])
        memcpy_args.append(p['source'])
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', 'int'))
        # TODO: support other datatypes
        memcpy_args.append(c_ast.BinaryOp("*", __loop_split_size_constant__, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
        new_outer_loop_stmts.append(memcpy)
    # assemble new loop
    pAST = c_ast.For(init=new_outer_loop_init, cond=new_outer_loop_cond, next=new_outer_loop_next,
                     stmt=c_ast.Compound(new_outer_loop_stmts))
    return pAST, new_buffer_decls, used_buffer_decls_names

