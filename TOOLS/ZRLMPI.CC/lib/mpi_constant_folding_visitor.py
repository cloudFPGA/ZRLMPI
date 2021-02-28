#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Jan 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Visitor searching for buffer definitions and compare values of variables
#  *
#  *

# based on
#   https://github.com/eliben/pycparser/blob/master/pycparser/c_generator.py
#   and https://github.com/eliben/pycparser/blob/master/pycparser/c_ast.py


from pycparser import c_ast
import ctypes

__c_int_compare_operators__ = ["==", "<=", ">=", ">", "<", "!="]
__root_context_hash__ = hash("ast_root_context")


def _equalize_node(n):
    new_n = n
    if hasattr(new_n, 'coord'):
        new_n.coord = None
    return new_n


def _own_poor_ast_hash(n):
    # TODO: improve
    h = str(_equalize_node(n))
    return h


__c_implicit_type_pyramide__ = ['bool', 'char', 'short int', 'int' , 'unsigned int', 'long', 'unsigned',
                                'long long', 'float', 'double', 'long double']
__functions_not_to_fold__ = ['MPI_Comm_rank', 'MPI_Comm_size']
__ids_not_to_replace__ = ['MPI_COMM_WORLD, MPI_INTEGER, MPI_FLOAT, MPI_SUM']


def _get_c_highest_shared_type(t1, t2):
    r1 = __c_implicit_type_pyramide__.index(t1)
    r2 = __c_implicit_type_pyramide__.index(t2)
    st = __c_implicit_type_pyramide__[max(r1,r2)]
    return st


