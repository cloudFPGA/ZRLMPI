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


class MpiMemoryFunctionFindVisitor(object):

    _method_cache = None

    def __init__(self, objects_to_search_start):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.objects_to_search = []
        self.objects_to_search.extend(objects_to_search_start)
        self.found_assignments = []

    def get_found_assignments(self):
       return self.found_assignments

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
            for o in self.objects_to_search:
                if c == o:
                    # we found smth, but we are not yet an assignment, so add it to objects
                    self.objects_to_search.append(node)
                    break

    def visit_Assignment(self, n):
        for c in n:
            self.visit(c)
            for o in self.objects_to_search:
                if c == o:
                    self.found_assignments.append(n)
                    break

