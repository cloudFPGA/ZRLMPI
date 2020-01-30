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

__fallback_max_buffer_size__ = 1500  # we have to find one


def process_ast(c_ast_orig, cluster_description, hw_file_pre_parsing, target_file_name):
    # 1. find buffer names
    # 2. find rank names
    find_name_visitor = name_visitor.MpiSignatureNameSearcher()
    find_name_visitor.visit(c_ast_orig)
    buffer_variable_names, buffer_variable_obj = find_name_visitor.get_results_buffers()
    rank_variable_names, rank_variable_obj = find_name_visitor.get_results_ranks()
    # print(buffer_variable_names)
    # print(rank_variable_names)

    # 3. determine buffer size (and return them)
    get_value_visitor = value_visitor.MpiVariableValueSearcher(buffer_variable_names, rank_variable_names)
    get_value_visitor.visit(c_ast_orig)
    found_compares = get_value_visitor.get_results_compares()
    found_buffer_dims = get_value_visitor.get_results_buffers()
    # print(found_buffer_dims)
    list_of_dims = []
    for e in found_buffer_dims.keys():
        if type(found_buffer_dims[e]) == c_ast.Constant:
            list_of_dims.append(int(found_buffer_dims[e].value)) # we can trust that buffer sizes are ints..
        else:
            print("Found NON CONSTANT BUFFER DECLARATION in {} : This is NOT YET SUPPORTED".format(str(e)))
    max_dimension = 0
    if len(list_of_dims) == 0:
        print("[WARNING] Did not found a maximumx buffer size, this could lead to a failing HLS synthesis")
        max_dimension = __fallback_max_buffer_size__
    else:
        max_dimension = max(list_of_dims)
    print("Found max MPI buffer size: {}\n".format(max_dimension))

    # 4. detect FPGA parts based on rank (from cluster description)
    found_compares = get_value_visitor.get_results_compares()
    print("Found {} compares of the rank variable.\n".format(len(found_compares)))
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

    # 5. modify AST with constant values (i.e. copy into new)
    # TODO: build visitor that copies all nodes into new ast
    # TODO: but in case of the compares in compares_invariant_for_fpgas, replace the node with the resulting block
    # visitors can't change the ast...so we want to have a list of statements to change...
    if len(compares_invariant_for_fpgas) > 0:
        find_affected_node_visitor = statement_visitor.MpiAffectedStatementSearcher(compares_invariant_for_fpgas)
        find_affected_node_visitor.visit(c_ast_orig)
        affected_nodes = find_affected_node_visitor.get_found_statements()
        new_ast = c_ast_orig
        # number_of_replacements = len(affected_nodes)
        # print("Start to replace {} nodes.\n".format(number_of_replacements))
        # # nodes_to_visit = new_ast.children()
        # nodes_to_visit = [new_ast]
        # while (len(nodes_to_visit) > 0) and (number_of_replacements > 0):
        #     current_node = nodes_to_visit[0]
        #     for e in affected_nodes:
        #         if current_node == e['old']:
        #             current_node = e['new']
        #             number_of_replacements -= 1
        #         else:
        #             for c in current_node.children():
        #                 nodes_to_visit.extend(c[1])
        #     del nodes_to_visit[0]
        # #for e in affected_nodes:
        # #    replacement = e['new']
        # #    e['old'] = replacement
    else:
        print("No invariant rank statement for FPGAs found.")
        new_ast = c_ast_orig

    # 5. generate C code again
    # 6. append original header lines and save in file

    generator = c_generator.CGenerator()
    generated_c = str(generator.visit(new_ast))

    awk_command = "awk '/int.*main\(/{ print NR; exit }'  " + str(hw_file_pre_parsing)
    print(awk_command)
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

    head = ""
    with open(hw_file_pre_parsing, 'r') as in_file:
         head = [next(in_file) for x in range(line_number - 1)]

    head_str = ""
    for e in head:
        head_str += str(e)

    concatenated_file = head_str + "\n" + generated_c

    print("Writing new c code to file {}.\n".format(target_file_name))

    with open(target_file_name, 'w+') as target_file:
        target_file.write(concatenated_file)

    return max_dimension    # ZRLMPI_MAX_DETECTED_BUFFER_SIZE