class MpiConstantFoldingVisitor(object):
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

    def __init__(self):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.replaced_constant_cache = {}
        # self.replaced_constant_list = []
        self.new_objects_list = []
        self.current_context_hash = __root_context_hash__
        self.context_stack = []
        self.do_not_touch_objects = []
        self.do_replace_constant = True
        self.Im_first_call = True
        self.init_of_for_loop = False
        self.body_of_loop = False

    def get_new_objects(self):
        return self.new_objects_list

    def get_context_based_obj_hash(self, n):
        # nhs = "{}_{}".format(self.current_context_hash, hash(_equalize_node(n)))
        nhs = "{}_{}".format(self.current_context_hash, _own_poor_ast_hash(n))
        return hash(nhs)

    def get_list_of_visible_obj_hashs(self, n):
        ret = []
        for c in self.context_stack:
            nhs = "{}_{}".format(c, _own_poor_ast_hash(n))
            ret.append(hash(nhs))
        ret.append(self.get_context_based_obj_hash(n))
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


        # if self.Im_first_call:
        #    self.Im_first_call = False
        #    visitor(node)
        #    self.do_replace_constant = True
        #    return visitor(node)
        #else:
        #    return visitor(node)
        return visitor(node)

    def generic_visit(self, node):
        """ Called if no explicit visitor function exists for a
            node. Implements preorder visiting of the node.
        """
        for c in node:
            self.visit(c)

    # def visit_Constant(self, n):
    #     return n.value

    # def visit_ID(self, n):
    #     return n.name

    # def visit_Pragma(self, n):
    #     ret = '#pragma'
    #     if n.string:
    #         ret += ' ' + n.string
    #     return ret

    # def visit_ArrayRef(self, n):
    #     arrref = self._parenthesize_unless_simple(n.name)
    #     return arrref + '[' + self.visit(n.subscript) + ']'

    # def visit_StructRef(self, n):
    #     sref = self._parenthesize_unless_simple(n.name)
    #     return sref + n.type + self.visit(n.field)

    def visit_FuncCall(self, n):
        func_name = n.name.name
        if func_name == 'sizeof' and len(n.args.exprs) == 1 and n.args.exprs[0].value != 'void':
            cmd = "ctypes.sizeof(ctypes.c_{})".format(n.args.exprs[0].value)
            result_value = eval(cmd)
            new_node = c_ast.Constant('int', str(result_value))
            new_entry = {'old': n, 'new': new_node}
            self.new_objects_list.append(new_entry)
            lnh = self.get_list_of_visible_obj_hashs(n)
            for e in lnh:
                self.replaced_constant_cache[e] = new_node
        else:
            need_replace = False
            new_args = []
            if n.args is not None and n.name.name not in __functions_not_to_fold__:
                for a in n.args.exprs:
                    found_local = False
                    if type(a) is not c_ast.Constant:
                        self.visit(a)
                        ahl = self.get_list_of_visible_obj_hashs(a)
                        ahl.reverse()
                        dont_continue = False
                        for ah in ahl:
                            if ah in self.do_not_touch_objects:
                                dont_continue = True
                                break
                        if type(a) is c_ast.ID and not dont_continue:
                            if a.name not in __ids_not_to_replace__:
                                for ah in ahl:
                                    if ah in self.replaced_constant_cache:
                                        constant = self.replaced_constant_cache[ah]
                                        new_args.append(constant)
                                        found_local = True
                                        need_replace = True
                                        # one is enough!
                                        break
                    if not found_local:
                        new_args.append(a)
                if need_replace and self.do_replace_constant:
                    expr_list = c_ast.ExprList(new_args)
                    new_node = c_ast.FuncCall(n.name, expr_list)
                    new_entry = {'old': n, 'new': new_node}
                    self.new_objects_list.append(new_entry)
                    # no need to add it to constant cache...
        return

    def visit_Assignment(self, n):
        # variables on the left are deleted from constant cache if not used as constants
        if type(n.lvalue) is not c_ast.ID:
            self.visit(n.lvalue)
        if type(n.rvalue) is not c_ast.ID:
            self.visit(n.rvalue)
        #if type(n.lvalue) is c_ast.ID:
        #    lhl = self.get_list_of_visible_obj_hashs(n.lvalue)
        #    lhl.reverse()
        #    for lh in lhl:
        #        if lh in self.do_not_touch_objects:
        #            return
        # rh = self.get_context_based_obj_hash(n.rvalue)
        rhl = self.get_list_of_visible_obj_hashs(n.rvalue)
        rhl.reverse()
        # for r in rhl:
        #     if r in self.do_not_touch_objects:
        #         return
        # nh = self.get_context_based_obj_hash(n)
        if type(n.lvalue) is c_ast.ID:
            if type(n.rvalue) is c_ast.Constant:
                constant = n.rvalue
                # add id to replace values
                decl_ID = c_ast.ID(n.lvalue.name)
                # nih = self.get_context_based_obj_hash(decl_ID)
                # overwrite if necessary...
                # self.replaced_constant_cache[nih] = constant
                lnh = self.get_list_of_visible_obj_hashs(decl_ID)
                for e in lnh:
                    self.replaced_constant_cache[e] = constant
            else:
                found_constant = False
                for rh in rhl:
                    if rh in self.replaced_constant_cache:
                        found_constant = True
                        if not self.do_replace_constant:
                            break
                        constant = self.replaced_constant_cache[rh]
                        new_node = c_ast.Assignment(op=n.op, lvalue=n.lvalue, rvalue=constant, coord=n.coord)
                        new_entry = {'old': n, 'new': new_node}
                        self.new_objects_list.append(new_entry)
                        # self.replaced_constant_cache[nh] = new_node
                        lnh = self.get_list_of_visible_obj_hashs(n)
                        for e in lnh:
                            self.replaced_constant_cache[e] = new_node
                        # also add ID
                        decl_ID = c_ast.ID(n.lvalue.name)
                        # nih = self.get_context_based_obj_hash(decl_ID)
                        # overwrite if necessary...
                        # self.replaced_constant_cache[nih] = constant
                        lnh2 = self.get_list_of_visible_obj_hashs(decl_ID)
                        for e in lnh2:
                            self.replaced_constant_cache[e] = constant
                        break
                if not found_constant and self.do_replace_constant:
                    # we have to remove it from all caches
                    decl_ID = c_ast.ID(n.lvalue.name)
                    lnh = self.get_list_of_visible_obj_hashs(decl_ID)
                    for e in lnh:
                        if e in self.replaced_constant_cache:
                            del self.replaced_constant_cache[e]
                        # self.do_not_touch_objects.append(e)
        return

    # def visit_IdentifierType(self, n):
    #     return ' '.join(n.names)
    #
    # def _visit_expr(self, n):
    #     if isinstance(n, c_ast.InitList):
    #         return '{' + self.visit(n) + '}'
    #     elif isinstance(n, c_ast.ExprList):
    #         return '(' + self.visit(n) + ')'
    #     else:
    #         return self.visit(n)

    def visit_Decl(self, n):
        #if self.init_of_for_loop or self.body_of_loop:
        #    decl_ID = c_ast.ID(n.name)
        #    lnh2 = self.get_list_of_visible_obj_hashs(decl_ID)
        #    for e in lnh2:
        #        self.do_not_touch_objects.append(e)
        #    return
        if n.init:
            self.visit(n.init)
            if type(n.init) is c_ast.Constant:
                decl_ID = c_ast.ID(n.name)
                # nih = self.get_context_based_obj_hash(decl_ID)
                # self.replaced_constant_cache[nih] = self.replaced_constant_cache[ch]
                lnh2 = self.get_list_of_visible_obj_hashs(decl_ID)
                for e in lnh2:
                    self.replaced_constant_cache[e] = n.init
            else:
                # ch = self.get_context_based_obj_hash(n.init)
                chl = self.get_list_of_visible_obj_hashs(n.init)
                chl.reverse()
                found_constant = False
                for ch in chl:
                    if ch in self.replaced_constant_cache:
                        found_constant = True
                        if not self.do_replace_constant:
                            break
                        new_node = c_ast.Decl(name=n.name, quals=n.quals, storage=n.storage, funcspec=n.funcspec, type=n.type,
                                          bitsize=n.bitsize, coord=n.coord, init=self.replaced_constant_cache[ch])
                        new_entry = {'old': n, 'new': new_node}
                        self.new_objects_list.append(new_entry)
                        # nh = self.get_context_based_obj_hash(n)
                        # self.replaced_constant_cache[nh] = new_node
                        lnh = self.get_list_of_visible_obj_hashs(n)
                        for e in lnh:
                            self.replaced_constant_cache[e] = new_node
                        # also add hash of ID and replace, if already constant
                        # if type(self.replaced_constant_cache[ch]) is c_ast.Constant:
                        decl_ID = c_ast.ID(n.name)
                        # nih = self.get_context_based_obj_hash(decl_ID)
                        # self.replaced_constant_cache[nih] = self.replaced_constant_cache[ch]
                        lnh2 = self.get_list_of_visible_obj_hashs(decl_ID)
                        for e in lnh2:
                            self.replaced_constant_cache[e] = self.replaced_constant_cache[ch]
                        break
                # if not found_constant and self.do_replace_constant:
                #     decl_ID = c_ast.ID(n.name)
                #     lnh2 = self.get_list_of_visible_obj_hashs(decl_ID)
                #     for e in lnh2:
                #         self.do_not_touch_objects.append(e)
        if n.type:
            self.visit(n.type)

    # def visit_DeclList(self, n):
    #     # for i in n.decls:
    #     #    self.visit(i)
    #     # DeclLists are part of FOR etc...so we better do nothing
    #     return

    def visit_ArrayDecl(self, n):
        self.visit(n.type)
        if n.dim:
            self.visit(n.dim)
            # ch = self.get_context_based_obj_hash(n.dim)
            chl = self.get_list_of_visible_obj_hashs(n.dim)
            chl.reverse()
            for ch in chl:
                if ch in self.replaced_constant_cache:
                    constant = self.replaced_constant_cache[ch]
                    new_node = c_ast.ArrayDecl(type=n.type, dim_quals=n.dim_quals, coord=n.coord,
                                               dim=constant)
                    new_entry = {'old': n, 'new': new_node}
                    self.new_objects_list.append(new_entry)
                    # no entry in replaced_cache...since it is "just" the definition

    # def visit_Typedef(self, n):
    #     s = ''
    #     if n.storage: s += ' '.join(n.storage) + ' '
    #     s += self._generate_type(n.type)
    #     return s
    #
    # def visit_Cast(self, n):
    #     s = '(' + self._generate_type(n.to_type) + ')'
    #     return s + ' ' + self._parenthesize_unless_simple(n.expr)
    #
    # def visit_ExprList(self, n):
    #     visited_subexprs = []
    #     for expr in n.exprs:
    #         visited_subexprs.append(self._visit_expr(expr))
    #     return ', '.join(visited_subexprs)
    #
    # def visit_InitList(self, n):
    #     visited_subexprs = []
    #     for expr in n.exprs:
    #         visited_subexprs.append(self._visit_expr(expr))
    #     return ', '.join(visited_subexprs)
    #
    # def visit_Enum(self, n):
    #     return self._generate_struct_union_enum(n, name='enum')
    #
    # def visit_Enumerator(self, n):
    #     if not n.value:
    #         return '{indent}{name},\n'.format(
    #             indent=self._make_indent(),
    #             name=n.name,
    #         )
    #     else:
    #         return '{indent}{name} = {value},\n'.format(
    #             indent=self._make_indent(),
    #             name=n.name,
    #             value=self.visit(n.value),
    #         )
    #
    # def visit_FuncDef(self, n):
    #     decl = self.visit(n.decl)
    #     self.indent_level = 0
    #     body = self.visit(n.body)
    #     if n.param_decls:
    #         knrdecls = ';\n'.join(self.visit(p) for p in n.param_decls)
    #         return decl + '\n' + knrdecls + ';\n' + body + '\n'
    #     else:
    #         return decl + '\n' + body + '\n'
    #
    # def visit_FileAST(self, n):
    #     s = ''
    #     for ext in n.ext:
    #         if isinstance(ext, c_ast.FuncDef):
    #             s += self.visit(ext)
    #         elif isinstance(ext, c_ast.Pragma):
    #             s += self.visit(ext) + '\n'
    #         else:
    #             s += self.visit(ext) + ';\n'
    #     return s

    def visit_Compound(self, n):
        self.context_stack.append(self.current_context_hash)
        self.current_context_hash = hash(n)
        if n.block_items:
            for stmt in n.block_items:
                self.visit(stmt)
        self.current_context_hash = self.context_stack[-1]
        del self.context_stack[-1]

    # def visit_CompoundLiteral(self, n):
    #     return '(' + self.visit(n.type) + '){' + self.visit(n.init) + '}'
    #
    #
    # def visit_EmptyStatement(self, n):
    #     return ';'
    #
    # def visit_ParamList(self, n):
    #     return ', '.join(self.visit(param) for param in n.params)
    #
    # def visit_Return(self, n):
    #     s = 'return'
    #     if n.expr: s += ' ' + self.visit(n.expr)
    #     return s + ';'
    #
    # def visit_Break(self, n):
    #     return 'break;'
    #
    # def visit_Continue(self, n):
    #     return 'continue;'

    def visit_UnaryOp(self, n):
        operand = None
        if type(n.expr) is c_ast.Constant:
            operand = n.expr
        elif type(n.expr) is c_ast.ID:
            self.visit(n.expr)
            # here, we cannot decide wether an i++ is in a loop or not
            # so, we stop
            operand = None
            # and we have to delete it from constant cache
            snh = self.get_list_of_visible_obj_hashs(n.expr)
            for e in snh:
                if e in self.replaced_constant_cache:
                    del self.replaced_constant_cache[e]
                # self.do_not_touch_objects.append(e)
        else:
            self.visit(n.expr)
            nhl = self.get_list_of_visible_obj_hashs(n.expr)
            nhl.reverse()
            for nh in nhl:
                if nh in self.replaced_constant_cache and self.do_replace_constant:
                    operand = self.replaced_constant_cache[nh]
                    assert type(operand) is c_ast.Constant
                    # NO break, because we want the most inner match?
        if operand is not None:
            # we execute it
            result_value = 0
            cmd_exectued = False
            if n.op == 'p++':
                cmd = "{} + 1".format(operand.value)
                result_value = eval(cmd)
                cmd_exectued = True
            elif n.op == 'p--':
                cmd = "{} - 1".format(operand.value)
                result_value = eval(cmd)
                cmd_exectued = True
            elif n.op == 'sizeof':
                cmd = "ctypes.sizeof(ctypes.c_{})".format(operand.value)
                result_value = eval(cmd)
                cmd_exectued = True
            if cmd_exectued:
                new_node = c_ast.Constant(operand.type, str(result_value))
                new_entry = {'old': n, 'new': new_node}
                self.new_objects_list.append(new_entry)
                # nh = self.get_context_based_obj_hash(n)
                # self.replaced_constant_cache[nh] = new_node
                lnh = self.get_list_of_visible_obj_hashs(n)
                for e in lnh:
                    self.replaced_constant_cache[e] = new_node
        return

    def visit_BinaryOp(self, n):
        self.visit(n.left)
        self.visit(n.right)
        right_operand = None
        left_operand = None
        lhs = self.get_list_of_visible_obj_hashs(n.left)
        lhs.reverse()
        rhs = self.get_list_of_visible_obj_hashs(n.right)
        rhs.reverse()
        if type(n.left) is c_ast.Constant:
            left_operand = n.left
        else:
            for lh in lhs:
                if lh in self.replaced_constant_cache and self.do_replace_constant:
                    if lh in self.do_not_touch_objects:
                        left_operand = None
                        break
                    left_operand = self.replaced_constant_cache[lh]
                    assert type(left_operand) is c_ast.Constant
                    # NO break, because we want the most inner match?
        if type(n.right) is c_ast.Constant:
            right_operand = n.right
        else:
            for rh in rhs:
                if rh in self.replaced_constant_cache and self.do_replace_constant:
                    if rh in self.do_not_touch_objects:
                        right_operand = None
                        break
                    right_operand = self.replaced_constant_cache[rh]
                    assert type(right_operand) is c_ast.Constant
                    # NO break, because we want the most inner match?

        if right_operand is not None and left_operand is not None:
            # we can execute it
            operation = n.op
            if operation == '||':
                operation = 'or'
            elif operation == '&&':
                operation = 'and'
            cmd = "{} {} {}".format(left_operand.value, operation, right_operand.value)
            result_value = eval(cmd)
            if left_operand.type == right_operand.type:
                result_type = left_operand.type
            else:
                result_type = _get_c_highest_shared_type(left_operand.type, right_operand.type)
                # result_type = type(result_value).__name__
            # if result_type == 'int':
            if result_type != 'float' and result_type != 'double' and result_type != 'long double':
                    result_value = int(result_value)
            new_node = c_ast.Constant(result_type, str(result_value))
            new_entry = {'old': n, 'new': new_node}
            self.new_objects_list.append(new_entry)
            # nh = self.get_context_based_obj_hash(n)
            # self.replaced_constant_cache[nh] = new_node
            lnh = self.get_list_of_visible_obj_hashs(n)
            for e in lnh:
                self.replaced_constant_cache[e] = new_node
        else:
            # only one is constant, we replace it
            if self.do_replace_constant:
                if left_operand is not None:
                    n.left = left_operand
                elif right_operand is not None:
                    n.right = right_operand

        return

    def visit_TernaryOp(self, n):
        self.visit(n.cond)
        compare = None
        ncl = self.get_list_of_visible_obj_hashs(n.cond)
        ncl.reverse()
        if type(n.cond) is c_ast.Constant:
            compare = n.cond
        else:
            for nc in ncl:
                if nc in self.replaced_constant_cache and self.do_replace_constant:
                    compare = self.replaced_constant_cache[nc]

        if compare is not None and self.do_replace_constant:
            result_node = None
            if compare.value == "True":
                result_node = n.iftrue
            elif compare.value == "False":
                result_node = n.iffalse
            else:
                print("ERROR: TenaryOperation with constant result that is not a booolean varuable, skipping this node: {}".format(n))
                return
            new_entry = {'old': n, 'new': result_node}
            self.new_objects_list.append(new_entry)
            if type(result_node) is c_ast.Constant:
                # nh = self.get_context_based_obj_hash(n)
                # self.replaced_constant_cache[nh] = result_node
                lnh = self.get_list_of_visible_obj_hashs(n)
                for e in lnh:
                    self.replaced_constant_cache[e] = result_node
        return

    # def visit_If(self, n):
    #     for e in self.conditions_to_search:
    #         if n.cond == e['operator_object']:
    #             new_found = {}
    #             result_value = -1
    #             new_found['old'] = n
    #             if e['fpga_decision_value'] == "True":
    #                 new_found['new'] = n.iftrue
    #                 result_value = 1
    #             else:
    #                 if n.iffalse is not None:
    #                     new_found['new'] = n.iffalse
    #                 else:
    #                     new_found['new'] = c_ast.EmptyStatement()
    #                 result_value = 0
    #             self.found_statements.append(new_found)
    #             n.cond = c_ast.Constant('int', str(result_value))

    def visit_For(self, n):
        # we better NOT visit n.init and n.cond, since we know there will rarely be constants
        # self.init_of_for_loop = True
        # self.visit(n.init)
        # self.init_of_for_loop = False
        was_true = self.body_of_loop
        self.body_of_loop = True
        self.visit(n.stmt)
        if not was_true:
            self.body_of_loop = False
        self.visit(n.next)
        return

    def visit_While(self, n):
        # we better NOT visit n.cond, since we know there will rarely be constants
        was_true = self.body_of_loop
        self.body_of_loop = True
        self.visit(n.stmt)
        if not was_true:
            self.body_of_loop = False
        return

    def visit_DoWhile(self, n):
        # we better NOT visit n.cond, since we know there will rarely be constants
        was_true = self.body_of_loop
        self.body_of_loop = True
        self.visit(n.stmt)
        if not was_true:
            self.body_of_loop = False
        return

    # def visit_Switch(self, n):
    #     for e in self.conditions_to_search:
    #         if n.cond == e['operator_object']:
    #             print("A switch statement has a rank condition: NOT YET IMPLEMENTED\n".format(n.cond))

    # def visit_Case(self, n):
    #     s = 'case ' + self.visit(n.expr) + ':\n'
    #     for stmt in n.stmts:
    #         s += self._generate_stmt(stmt, add_indent=True)
    #     return s
    #
    # def visit_Default(self, n):
    #     s = 'default:\n'
    #     for stmt in n.stmts:
    #         s += self._generate_stmt(stmt, add_indent=True)
    #     return s
    #
    # def visit_Label(self, n):
    #     return n.name + ':\n' + self._generate_stmt(n.stmt)
    #
    # def visit_Goto(self, n):
    #     return 'goto ' + n.name + ';'
    #
    # def visit_EllipsisParam(self, n):
    #     return '...'
    #
    # def visit_Struct(self, n):
    #     return self._generate_struct_union_enum(n, 'struct')
    #
    # def visit_Typename(self, n):
    #     return self._generate_type(n.type)
    #
    # def visit_Union(self, n):
    #     return self._generate_struct_union_enum(n, 'union')
    #
    # def visit_NamedInitializer(self, n):
    #     s = ''
    #     for name in n.name:
    #         if isinstance(name, c_ast.ID):
    #             s += '.' + name.name
    #         else:
    #             s += '[' + self.visit(name) + ']'
    #     s += ' = ' + self._visit_expr(n.expr)
    #     return s
    #
    # def visit_FuncDecl(self, n):
    #     return self._generate_type(n)
    #
    # def _generate_struct_union_enum(self, n, name):
    #     """ Generates code for structs, unions, and enums. name should be
    #         'struct', 'union', or 'enum'.
    #     """
    #     if name in ('struct', 'union'):
    #         members = n.decls
    #         body_function = self._generate_struct_union_body
    #     else:
    #         assert name == 'enum'
    #         members = None if n.values is None else n.values.enumerators
    #         body_function = self._generate_enum_body
    #     s = name + ' ' + (n.name or '')
    #     if members is not None:
    #         # None means no members
    #         # Empty sequence means an empty list of members
    #         s += '\n'
    #         s += self._make_indent()
    #         self.indent_level += 2
    #         s += '{\n'
    #         s += body_function(members)
    #         self.indent_level -= 2
    #         s += self._make_indent() + '}'
    #     return s
    #
    # def _generate_struct_union_body(self, members):
    #     return ''.join(self._generate_stmt(decl) for decl in members)
    #
    # def _generate_enum_body(self, members):
    #     # `[:-2] + '\n'` removes the final `,` from the enumerator list
    #     return ''.join(self.visit(value) for value in members)[:-2] + '\n'
    #
    # def _generate_stmt(self, n, add_indent=False):
    #     """ Generation from a statement node. This method exists as a wrapper
    #         for individual visit_* methods to handle different treatment of
    #         some statements in this context.
    #     """
    #     typ = type(n)
    #     if add_indent: self.indent_level += 2
    #     indent = self._make_indent()
    #     if add_indent: self.indent_level -= 2
    #
    #     if typ in (
    #             c_ast.Decl, c_ast.Assignment, c_ast.Cast, c_ast.UnaryOp,
    #             c_ast.BinaryOp, c_ast.TernaryOp, c_ast.FuncCall, c_ast.ArrayRef,
    #             c_ast.StructRef, c_ast.Constant, c_ast.ID, c_ast.Typedef,
    #             c_ast.ExprList):
    #         # These can also appear in an expression context so no semicolon
    #         # is added to them automatically
    #         #
    #         return indent + self.visit(n) + ';\n'
    #     elif typ in (c_ast.Compound,):
    #         # No extra indentation required before the opening brace of a
    #         # compound - because it consists of multiple lines it has to
    #         # compute its own indentation.
    #         #
    #         return self.visit(n)
    #     else:
    #         return indent + self.visit(n) + '\n'
    #
    # def _generate_decl(self, n):
    #     """ Generation from a Decl node.
    #     """
    #     s = ''
    #     if n.funcspec: s = ' '.join(n.funcspec) + ' '
    #     if n.storage: s += ' '.join(n.storage) + ' '
    #     s += self._generate_type(n.type)
    #     return s
    #
    # def _generate_type(self, n, modifiers=[]):
    #     """ Recursive generation from a type node. n is the type node.
    #         modifiers collects the PtrDecl, ArrayDecl and FuncDecl modifiers
    #         encountered on the way down to a TypeDecl, to allow proper
    #         generation from it.
    #     """
    #     typ = type(n)
    #     #~ print(n, modifiers)
    #
    #     if typ == c_ast.TypeDecl:
    #         s = ''
    #         if n.quals: s += ' '.join(n.quals) + ' '
    #         s += self.visit(n.type)
    #
    #         nstr = n.declname if n.declname else ''
    #         # Resolve modifiers.
    #         # Wrap in parens to distinguish pointer to array and pointer to
    #         # function syntax.
    #         #
    #         for i, modifier in enumerate(modifiers):
    #             if isinstance(modifier, c_ast.ArrayDecl):
    #                 if (i != 0 and isinstance(modifiers[i - 1], c_ast.PtrDecl)):
    #                     nstr = '(' + nstr + ')'
    #                 nstr += '[' + self.visit(modifier.dim) + ']'
    #             elif isinstance(modifier, c_ast.FuncDecl):
    #                 if (i != 0 and isinstance(modifiers[i - 1], c_ast.PtrDecl)):
    #                     nstr = '(' + nstr + ')'
    #                 nstr += '(' + self.visit(modifier.args) + ')'
    #             elif isinstance(modifier, c_ast.PtrDecl):
    #                 if modifier.quals:
    #                     nstr = '* %s %s' % (' '.join(modifier.quals), nstr)
    #                 else:
    #                     nstr = '*' + nstr
    #         if nstr: s += ' ' + nstr
    #         return s
    #     elif typ == c_ast.Decl:
    #         return self._generate_decl(n.type)
    #     elif typ == c_ast.Typename:
    #         return self._generate_type(n.type)
    #     elif typ == c_ast.IdentifierType:
    #         return ' '.join(n.names) + ' '
    #     elif typ in (c_ast.ArrayDecl, c_ast.PtrDecl, c_ast.FuncDecl):
    #         return self._generate_type(n.type, modifiers + [n])
    #     else:
    #         return self.visit(n)
    #
    # def _parenthesize_if(self, n, condition):
    #     """ Visits 'n' and returns its string representation, parenthesized
    #         if the condition function applied to the node returns True.
    #     """
    #     s = self._visit_expr(n)
    #     if condition(n):
    #         return '(' + s + ')'
    #     else:
    #         return s
    #
    # def _parenthesize_unless_simple(self, n):
    #     """ Common use case for _parenthesize_if
    #     """
    #     return self._parenthesize_if(n, lambda d: not self._is_simple_node(d))
    #
    # def _is_simple_node(self, n):
    #     """ Returns True for nodes that are "simple" - i.e. nodes that always
    #         have higher precedence than operators.
    #     """
    #     return isinstance(n, (c_ast.Constant, c_ast.ID, c_ast.ArrayRef,
    #                           c_ast.StructRef, c_ast.FuncCall))
    #
