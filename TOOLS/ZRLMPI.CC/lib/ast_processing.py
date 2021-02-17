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
import lib.mpi_variable_value_visitor as value_visitor
import lib.mpi_affected_statement_visitor as statement_visitor
import lib.mpi_replace_visitor as replace_visitor
import lib.mpi_constant_folding_visitor as constant_visitor
import lib.template_generator as template_generator
import lib.resource_checker as resource_checker
from lib.util import get_line_number_of_occurence


__fallback_max_buffer_size__ = 1500  # we have to find one

__size_of_c_type__ = {'char': 1, 'short': 2, 'int': 4, 'float': 4, 'double': 8}


def process_ast(c_ast_orig, cluster_description, cFp_description, hw_file_pre_parsing, target_file_name, template_only=False,
                replace_send_recv=False, optimize_scatter_gather=True, replicator_nodes=None, reuse_interim_buffers=False):
    # 0. process cluster description
    max_rank = 0
    total_size = 0
    for e in cluster_description['nodes']['cpu']:
        total_size += 1
        if e > max_rank:
            max_rank = e
    for e in cluster_description['nodes']['fpga']:
        total_size += 1
        if e > max_rank:
            max_rank = e
    if 'config' in cluster_description:
        if 'optimization' in cluster_description['config']:
            opt_val = cluster_description['config']['optimization']
            if opt_val == 0:
                optimize_scatter_gather = False
                reuse_interim_buffers = False
                print("disabled optimizations as configured")
            else:
                if opt_val >= 1:
                    optimize_scatter_gather = True
                    print("enabled Collective Tree Optimizations")
                if opt_val >= 3:
                    reuse_interim_buffers = True
                    print("enabled buffer reuse")
    # print("Maximum rank in cluster: {}\n".format(max_rank))
    all_ranks = list(range(0, max_rank+1))
    fpga_ranks = cluster_description['nodes']['fpga']
    cluster_size_constant = c_ast.Constant('int', str(total_size))
    # 1. find size names
    find_name_visitor = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor.visit(c_ast_orig)
    # rank_variable_names, rank_variable_obj = find_name_visitor.get_results_ranks()
    # print(rank_variable_names)
    size_variable_names, size_variable_obj = find_name_visitor.get_results_sizes()
    # 2. replace sizes
    sizes_to_replace = []
    for e in size_variable_obj:
        new_entry = {}
        new_entry['old'] = e
        if type(e.args.exprs[1]) is c_ast.UnaryOp:
            # pointer
            variable_obj = e.args.exprs[1].expr
        else:
            # no pointer
            variable_obj = e.args.exprs[1]
        new_constant_expression = c_ast.Assignment("=", variable_obj, cluster_size_constant)
        new_entry['new'] = new_constant_expression
        sizes_to_replace.append(new_entry)
    replace_stmt_visitor1 = replace_visitor.MpiStatementReplaceVisitor(sizes_to_replace)
    ast_m_1 = c_ast_orig
    replace_stmt_visitor1.visit(ast_m_1)
    # 3. constant folding
    # maybe another visitor that finds "variables that are used as constants"
    # only those variables should be part of constant folding at their time of assignment
    constant_folding_visitor1 = constant_visitor.MpiConstantFoldingVisitor()
    constant_folding_visitor1.visit(ast_m_1)
    constants_to_replace = constant_folding_visitor1.get_new_objects()
    replace_stmt_visitor1 = replace_visitor.MpiStatementReplaceVisitor(constants_to_replace)
    ast_m_2 = ast_m_1
    replace_stmt_visitor1.visit(ast_m_2)
    # 4. find rank names & find collective names
    find_name_visitor2 = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor2.visit(ast_m_2)
    rank_variable_names, rank_variable_obj = find_name_visitor2.get_results_ranks()
    scatter_calls_obj = find_name_visitor2.get_results_scatter()
    gather_calls_obj = find_name_visitor2.get_results_gather()
    bcast_calls_obj = find_name_visitor2.get_results_bcast()
    # 5. replace templates
    if len(rank_variable_names) > 1:
        print("WARNING: multiple rank variables detected, template generation may fail.")
    collectives_new_obj = []
    dont_optimize = True
    available_optimization_buffer_list = template_generator.create_available_buffer_structure()
    if replicator_nodes is not None:
        if replicator_nodes['msg'] == template_generator.__NO_OPTIMIZATION_MSG__:
            dont_optimize = True
        else:
            dont_optimize = False
    for e in scatter_calls_obj:
        new_entry = {}
        new_entry['old'] = e
        if (not optimize_scatter_gather) or dont_optimize:
            new_entry['new'] = template_generator.scatter_replacement(e, cluster_size_constant, c_ast.ID(rank_variable_names[0]))
        else:
            if not reuse_interim_buffers:
                available_optimization_buffer_list = None
            pAST, available_optimization_buffer_list = template_generator.optimized_scatter_replacement(e, replicator_nodes, c_ast.ID(rank_variable_names[0]), available_optimization_buffer_list)
            new_entry['new'] = pAST
        collectives_new_obj.append(new_entry)
    for e in gather_calls_obj:
        new_entry = {}
        new_entry['old'] = e
        if (not optimize_scatter_gather) or dont_optimize:
            new_entry['new'] = template_generator.gather_replacement(e, cluster_size_constant, c_ast.ID(rank_variable_names[0]))
        else:
            if not reuse_interim_buffers:
                available_optimization_buffer_list = None
            pAST, available_optimization_buffer_list = template_generator.optimized_gather_replacement(e, replicator_nodes, c_ast.ID(rank_variable_names[0]), available_optimization_buffer_list)
            new_entry['new'] = pAST
        collectives_new_obj.append(new_entry)
    for e in bcast_calls_obj:
        new_entry = {}
        new_entry['old'] = e
        if (not optimize_scatter_gather) or dont_optimize:
            new_entry['new'] = template_generator.bcast_replacement(e, cluster_size_constant, c_ast.ID(rank_variable_names[0]))
        else:
            if not reuse_interim_buffers:
                available_optimization_buffer_list = None
            new_entry['new'] = template_generator.bcast_replacement(e, cluster_size_constant, c_ast.ID(rank_variable_names[0]))
            #pAST, available_optimization_buffer_list = template_generator.optimized_gather_replacement(e, replicator_nodes, c_ast.ID(rank_variable_names[0]), available_optimization_buffer_list)
            #new_entry['new'] = pAST
        collectives_new_obj.append(new_entry)
    # replace send and recv if necessary
    if replace_send_recv:
        send_calls_obj = find_name_visitor.get_results_send()
        recv_calls_obj = find_name_visitor.get_results_recv()
        for e in send_calls_obj:
            new_entry = {}
            new_entry['old'] = e
            new_entry['new'] = template_generator.send_replacemet(e)
            collectives_new_obj.append(new_entry)
        for e in recv_calls_obj:
            new_entry = {}
            new_entry['old'] = e
            new_entry['new'] = template_generator.recv_replacement(e)
            collectives_new_obj.append(new_entry)
    ast_m_3 = ast_m_2
    replace_stmt_visitor0 = replace_visitor.MpiStatementReplaceVisitor(collectives_new_obj)
    replace_stmt_visitor0.visit(ast_m_3)
    # replace send and recv if necessary in generated templates
    if replace_send_recv:
        find_name_visitor2 = name_visitor.MpiSignatureNameSearcher()
        find_name_visitor2.visit(ast_m_3)
        send_calls_obj = find_name_visitor2.get_results_send()
        recv_calls_obj = find_name_visitor2.get_results_recv()
        send_recv_new_obj = []
        for e in send_calls_obj:
            new_entry = {}
            new_entry['old'] = e
            new_entry['new'] = template_generator.send_replacemet(e)
            send_recv_new_obj.append(new_entry)
        for e in recv_calls_obj:
            new_entry = {}
            new_entry['old'] = e
            new_entry['new'] = template_generator.recv_replacement(e)
            send_recv_new_obj.append(new_entry)
        c_ast_tmpl = ast_m_3
        replace_stmt_visitor3 = replace_visitor.MpiStatementReplaceVisitor(send_recv_new_obj)
        replace_stmt_visitor3.visit(c_ast_tmpl)
    else:
        c_ast_tmpl = ast_m_3
    # 6. another constant folding
    # TODO: another visitor that finds "variables that are used as constants"
    # only those variables should be part of constant folding at their time of assignment
    constant_folding_visitor2 = constant_visitor.MpiConstantFoldingVisitor()
    constant_folding_visitor2.visit(c_ast_tmpl)
    constants_to_replace2 = constant_folding_visitor2.get_new_objects()
    replace_stmt_visitor4 = replace_visitor.MpiStatementReplaceVisitor(constants_to_replace2)
    c_ast_tmpl_c = c_ast_tmpl
    replace_stmt_visitor4.visit(c_ast_tmpl_c)

    if template_only:
        # dump to target file
        generator2 = c_generator.CGenerator()
        generated_c = str(generator2.visit(c_ast_tmpl_c))

        line_number = get_line_number_of_occurence('int.*main\(', hw_file_pre_parsing)
        head = ""
        with open(hw_file_pre_parsing, 'r') as in_file:
            head = [next(in_file) for x in range(line_number - 1)]
        head_str = ""
        for e in head:
            head_str += str(e)
        concatenated_file = head_str + "\n" + generated_c
        # print("Writing new c code to file {}.".format(target_file_name))
        with open(target_file_name, 'w+') as target_file:
            target_file.write(concatenated_file)
        return 0

    # 7. search for rank names in new ast
    find_name_visitor3 = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor3.visit(c_ast_tmpl_c)
    rank_variable_names, rank_variable_obj = find_name_visitor3.get_results_ranks()
    # print(rank_variable_names)
    # size_variable_names, size_variable_obj = find_name_visitor3.get_results_sizes()

    # 8. detect FPGA parts based on rank (from cluster description)
    get_value_visitor = value_visitor.MpiVariableValueSearcher([], rank_variable_names)
    get_value_visitor.visit(c_ast_tmpl_c)
    found_compares = get_value_visitor.get_results_compares()
    print("Found {} compares of the rank variable.".format(len(found_compares)))
    # print(found_compares)
    rank_compare_results = []
    compares_invariant_for_fpgas = []
    for c in found_compares:
        fpga_rank_results = []
        if type(c['other']) != c_ast.Constant or 'c_value' not in c.keys():
            print("[NOT YET IMPLEMENTED] found non constant comparision of a rank, skipping this comparision.")
            continue
        for f in fpga_ranks:
            compare_str = ""
            if c['position'] == "left":
                compare_str = "{} {} {}".format(f, c['op'], c['c_value'])
            else:
                compare_str = "{} {} {}".format(c['c_value'], c['op'], f)
            # this is secure, because we checked in the visitor, that c.op could only be valid c compares...
            result = eval(compare_str)
            fpga_rank_results.append(result)
        new_result = {}
        new_result['operator_object'] = c['operator_object']
        new_result['fpga_results'] = fpga_rank_results
        unique_values = set(fpga_rank_results)
        differences_in_results = True
        if len(unique_values) == 1:
            differences_in_results = False
        new_result['differences_in_results'] = differences_in_results
        rank_compare_results.append(new_result)
        if not differences_in_results:
            # we only update the AST with invariant decisions, further optimization possible
            new_result['fpga_decision_value'] = str(list(unique_values)[0])
            compares_invariant_for_fpgas.append(new_result)

    # 9. modify AST with constant values (i.e. copy into new)
    if len(compares_invariant_for_fpgas) > 0:
        find_affected_node_visitor = statement_visitor.MpiAffectedStatementSearcher(compares_invariant_for_fpgas)
        find_affected_node_visitor.visit(c_ast_tmpl_c)
        affected_nodes = find_affected_node_visitor.get_found_statements()
        new_ast = c_ast_tmpl_c
        replace_stmt_visitor = replace_visitor.MpiStatementReplaceVisitor(affected_nodes)
        replace_stmt_visitor.visit(new_ast)
    else:
        print("No invariant rank statement for FPGAs found.")
        new_ast = c_ast_tmpl_c

    # TODO: detect unused variables?
    # 10. determine buffer size (and return them) AFTER the AST has been modified
    # TODO: also replace buffers of void type with dynamic type?
    find_name_visitor2 = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor2.visit(new_ast)
    buffer_variable_names, buffer_variable_obj = find_name_visitor2.get_results_buffers()
    # print(buffer_variable_names)
    get_value_visitor2 = value_visitor.MpiVariableValueSearcher(buffer_variable_names, [])
    get_value_visitor2.visit(new_ast)
    found_buffer_dims = get_value_visitor2.get_results_buffers()
    # print(found_buffer_dims)
    list_of_dims = []
    for e in found_buffer_dims.keys():
        try:
            new_dim = int(found_buffer_dims[e]['calculated_value'])*int(__size_of_c_type__[found_buffer_dims[e]['found_type']])
        except:
            new_dim = 0
        list_of_dims.append(new_dim)
    max_dimension_bytes = 0
    if len(list_of_dims) == 0:
        print("[WARNING] Did not found a maximum buffer size, this could lead to a failing HLS synthesis")
        max_dimension_bytes = __fallback_max_buffer_size__
    else:
        max_dimension_bytes = max(list_of_dims)
    print("Found max MPI buffer size: {} bytes".format(max_dimension_bytes))

    # 7. generate C code again
    # 8. append original header lines and save in file
    generator = c_generator.CGenerator()
    generated_c = str(generator.visit(new_ast))

    line_number = get_line_number_of_occurence('int.*main\(', hw_file_pre_parsing)
    head = ""
    with open(hw_file_pre_parsing, 'r') as in_file:
         head = [next(in_file) for x in range(line_number - 1)]

    head_str = ""
    for e in head:
        head_str += str(e)

    concatenated_file = head_str + "\n" + generated_c

    # print("Writing new c code to file {}.".format(target_file_name))

    with open(target_file_name, 'w+') as target_file:
        target_file.write(concatenated_file)

    # 9. check resource usages
    ignore_ret = resource_checker.check_resources(cFp_description, max_dimension_bytes)

    return max_dimension_bytes    # ZRLMPI_MAX_DETECTED_BUFFER_SIZE_bytes

