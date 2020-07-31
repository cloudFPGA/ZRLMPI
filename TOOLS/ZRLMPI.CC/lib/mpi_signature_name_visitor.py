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

__mpi_api_signatures_buffers__ = ['MPI_Send', 'MPI_Recv']
__mpi_api_signatures_rank__ = ['MPI_Comm_rank']
__mpi_api_signatures_size__ = ['MPI_Comm_size']
__mpi_api_signatures_scatter__ = ['MPI_Scatter']
__mpi_api_signatures_gather__ = ['MPI_Gather']


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

    def __init__(self):
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
    #     arrref = self._parenthesize_unless_simple(n.name)
    #     return arrref + '[' + self.visit(n.subscript) + ']'

    # def visit_StructRef(self, n):
    #     sref = self._parenthesize_unless_simple(n.name)
    #     return sref + n.type + self.visit(n.field)

    def visit_FuncCall(self, n):
        func_name = n.name.name
        # print("visiting FuncCall {}\n".format(func_name))
        if func_name in __mpi_api_signatures_buffers__:
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
        return

    # def visit_UnaryOp(self, n):
    #     operand = self._parenthesize_unless_simple(n.expr)
    #     if n.op == 'p++':
    #         return '%s++' % operand
    #     elif n.op == 'p--':
    #         return '%s--' % operand
    #     elif n.op == 'sizeof':
    #         # Always parenthesize the argument of sizeof since it can be
    #         # a name.
    #         return 'sizeof(%s)' % self.visit(n.expr)
    #     else:
    #         return '%s%s' % (n.op, operand)
    #
    # def visit_BinaryOp(self, n):
    #     lval_str = self._parenthesize_if(n.left,
    #                                      lambda d: not self._is_simple_node(d))
    #     rval_str = self._parenthesize_if(n.right,
    #                                      lambda d: not self._is_simple_node(d))
    #     return '%s %s %s' % (lval_str, n.op, rval_str)
    #
    # def visit_Assignment(self, n):
    #     rval_str = self._parenthesize_if(
    #         n.rvalue,
    #         lambda n: isinstance(n, c_ast.Assignment))
    #     return '%s %s %s' % (self.visit(n.lvalue), n.op, rval_str)
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
    #
    # def visit_Decl(self, n, no_type=False):
    #     # no_type is used when a Decl is part of a DeclList, where the type is
    #     # explicitly only for the first declaration in a list.
    #     #
    #     s = n.name if no_type else self._generate_decl(n)
    #     if n.bitsize: s += ' : ' + self.visit(n.bitsize)
    #     if n.init:
    #         s += ' = ' + self._visit_expr(n.init)
    #     return s
    #
    # def visit_DeclList(self, n):
    #     s = self.visit(n.decls[0])
    #     if len(n.decls) > 1:
    #         s += ', ' + ', '.join(self.visit_Decl(decl, no_type=True)
    #                               for decl in n.decls[1:])
    #     return s
    #
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
    # def visit_TernaryOp(self, n):
    #     s  = '(' + self._visit_expr(n.cond) + ') ? '
    #     s += '(' + self._visit_expr(n.iftrue) + ') : '
    #     s += '(' + self._visit_expr(n.iffalse) + ')'
    #     return s
    #
    # def visit_If(self, n):
    #     s = 'if ('
    #     if n.cond: s += self.visit(n.cond)
    #     s += ')\n'
    #     s += self._generate_stmt(n.iftrue, add_indent=True)
    #     if n.iffalse:
    #         s += self._make_indent() + 'else\n'
    #         s += self._generate_stmt(n.iffalse, add_indent=True)
    #     return s
    #
    # def visit_For(self, n):
    #     s = 'for ('
    #     if n.init: s += self.visit(n.init)
    #     s += ';'
    #     if n.cond: s += ' ' + self.visit(n.cond)
    #     s += ';'
    #     if n.next: s += ' ' + self.visit(n.next)
    #     s += ')\n'
    #     s += self._generate_stmt(n.stmt, add_indent=True)
    #     return s
    #
    # def visit_While(self, n):
    #     s = 'while ('
    #     if n.cond: s += self.visit(n.cond)
    #     s += ')\n'
    #     s += self._generate_stmt(n.stmt, add_indent=True)
    #     return s
    #
    # def visit_DoWhile(self, n):
    #     s = 'do\n'
    #     s += self._generate_stmt(n.stmt, add_indent=True)
    #     s += self._make_indent() + 'while ('
    #     if n.cond: s += self.visit(n.cond)
    #     s += ');'
    #     return s
    #
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

