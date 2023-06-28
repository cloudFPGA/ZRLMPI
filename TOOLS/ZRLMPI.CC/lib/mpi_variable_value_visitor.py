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
#  *     Created: Jan 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Visitor searching for buffer definitions and compare values of variables
#  *
#  *


from pycparser import c_ast

__c_int_compare_operators__ = ["==", "<=", ">=", ">", "<", "!="]


class MpiVariableValueSearcher(object):
    """
     A base NodeVisitor class for visiting c_ast nodes.
        Subclass it and define your own visit_XXX methods, where
        XXX is the class name you want to visit with these
        methods.
        For example:
        class ConstantVisitor(NodeVisitor):
            def __init__(self):
                self.values = []
            def visit_Constant(self, node):
                self.values.append(node.value)
        Creates a list of values of all the constant nodes
        encountered below the given node. To use it:
        cv = ConstantVisitor()
        cv.visit(node)
        Notes:
        *   generic_visit() will be called for AST nodes for which
            no visit_XXX method was defined.
        *   The children of nodes for which a visit_XXX was
            defined will not be visited - if you need this, call
            generic_visit() on the node.
            You can use:
                NodeVisitor.generic_visit(self, node)
        *   Modeled after Python's own AST visiting facilities
            (the ast module of Python 3.0)
    """

    _method_cache = None

    def __init__(self, buffer_names, compare_names):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.buffer_names_to_search = buffer_names
        self.variable_names_to_comapre = compare_names
        self.found_buffer_definition = {}
        self.found_compares = []

    def get_results_buffers(self):
        return self.found_buffer_definition

    def get_results_compares(self):
        return self.found_compares

    def visit(self, node):
        """ Visit a node.
        """

        if self._method_cache is None:
            self._method_cache = {}

        visitor = self._method_cache.get(node.__class__.__name__, None)
        if visitor is None:
            method = 'visit_' + node.__class__.__name__
            visitor = getattr(self, method, self.generic_visit)
            self._method_cache[node.__class__.__name__] = visitor

        return visitor(node)

    def generic_visit(self, node):
        """ Called if no explicit visitor function exists for a
            node. Implements preorder visiting of the node.
        """
        for c in node:
            self.visit(c)

    def visit_BinaryOp(self, n):
        if n.op in __c_int_compare_operators__:
            new_compare = {}
            if hasattr(n.left, 'name') and n.left.name in self.variable_names_to_comapre:
                new_compare['name'] = n.left.name
                new_compare['other'] = n.right
                new_compare['position'] = "left"
            elif hasattr(n.right, 'name') and n.right.name in self.variable_names_to_comapre:
                new_compare['name'] = n.right.name
                new_compare['other'] = n.left
                new_compare['position'] = "right"

            if 'other' in new_compare.keys() and type(new_compare['other']) == c_ast.Constant:
                new_compare['c_value'] = new_compare['other'].value
                new_compare['c_type'] = new_compare['other'].type

            if 'other' in new_compare.keys():
                new_compare['op'] = n.op
                new_compare['operator_object'] = n
                self.found_compares.append(new_compare)

    def visit_Decl(self, n, no_type=False):
        if n.name in self.buffer_names_to_search:
            try:
                new_obj = {}
                new_obj['node'] = n
                new_obj['further_def'] = []
                name = n.name
                dim = 1
                buffer_type = ""
                current_obj = n.type
                if type(current_obj) is c_ast.PtrDecl:
                    # skip for now TODO
                    return
                parsed = False
                while True:
                    if type(current_obj) is c_ast.IdentifierType:
                        # last in a chain...
                        buffer_type = current_obj.names[0]
                        parsed = True
                        break
                    new_obj['further_def'].append(current_obj)
                    if hasattr(current_obj, 'dim'):
                        if type(current_obj.dim) is c_ast.Constant:
                            dim *= int(current_obj.dim.value)
                            parsed = True
                        elif type(current_obj.dim) is c_ast.BinaryOp:
                            if type(current_obj.dim.left) is c_ast.Constant and type(current_obj.dim.right) is c_ast.Constant:
                                result = eval(current_obj.dim.left.value + current_obj.dim.op + current_obj.dim.right.value)
                                dim *= int(result)
                                parsed = True
                            else:
                                print("Found NON CONSTANT BUFFER DECLARATION in {} : This is NOT YET SUPPORTED".format(str(current_obj)))
                    if not hasattr(current_obj, 'type'):
                        if hasattr(current_obj, 'names'):
                            buffer_type = current_obj.names[0]
                        break
                    else:
                        if type(current_obj.type) is str:
                            buffer_type = current_obj.type
                            break
                        current_obj = current_obj.type

                if not parsed:
                    print("FAILED to parse buffer dimension in {} : This is NOT YET SUPPORTED".format(str(current_obj)))
                new_obj['calculated_value'] = dim
                new_obj['found_type'] = buffer_type
                new_obj['name'] = name
                self.found_buffer_definition[n.name] = new_obj
            except Exception as e:
                print(e)
                print("FAILED to determine buffer dimension of declaration at {}\n".format(str(n.coord)))

    def visit_DeclList(self, n):
        # TODO...but may also not be important, since arrays cannot be implemented as declList?
        # print("[WARNING] visit of DeclList NOT YET IMPLEMENTED")
        return

    def visit_Switch(self, n):
        if n.cond in self.variable_names_to_comapre:
            print("variable {} found in switch statement: NOT YET IMPLEMENTED\n".format(n.cond))
