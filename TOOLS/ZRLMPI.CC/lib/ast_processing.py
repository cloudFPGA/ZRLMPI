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

from lib.util import get_line_number_of_occurence


__fallback_max_buffer_size__ = 1500  # we have to find one

__size_of_c_type__ = {'char': 1, 'short': 2, 'int': 4, 'float': 4, 'double': 8}


def process_ast(c_ast_orig, cluster_description, hw_file_pre_parsing, target_file_name):
    # 1. find buffer names
    # 2. find rank names
    find_name_visitor = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor.visit(c_ast_orig)
    rank_variable_names, rank_variable_obj = find_name_visitor.get_results_ranks()
    # print(rank_variable_names)

    # 3. detect FPGA parts based on rank (from cluster description)
    get_value_visitor = value_visitor.MpiVariableValueSearcher([], rank_variable_names)
    get_value_visitor.visit(c_ast_orig)
    found_compares = get_value_visitor.get_results_compares()
    print("Found {} compares of the rank variable.".format(len(found_compares)))
    # print(found_compares)
    max_rank = 0
    for e in cluster_description['nodes']['cpu']:
        if e > max_rank:
            max_rank = e
    for e in cluster_description['nodes']['fpga']:
        if e > max_rank:
            max_rank = e
    # print("Maximum rank in cluster: {}\n".format(max_rank))
    all_ranks = list(range(0, max_rank+1))
    fpga_ranks = cluster_description['nodes']['fpga']
    rank_compare_results = []
    compares_invariant_for_fpgas = []
    for c in found_compares:
        fpga_rank_results = []
        if type(c['other']) != c_ast.Constant or 'c_value' not in c.keys():
            print("[NOT YET IMPLEMENTED] found non constant comparsion of a rank")
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
            new_result['fpga_decision_value'] = str(list(unique_values)[0])
            compares_invariant_for_fpgas.append(new_result)

    # 4. modify AST with constant values (i.e. copy into new)
    if len(compares_invariant_for_fpgas) > 0:
        find_affected_node_visitor = statement_visitor.MpiAffectedStatementSearcher(compares_invariant_for_fpgas)
        find_affected_node_visitor.visit(c_ast_orig)
        affected_nodes = find_affected_node_visitor.get_found_statements()
        new_ast = c_ast_orig
        replace_stmt_visitor = replace_visitor.MpiStatementReplaceVisitor(affected_nodes)
        replace_stmt_visitor.visit(new_ast)
    else:
        print("No invariant rank statement for FPGAs found.")
        new_ast = c_ast_orig

    # 5. determine buffer size (and return them) AFTER the AST has been modified
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
        list_of_dims.append(int(found_buffer_dims[e]['calculated_value'])*int(__size_of_c_type__[found_buffer_dims[e]['found_type']]))
    max_dimension_bytes = 0
    if len(list_of_dims) == 0:
        print("[WARNING] Did not found a maximum buffer size, this could lead to a failing HLS synthesis")
        max_dimension_bytes = __fallback_max_buffer_size__
    else:
        max_dimension_bytes = max(list_of_dims)
    print("Found max MPI buffer size: {} bytes".format(max_dimension_bytes))

    # 5. generate C code again
    # 6. append original header lines and save in file
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

    return max_dimension_bytes    # ZRLMPI_MAX_DETECTED_BUFFER_SIZE_bytes

