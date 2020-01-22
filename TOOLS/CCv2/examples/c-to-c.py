#------------------------------------------------------------------------------
# pycparser: c-to-c.py
#
# Example of using pycparser.c_generator, serving as a simplistic translator
# from C to AST and back to C.
#
# Eli Bendersky [https://eli.thegreenplace.net/]
# License: BSD
#------------------------------------------------------------------------------
from __future__ import print_function
import sys

# This is not required if you've installed pycparser into
# your site-packages/ with setup.py
#
sys.path.extend(['.', '..'])

from pycparser import parse_file, c_parser, c_generator, c_ast


def translate_to_c(filename):
    """ Simply use the c_generator module to emit a parsed AST.
    """
    ast = parse_file(filename, use_cpp=True)
    for e in ast.ext[-1].body.block_items:
        if type(e) == c_ast.Decl:
            print(e.name)
        elif type(e) == c_ast.FuncCall:
            print(e.name.name)
    generator = c_generator.CGenerator()
    ast_main_function = ast.ext[-1]
    parsed_ast = str(generator.visit(ast))
    split_ast = parsed_ast.split("int main(int argc, char **argv);")

    print(split_ast[-1])




def _zz_test_translate():
    # internal use
    src = r'''

    void f(char * restrict joe){}

int main(void)
{
    unsigned int long k = 4;
    int p = - - k;
    return 0;
}
'''
    parser = c_parser.CParser()
    ast = parser.parse(src)
    ast.show()
    generator = c_generator.CGenerator()

    print(generator.visit(ast))

    # tracing the generator for debugging
    #~ import trace
    #~ tr = trace.Trace(countcallers=1)
    #~ tr.runfunc(generator.visit, ast)
    #~ tr.results().write_results()


#------------------------------------------------------------------------------
if __name__ == "__main__":
    #_zz_test_translate()
    if len(sys.argv) > 1:
        translate_to_c(sys.argv[1])
    else:
        print("Please provide a filename as argument")
