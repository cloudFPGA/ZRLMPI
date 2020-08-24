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
__new_start_variable_name__ = "new_start"
__new_count_variable_name__ = "new_count"
__random_name_suffix_length__ = 5

# __max_packet_length__ = "(1024/sizeof({})"
__max_packet_length__ = 256
__packet_length_margin__ = 64


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


def optimized_scatter_replacement(scatter_call, replicator_nodes, cluster_size, target_node):
    """

    :param scatter_call: original method call
    :param replicator_nodes: the node that should be replicators in the tree
    :param cluster_size: the cluster size
    :param target_node: the target node for generation
    :return:
    """
    return None


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


