#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
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

__scatter_tag__ = c_ast.Constant("int", "0")  # or smth else?
__gather_tag__ = c_ast.Constant("int", "0")  # or smth else?
__status_ignore__ = c_ast.Constant("int", "0")  # from zrlmpi_common.hpp
__loop_variable__name__ = "template_variable"
__buffer_variable__name__ = "template_buffer"
__new_start_variable_name__ = "new_start"
__new_count_variable_name__ = "new_count"
__random_name_suffix_length__ = 5

# __max_packet_length__ = "(1024/sizeof({})"
# __max_packet_length__ = 256
__max_packet_length__ = 352
__packet_length_margin__ = 64

__NO_OPTIMIZATION_MSG__ = "NO-Optimization-Possible"


def get_type_string_from_int(datatype_int):
    if datatype_int == 0:
        return "int"
    elif datatype_int == 1:
        return "float"
    else:
        return "void"


def get_random_name_extension():
    alphabet = string.ascii_lowercase + string.ascii_uppercase
    ret = ''.join(random.choice(alphabet) for i in range(__random_name_suffix_length__))
    return ret


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
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
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
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
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


def optimized_scatter_replacement(scatter_call, replicator_nodes, rank_obj):
    """

    :param scatter_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    orig_datatype = scatter_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(orig_datatype.value))
    orig_chunk_size = scatter_call.args.exprs[1]
    orig_src_buffer = scatter_call.args.exprs[0]
    orig_root_rank = scatter_call.args.exprs[6]
    orig_communicator = scatter_call.args.exprs[7]
    orig_local_buffer = scatter_call.args.exprs[3]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # for first replicator node, create buffer
    # 1. global inits
    all_inter_buffer_size = c_ast.BinaryOp('*', c_ast.Constant('int', str(replicator_nodes["group_size"])), orig_chunk_size)
    all_buffer_variable_name = "{}_{}_4".format(__buffer_variable__name__, get_random_name_extension())
    all_buffer_variable_decl = c_ast.Decl(name=all_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                 type=c_ast.ArrayDecl(type=c_ast.TypeDecl(all_buffer_variable_name, [],
                                                      c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                      #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                      dim=all_inter_buffer_size
                                                      ),
                                 init=None,
                                 bitsize=None)
    all_buffer_varibale_id = c_ast.ID(all_buffer_variable_name)
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
        # if rr == rns[0]:
        #     # first node, declare buffer
        #     inter_then_part_stmts.append(all_buffer_variable_decl)
        func_call_args = []
        func_call_args.append(all_buffer_varibale_id)
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
        memcpy_args.append(all_buffer_varibale_id)
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
        inter_then_part_stmts.append(memcpy)
        # 3. distribute to groups
        send_cnt = 1  # start with 1, 0 is the node itself
        for rcvi in group_rcv_nodes:
            send_args = []
            send_args.append(c_ast.BinaryOp('+', all_buffer_varibale_id,
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
        memcpy_args2.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy2 = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args2))
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
    past_stmts = []
    past_stmts.append(all_buffer_variable_decl)
    past_stmts.append(if_else_tree)
    pAST = c_ast.Compound(past_stmts)
    return pAST


def optimized_gather_replacement(gather_call, replicator_nodes, rank_obj):
    """

    :param gather_call: original method call
    :param replicator_nodes: the set that describes the tree structure
    :param rank_obj: the target node for generation
    :return:
    """
    orig_datatype = gather_call.args.exprs[2]
    datatype_string = get_type_string_from_int(int(orig_datatype.value))
    orig_chunk_size = gather_call.args.exprs[1]
    orig_src_buffer = gather_call.args.exprs[0]
    orig_root_rank = gather_call.args.exprs[6]
    orig_communicator = gather_call.args.exprs[7]
    orig_dst_buffer = gather_call.args.exprs[3]
    # structure is if -then- else -else -else
    # then: root, else: for replicator nodes,
    # for first replicator node, create buffer
    # 1. global inits
    all_inter_buffer_size = c_ast.BinaryOp('*', c_ast.Constant('int', str(replicator_nodes["group_size"])), orig_chunk_size)
    all_buffer_variable_name = "{}_{}_5".format(__buffer_variable__name__, get_random_name_extension())
    # TODO: optimize: use for all optimizations same buffer?
    all_buffer_variable_decl = c_ast.Decl(name=all_buffer_variable_name, quals=[], storage=[], funcspec=[], # type="{}*".format(datatype_string),
                                          type=c_ast.ArrayDecl(type=c_ast.TypeDecl(all_buffer_variable_name, [],
                                                                                   c_ast.IdentifierType([datatype_string])), dim_quals=[],
                                                               #dim=c_ast.Constant('int', str(all_inter_buffer_size))
                                                               dim=all_inter_buffer_size
                                                               ),
                                          init=None,
                                          bitsize=None)
    all_buffer_varibale_id = c_ast.ID(all_buffer_variable_name)
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
        # 1. collect from groups
        recv_cnt = 1  # start with 1, 0 is the node itself
        for rcvi in group_rcv_nodes:
            recv_args = []
            recv_args.append(c_ast.BinaryOp('+', all_buffer_varibale_id,
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
        memcpy_args.append(all_buffer_varibale_id)
        memcpy_args.append(orig_src_buffer)
        sizeof_args = []
        sizeof_args.append(c_ast.Constant('string', datatype_string))
        memcpy_args.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args))
        inter_then_part_stmts.append(memcpy)
        # 3. receive large chung
        send_call_args = []
        send_call_args.append(all_buffer_varibale_id)
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
        memcpy_args2.append(c_ast.BinaryOp("*", orig_chunk_size, c_ast.FuncCall(c_ast.ID('sizeof'), c_ast.ExprList(sizeof_args))))
        memcpy2 = c_ast.FuncCall(c_ast.ID('memcpy'), c_ast.ExprList(memcpy_args2))
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
    past_stmts = []
    past_stmts.append(all_buffer_variable_decl)
    past_stmts.append(if_else_tree)
    pAST = c_ast.Compound(past_stmts)
    return pAST


def send_replacemet(send_call):
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
                                                     #c_ast.BinaryOp("*", c_ast.ID(loop_variable_name),
                                                     c_ast.Constant('int', str(__max_packet_length__)))
                                                     #)
                                                     ,
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    count_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[],
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
                                                     c_ast.Constant('int', str(__max_packet_length__))),
                                 bitsize=None)
    loop_stmts.append(buffer_variable)
    count_variable = c_ast.Decl(name=new_start_variable_name, quals=[], storage=[], funcspec=[],
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


