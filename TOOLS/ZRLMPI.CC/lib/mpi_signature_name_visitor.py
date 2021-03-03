#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Jan 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        Visitor searching for all buffers that are used in context of MPI calls.
#  *
#  *

# based on
#   https://github.com/eliben/pycparser/blob/master/pycparser/c_generator.py
#   and https://github.com/eliben/pycparser/blob/master/pycparser/c_ast.py


from pycparser import c_ast

__mpi_api_signatures_send__ = ['MPI_Send']
__mpi_api_signatures_recv__ = ['MPI_Recv']
# __mpi_api_signatures_buffers__ = ['MPI_Send', 'MPI_Recv']
__mpi_api_signatures_buffers__ = []
__mpi_api_signatures_buffers__.extend(__mpi_api_signatures_send__)
__mpi_api_signatures_buffers__.extend(__mpi_api_signatures_recv__)
__mpi_api_signatures_rank__ = ['MPI_Comm_rank']
__mpi_api_signatures_size__ = ['MPI_Comm_size']
__mpi_api_signatures_scatter__ = ['MPI_Scatter']
__mpi_api_signatures_gather__ = ['MPI_Gather']
__mpi_api_signatures_bcast__ = ['MPI_Bcast']
__mpi_api_signatres_reduce__ = ['MPI_Reduce']

__c_api_malloc_signatures__ = ['malloc']
__c_api_clib_signatures__ = ['free', 'perror', 'exit', 'memcopy']

__dram_func_name_extension__ = "_DRAM"

__null_constant__ = c_ast.Constant(type='int', value='0')

__buffer_write_ident__ = 'w'
__buffer_read_ident__ = 'r'


def _get_id_name_of_arg(arg_0, with_binary_op=True):
    id_name = ''
    obj_to_visit = [arg_0]
    while len(obj_to_visit) > 0:
        current_obj = obj_to_visit[0]
        del obj_to_visit[0]
        if type(current_obj) == c_ast.ID:
            id_name = current_obj.name
            break
        elif type(current_obj) == c_ast.BinaryOp and with_binary_op:
            obj_to_visit.append(current_obj.left)
            obj_to_visit.append(current_obj.right)
        elif type(current_obj) == c_ast.UnaryOp:
            el = current_obj.expr
            if type(el) is list:
                obj_to_visit.append(el[0])
            else:
                obj_to_visit.append(el)
        elif type(current_obj) == c_ast.ArrayRef:
            obj_to_visit.append(current_obj.name)
    return id_name


def _merge_access_list(access_list):
    covered_read_access = []
    covered_write_access = []
    final_access_list = []
    names_covered_read = []
    names_covered_write = []
    names_covered_both = []
    for e in access_list:
        en = e['name']
        ew = e['write']
        er = e['read']
        if ew and er:
            if en in names_covered_both:
                # nothing to do
                continue
            if en in names_covered_read:
                ti = names_covered_read.index(en)
                del covered_read_access[ti]
                del names_covered_read[ti]
            if en in names_covered_write:
                ti = names_covered_write.index(en)
                del covered_write_access[ti]
                del names_covered_write[ti]
            final_access_list.append(e)
            names_covered_both.append(en)
        elif ew:
            if en not in names_covered_both \
                    and en in names_covered_read:
                new_entry = {}
                new_entry['name'] = en
                new_entry['write'] = True
                new_entry['read'] = True
                ti = names_covered_read.index(en)
                del names_covered_read[ti]
                del covered_read_access[ti]
                names_covered_both.append(en)
                final_access_list.append(en)
            elif en not in names_covered_both \
                    and en not in names_covered_write:
                covered_write_access.append(e)
                names_covered_write.append(en)
        else:
            if en not in names_covered_both \
                    and en in names_covered_write:
                new_entry = {}
                new_entry['name'] = en
                new_entry['write'] = True
                new_entry['read'] = True
                ti = names_covered_write.index(en)
                del names_covered_write[ti]
                del covered_write_access[ti]
                names_covered_both.append(en)
                final_access_list.append(en)
            if en not in names_covered_both \
                    and en not in names_covered_read:
                covered_read_access.append(e)
                names_covered_read.append(en)
    # after for each
    final_access_list.extend(covered_write_access)
    final_access_list.extend(covered_read_access)
    return final_access_list


