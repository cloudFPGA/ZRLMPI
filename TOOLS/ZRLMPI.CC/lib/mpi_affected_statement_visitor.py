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


class MpiAffectedStatementSearcher(object):
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

    def __init__(self, conditions_to_search):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.conditions_to_search = conditions_to_search
        self.found_statements = []
        # self.found_assignments = []

    def get_found_statements(self):
        return self.found_statements

    # def get_found_assignments(self):
    #    return self.found_assignments

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

    def visit_TernaryOp(self, n):
        for e in self.conditions_to_search:
            if n.cond == e['operator_object']:
                new_found = {}
                new_found['old'] = n
                if e['fpga_decision_value'] == "True":
                    new_found['new'] = n.iftrue
                else:
                    new_found['new'] = n.iffalse
                self.found_statements.append(new_found)

    def visit_If(self, n):
        for e in self.conditions_to_search:
            if n.cond == e['operator_object']:
                new_found = {}
                result_value = -1
                new_found['old'] = n
                if e['fpga_decision_value'] == "True":
                    new_found['new'] = n.iftrue
                    result_value = 1
                else:
                    if n.iffalse is not None:
                        new_found['new'] = n.iffalse
                    else:
                        new_found['new'] = c_ast.EmptyStatement()
                    result_value = 0
                self.found_statements.append(new_found)
                n.cond = c_ast.Constant('int', str(result_value))

    def visit_Switch(self, n):
        for e in self.conditions_to_search:
            if n.cond == e['operator_object']:
                print("A switch statement has a rank condition: NOT YET IMPLEMENTED\n".format(n.cond))

