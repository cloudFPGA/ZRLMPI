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
#  *        Visitor searching for all buffers that are used in context of MPI calls.
#  *
#  *

from pycparser import c_ast


class MpiStatementReplaceVisitor(object):
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

    def __init__(self, objects_to_replace_dict):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.objects_to_replace = []
        self.objects_to_search = []
        for e in objects_to_replace_dict:
            self.objects_to_replace.append(e['new'])
            self.objects_to_search.append(e['old'])

    def check_and_replace_list(self, list_to_replace):
        ret = False
        for i in range(0, len(list_to_replace)):
            e = list_to_replace[i]
            if e in self.objects_to_search:
                target_index = self.objects_to_search.index(e)
                list_to_replace[i] = self.objects_to_replace[target_index]
                ret = True
        return ret

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
            if c in self.objects_to_search:
                target_index = self.objects_to_search.index(c)
                # if node.block_items:
                if hasattr(node, 'block_items'):
                    insert_index = node.block_items.index(c)
                    # node.block_items[ni] = self.objects_to_replace[oi]
                    # ok, now we know where to insert, but we have to split  the "stmt" of if/else, otherwise there would be
                    # some unnecessary {..} in the code
                    list_to_insert = []
                    if type(self.objects_to_replace[target_index]) is list:
                        for e in self.objects_to_replace[target_index]:
                            if type(e) is c_ast.Compound:
                                list_to_insert.extend(e.block_items)
                            else:
                                list_to_insert.append(e)
                    else:
                        if type(self.objects_to_replace[target_index]) is c_ast.Compound:
                            list_to_insert.extend(self.objects_to_replace[target_index].block_items)
                        else:
                            list_to_insert.append(self.objects_to_replace[target_index])
                    del node.block_items[insert_index]
                    # insert_index -= 1 #TODO
                    if insert_index < 0:
                        insert_index = 0
                    for e in list_to_insert:
                        node.block_items.insert(insert_index, e)
                        insert_index += 1
                elif hasattr(node, 'expr') and node.expr == self.objects_to_search[target_index]:
                    node.expr = self.objects_to_replace[target_index]
                else:
                    # node[c] = self.objects_to_replace[target_index]
                    self.visit(c)
            else:
                self.visit(c)

    def visit_UnaryOp(self, n):
        list_to_replace = [n.expr]
        was_replaced = self.check_and_replace_list(list_to_replace)
        # assumption: if one thing was replaced, we don't need to visit further
        if not was_replaced:
            for e in list_to_replace:
                self.visit(e)
        else:
            n.expr = list_to_replace[0]

    def visit_BinaryOp(self, n):
        list_to_replace = [n.left, n.right]
        was_replaced = self.check_and_replace_list(list_to_replace)
        # assumption: if one thing was replaced, we don't need to visit further
        if not was_replaced:
            for e in list_to_replace:
                self.visit(e)
        else:
            n.left = list_to_replace[0]
            n.right = list_to_replace[1]

    def visit_Assignment(self, n):
        list_to_replace = [n.rvalue, n.lvalue]
        was_replaced = self.check_and_replace_list(list_to_replace)
        # assumption: if one thing was replaced, we don't need to visit further
        if not was_replaced:
            for e in list_to_replace:
                self.visit(e)
        else:
            n.rvalue = list_to_replace[0]
            n.lvalue = list_to_replace[1]

    def visit_Decl(self, n):
        if n.init:
            list_to_replace = [n.init]
            was_replaced = self.check_and_replace_list(list_to_replace)
            # assumption: if one thing was replaced, we don't need to visit further
            if not was_replaced:
                for e in list_to_replace:
                    self.visit(e)
            else:
                n.init = list_to_replace[0]
        if n.type:
            list_to_replace = [n.type]
            was_replaced = self.check_and_replace_list(list_to_replace)
            # assumption: if one thing was replaced, we don't need to visit further
            if not was_replaced:
                for e in list_to_replace:
                    self.visit(e)
            else:
                n.type = list_to_replace[0]

    def visit_ArrayDecl(self, n):
        if n.type:
            list_to_replace = [n.type]
            was_replaced = self.check_and_replace_list(list_to_replace)
            # assumption: if one thing was replaced, we don't need to visit further
            if not was_replaced:
                for e in list_to_replace:
                    self.visit(e)
            else:
                n.type = list_to_replace[0]