class MpiSignatureNameSearcher(object):
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

    def __init__(self, search_for_decls=None, seach_for_dram=None, search_for_loop=None, replace_names=None):
        # Statements start with indentation of self.indent_level spaces, using
        # the _make_indent method
        #
        self.found_buffers_obj = []
        self.found_buffers_names = []
        self.found_rank_obj = []
        self.found_rank_names = []
        self.found_size_obj = []
        self.found_size_names = []
        self.found_scatter_obj = []
        self.found_gather_obj = []
        self.found_bcast_obj = []
        self.found_reduce_obj = []
        self.found_send_obj = []
        self.found_recv_obj = []
        self.found_malloc_obj = []
        self.found_clib_obj = []
        self.search_for_decls = search_for_decls
        self.found_decl_obj = []
        self.found_if_obj = []
        self.search_for_dram = seach_for_dram
        self.found_dram_calls = []
        self.search_for_loop = None
        self.dram_buffer_replace_names = None
        if search_for_loop is not None and seach_for_dram is not None and replace_names is not None:
            self.search_for_loop = search_for_loop
            self.dram_buffer_replace_names = replace_names
        self.found_inner_loops = []
        self.is_loop = False
        self.current_inner_loop = None
        self.buffer_access_in_loop = []
        self.tmp_buffer_access_list = []
        self.loop_buffer_calls_to_replace = []
        self.tmp_buffer_calls_to_replace = []

    def get_results_buffers(self):
        return self.found_buffers_names, self.found_buffers_obj

    def get_results_ranks(self):
        return self.found_rank_names, self.found_rank_obj

    def get_results_sizes(self):
        return self.found_size_names, self.found_size_obj

    def get_results_scatter(self):
        return self.found_scatter_obj

    def get_results_gather(self):
        return self.found_gather_obj

    def get_results_bcast(self):
        return self.found_bcast_obj

    def get_results_reduce(self):
        return self.found_reduce_obj

    def get_results_send(self):
        return self.found_send_obj

    def get_results_recv(self):
        return self.found_recv_obj

    def get_results_malloc(self):
        ret = []
        for e in self.found_malloc_obj:
            if type(e) != c_ast.Cast:
                ret.append(e)
        return ret

    def get_results_clib(self):
        return self.found_clib_obj

    def get_results_dcls(self):
        return self.found_decl_obj

    def get_results_if_obj(self):
        return self.found_if_obj

    def get_found_dram_calls(self):
        return self.found_dram_calls

    def get_found_inner_loops(self):
        return self.found_inner_loops

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
    #     if self.search_for_loop is not None and self.is_loop:
    #         id_name = _get_id_name_of_arg(n.name, with_binary_op=False)
    #         if id_name != '' and id_name in self.search_for_dram:
    #             new_entry = {}
    #             new_entry['name'] = id_name
    #             # could be both, we don't know
    #             # TODO: evaluate potential "const" etc.
    #             new_entry['write'] = True
    #             new_entry['read'] = True
    #             self.tmp_buffer_access_list.append(new_entry)
    #     else:
    #         for c in n:
    #             self.visit(c)

    # def visit_StructRef(self, n):
    #     if self.search_for_loop is not None and self.is_loop:
    #         id_name = _get_id_name_of_arg(n.name, with_binary_op=False)
    #         if id_name != '' and id_name in self.search_for_dram:
    #             new_entry = {}
    #             new_entry['name'] = id_name
    #             # could be both, we don't know
    #             new_entry['write'] = True
    #             new_entry['read'] = True
    #             self.tmp_buffer_access_list.append(new_entry)
    #     else:
    #         for c in n:
    #             self.visit(c)

    def visit_FuncCall(self, n):
        func_name = n.name.name
        # print("visiting FuncCall {}\n".format(func_name))
        if func_name in __mpi_api_signatures_buffers__:
            # fist, save obj
            if func_name in __mpi_api_signatures_send__:
                self.found_send_obj.append(n)
            elif func_name in __mpi_api_signatures_recv__:
                self.found_recv_obj.append(n)
            # it's always the first argument
            arg_0 = n.args.exprs[0]
            # print("found 1st arg: {}\n".format(str(arg_0)))
            buffer_name = ""
            current_obj = arg_0
            while True:
                if hasattr(current_obj, 'name'):
                    if type(current_obj.name) is str:
                        buffer_name = current_obj.name
                        break
                    current_obj = current_obj.name
                elif hasattr(current_obj, 'expr'):
                    current_obj = current_obj.expr
                elif hasattr(current_obj, 'stmt'):
                    current_obj = current_obj.stmt
                else:
                    break
            # print("found buffer name {}\n".format(buffer_name))
            if buffer_name not in self.found_buffers_names:
                self.found_buffers_names.append(buffer_name)
                self.found_buffers_obj.append(arg_0)
            if self.search_for_dram is not None:
                id_name = _get_id_name_of_arg(arg_0)
                if id_name == '':
                    print("[DEBUG] unable to find buffer name of MPI call.")
                else:
                    if id_name in self.search_for_dram:
                        new_entry = {}
                        new_entry['old'] = n
                        new_entry['new'] = c_ast.FuncCall(c_ast.ID(func_name+__dram_func_name_extension__), n.args)
                        self.found_dram_calls.append(new_entry)
        elif func_name in __mpi_api_signatures_rank__:
            # it's always the second argument
            arg_1 = n.args.exprs[1]
            # print("found 2nd arg: {}\n".format(str(arg_1)))
            rank_name = ""
            current_obj = arg_1
            while True:
                if hasattr(current_obj, 'name'):
                    rank_name = current_obj.name
                    break
                elif hasattr(current_obj, 'expr'):
                    current_obj = current_obj.expr
                elif hasattr(current_obj, 'stmt'):
                    current_obj = current_obj.stmt
                else:
                    break
            # print("found rank name {}\n".format(rank_name))
            if rank_name not in self.found_rank_names:
                self.found_rank_names.append(rank_name)
                self.found_rank_obj.append(arg_1)
        elif func_name in __mpi_api_signatures_size__:
            # it's always the second argument
            arg_1 = n.args.exprs[1]
            # print("found 2nd arg: {}\n".format(str(arg_1)))
            size_name = ""
            current_obj = arg_1
            while True:
                if hasattr(current_obj, 'name'):
                    size_name = current_obj.name
                    break
                elif hasattr(current_obj, 'expr'):
                    current_obj = current_obj.expr
                elif hasattr(current_obj, 'stmt'):
                    current_obj = current_obj.stmt
                else:
                    break
            # print("found rank name {}\n".format(rank_name))
            if size_name not in self.found_size_names:
                self.found_size_names.append(size_name)
                #self.found_size_obj.append(arg_1)
                self.found_size_obj.append(n)
        elif func_name in __mpi_api_signatures_scatter__:
                self.found_scatter_obj.append(n)
        elif func_name in __mpi_api_signatures_gather__:
            self.found_gather_obj.append(n)
        elif func_name in __mpi_api_signatures_bcast__:
            self.found_bcast_obj.append(n)
        elif func_name in __mpi_api_signatres_reduce__:
            self.found_reduce_obj.append(n)
        elif func_name in __c_api_malloc_signatures__:
            # TODO: add current func_name from context stack as done in constant folding visitor
            # the placement is maybe not always 'app_main'
            self.found_malloc_obj.append(n)
        elif func_name in __c_api_clib_signatures__:
            self.found_clib_obj.append(n)

        if self.search_for_loop is not None and self.is_loop:
            replace_args = False
            new_args = []
            for a in n.args.exprs:
                # self.visit(a)
                found_local = False
                if type(a) is c_ast.ID or type(a) is c_ast.ArrayRef \
                        or (type(a) is c_ast.UnaryOp and (a.op == '&' or a.op == '*')):
                    id_name = _get_id_name_of_arg(a, with_binary_op=False)
                    if id_name != '':
                        if id_name in self.search_for_dram:
                            new_entry = {}
                            new_entry['name'] = id_name
                            # pointer will be covered somewhere else --> false, we don't know it here
                            new_entry['write'] = True
                            new_entry['read'] = True
                            self.tmp_buffer_access_list.append(new_entry)
                            ni = self.search_for_dram.index(id_name)
                            found_local = True
                            replace_args = True
                            if type(a) is c_ast.ID:
                                nn = c_ast.ID(name=self.dram_buffer_replace_names[ni])
                                new_args.append(nn)
                            elif type(a) is c_ast.ArrayRef:
                                nn = c_ast.ArrayRef(name=self.dram_buffer_replace_names[ni], subscript=a.subscript)
                                new_args.append(nn)
                            elif type(a) is c_ast.UnaryOp:
                                nn = c_ast.UnaryOp(a.op, expr=self.dram_buffer_replace_names[ni])
                                new_args.append(nn)
                if not found_local:
                    new_args.append(a)
            if replace_args:
                expr_list = c_ast.ExprList(new_args)
                new_node = c_ast.FuncCall(n.name, expr_list)
                new_entry = {'old': n, 'new': new_node}
                self.tmp_buffer_calls_to_replace.append(new_entry)
        return

    def visit_UnaryOp(self, n):
        if self.search_for_loop is not None and self.is_loop:
            id_name = _get_id_name_of_arg(n.expr, with_binary_op=False)
            if id_name == '':
                print("[DEBUG] unable to find buffer of UnaryOp in loop.")
            else:
                if id_name in self.search_for_dram:
                    new_entry = {}
                    new_entry['name'] = id_name
                    # could be pointer --> both
                    new_entry['write'] = True
                    new_entry['read'] = True
                    self.tmp_buffer_access_list.append(new_entry)
        else:
            for c in n:
                self.visit(c)

    def visit_BinaryOp(self, n):
        if self.search_for_loop is not None and self.is_loop:
            # self.visit(n.op)
            new_r = None
            new_l = None
            if type(n.left) is c_ast.ID:
                id_name = _get_id_name_of_arg(n.left, with_binary_op=False)
                if id_name == '':
                    print("[DEBUG] unable to find buffer of BinaryOp in loop.")
                else:
                    if id_name in self.search_for_dram:
                        new_entry = {}
                        new_entry['name'] = id_name
                        new_entry['write'] = False
                        new_entry['read'] = True
                        self.tmp_buffer_access_list.append(new_entry)
                        ni = self.search_for_dram.index(id_name)
                        new_l = c_ast.ID(name=self.dram_buffer_replace_names[ni])
            else:
                self.visit(n.left)
            if type(n.right) is c_ast.ID:
                id_name = _get_id_name_of_arg(n.right, with_binary_op=False)
                if id_name == '':
                    print("[DEBUG] unable to find buffer of BinaryOp in loop.")
                else:
                    if id_name in self.search_for_dram:
                        new_entry = {}
                        new_entry['name'] = id_name
                        new_entry['write'] = False
                        new_entry['read'] = True
                        self.tmp_buffer_access_list.append(new_entry)
                        ni = self.search_for_dram.index(id_name)
                        new_r = c_ast.ID(name=self.dram_buffer_replace_names[ni])
            if new_l is not None or new_r is not None:
                new_entry = {}
                new_entry['old'] = n
                nr = n.right
                nl = n.left
                if new_r is not None:
                    nr = new_r
                if new_l is not None:
                    nl = new_l
                new_entry['new'] = c_ast.BinaryOp(n.op,nl,nr)
                self.tmp_buffer_access_list.append(new_entry)
            else:
                self.visit(n.right)
        else:
            for c in n:
                self.visit(c)

    def visit_Assignment(self, n):
        if self.search_for_loop is not None and self.is_loop and n.op == '=':
            access_list = []
            if type(n.lvalue) is c_ast.ID or type(n.lvalue) is c_ast.ArrayRef:
                id_name = _get_id_name_of_arg(n.lvalue)
                if id_name == '':
                    print("[DEBUG] unable to find buffer name of assignment in loop.")
                else:
                    if id_name in self.search_for_dram:
                        new_entry = {}
                        new_entry['name'] = id_name
                        # new_entry['access'] = __buffer_write_ident__
                        new_entry['write'] = True
                        new_entry['read'] = False
                        access_list.append(new_entry)
                        ni = self.search_for_dram.index(id_name)
                        new_entry = {}
                        new_entry['old'] = n
                        new_l = None
                        if type(n.lvalue) is c_ast.ID:
                            new_l = c_ast.ID(name=self.dram_buffer_replace_names[ni])
                        elif type(n.lvalue) is c_ast.ArrayRef:
                            new_l = c_ast.ArrayRef(name=self.dram_buffer_replace_names[ni], subscript=n.lvalue.subscript)
                        if new_l is not None:
                            new_entry['new'] = c_ast.Assignment(n.op, new_l, n.rvalue)
                            self.tmp_buffer_calls_to_replace.append(new_entry)
            nested = True
            prev_access_list = self.tmp_buffer_access_list
            if len(self.tmp_buffer_access_list) == 0:
                nested = False
            self.tmp_buffer_access_list = []
            self.visit(n.rvalue)
            if nested:
                self.tmp_buffer_access_list.extend(prev_access_list)
                self.tmp_buffer_access_list.extend(access_list)
                # done for now
            elif len(self.tmp_buffer_access_list) > 0:
                self.tmp_buffer_access_list.extend(access_list)
                final_access_list = _merge_access_list(self.tmp_buffer_access_list)
                self.buffer_access_in_loop.extend(final_access_list)
                self.tmp_buffer_access_list = []
        else:
            for c in n:
                self.visit(c)
    #
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

    def visit_Decl(self, n, no_type=False):
        if self.search_for_decls is not None:
            if n.name in self.search_for_decls:
                self.found_decl_obj.append(n)
        if self.search_for_loop is not None and self.is_loop and n.init is not None:
            nested = True
            prev_access_list = self.tmp_buffer_access_list
            if len(self.tmp_buffer_access_list) == 0:
                nested = False
            self.tmp_buffer_access_list = []
            self.visit(n.init)
            if nested:
                self.tmp_buffer_access_list.extend(prev_access_list)
                # done for now
            elif len(self.tmp_buffer_access_list) > 0:
                final_access_list = _merge_access_list(self.tmp_buffer_access_list)
                self.buffer_access_in_loop.extend(final_access_list)
                self.tmp_buffer_access_list = []
        else:
            for c in n:
                self.visit(c)

    # def visit_DeclList(self, n):
    #     s = self.visit(n.decls[0])
    #     if len(n.decls) > 1:
    #         s += ', ' + ', '.join(self.visit_Decl(decl, no_type=True)
    #                               for decl in n.decls[1:])
    #     return s
    #
    # def visit_Typedef(self, n):
    #     for c in n:
    #         self.visit(c)
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
    #
    # def visit_Compound(self, n):
    #     s = self._make_indent() + '{\n'
    #     self.indent_level += 2
    #     if n.block_items:
    #         s += ''.join(self._generate_stmt(stmt) for stmt in n.block_items)
    #     self.indent_level -= 2
    #     s += self._make_indent() + '}\n'
    #     return s
    #
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
    #
    def visit_TernaryOp(self, n):
        if self.search_for_decls is not None:
            self.visit_If(n)
        else:
            for c in n:
                self.visit(c)

    def visit_If(self, n):
        # searching for (variable == NULL) in case of malloc
        if self.search_for_decls is not None:
            if type(n.cond) == c_ast.BinaryOp:
                if type(n.cond.left) == c_ast.ID and \
                        type(n.cond.right) == c_ast.Constant and n.cond.right.type == __null_constant__.type and \
                        n.cond.right.value == __null_constant__.value:
                    if n.cond.left.name in self.search_for_decls:
                        node_to_add = n
                        if n.cond.op == "!=":
                            # switch to expected format
                            node_to_add.iftrue = n.iffalse
                            node_to_add.iffales = n.iftrue
                        self.found_if_obj.append(node_to_add)
                elif type(n.cond.right) == c_ast.ID and \
                        type(n.cond.left) == c_ast.Constant and n.cond.left.type == __null_constant__.type and \
                        n.cond.left.value == __null_constant__.value:
                    if n.cond.right.name in self.search_for_decls:
                        node_to_add = n
                        if n.cond.op == "!=":
                            # switch to expected format
                            node_to_add.iftrue = n.iffalse
                            node_to_add.iffales = n.iftrue
                        self.found_if_obj.append(node_to_add)
                else:
                    for c in n:
                        self.visit(c)
        else:
            for c in n:
                self.visit(c)

    def visit_For(self, n):
        if self.search_for_loop is not None:
            prev_loop = self.is_loop
            self.is_loop = False  # for init etc.
            self.visit(n.init)
            self.visit(n.cond)
            self.visit(n.next)
            self.is_loop = True
            self.current_inner_loop = n
            prev_buffer_access = self.buffer_access_in_loop
            prev_buffer_calls = self.tmp_buffer_calls_to_replace
            self.buffer_access_in_loop = []
            self.tmp_buffer_calls_to_replace = []
            prev_tmp_access = self.tmp_buffer_access_list
            self.tmp_buffer_access_list = []
            self.visit(n.stmt)
            if self.current_inner_loop == n and len(self.buffer_access_in_loop) > 0:
                # I'm a inner loop
                new_entry = {}
                new_entry['loop'] = n
                new_entry['buffers'] = self.buffer_access_in_loop
                new_entry['replace'] = self.tmp_buffer_calls_to_replace
                self.found_inner_loops.append(new_entry)
            self.is_loop = prev_loop
            self.buffer_access_in_loop = prev_buffer_access
            self.tmp_buffer_access_list = prev_tmp_access
            self.tmp_buffer_calls_to_replace = prev_buffer_calls
        else:
            for c in n:
                self.visit(c)

    def visit_While(self, n):
        if self.search_for_loop is not None:
            prev_loop = self.is_loop
            self.is_loop = False  # for init etc.
            self.visit(n.cond)
            self.is_loop = True
            self.current_inner_loop = n
            prev_buffer_access = self.buffer_access_in_loop
            prev_buffer_calls = self.tmp_buffer_calls_to_replace
            self.tmp_buffer_calls_to_replace = []
            self.buffer_access_in_loop = []
            prev_tmp_access = self.tmp_buffer_access_list
            self.tmp_buffer_access_list = []
            self.visit(n.stmt)
            if self.current_inner_loop == n and len(self.buffer_access_in_loop) > 0:
                # I'm a inner loop
                new_entry = {}
                new_entry['loop'] = n
                new_entry['buffers'] = self.buffer_access_in_loop
                new_entry['replace'] = self.tmp_buffer_calls_to_replace
                self.found_inner_loops.append(new_entry)
            self.is_loop = prev_loop
            self.buffer_access_in_loop = prev_buffer_access
            self.tmp_buffer_access_list = prev_tmp_access
            self.tmp_buffer_calls_to_replace = prev_buffer_calls
        else:
            for c in n:
                self.visit(c)

    def visit_DoWhile(self, n):
        if self.search_for_loop is not None:
            prev_loop = self.is_loop
            self.is_loop = True
            self.current_inner_loop = n
            prev_buffer_access = self.buffer_access_in_loop
            self.buffer_access_in_loop = []
            prev_tmp_access = self.tmp_buffer_access_list
            self.tmp_buffer_access_list = []
            prev_buffer_calls = self.tmp_buffer_calls_to_replace
            self.tmp_buffer_calls_to_replace = []
            self.visit(n.stmt)
            if self.current_inner_loop == n and len(self.buffer_access_in_loop) > 0:
                # I'm a inner loop
                new_entry = {}
                new_entry['loop'] = n
                new_entry['buffers'] = self.buffer_access_in_loop
                new_entry['replace'] = self.tmp_buffer_calls_to_replace
                self.found_inner_loops.append(new_entry)
            # cond after stmt here
            self.is_loop = False  # for init etc.
            self.visit(n.cond)
            self.is_loop = prev_loop
            self.buffer_access_in_loop = prev_buffer_access
            self.tmp_buffer_access_list = prev_tmp_access
            self.tmp_buffer_calls_to_replace = prev_buffer_calls
        else:
            for c in n:
                self.visit(c)

    # def visit_Switch(self, n):
    #     s = 'switch (' + self.visit(n.cond) + ')\n'
    #     s += self._generate_stmt(n.stmt, add_indent=True)
    #     return s
    #
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

