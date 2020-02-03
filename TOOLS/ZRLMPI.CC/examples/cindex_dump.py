
#===- cindex-dump.py - cindex/Python Source Dump -------------*- python -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

"""
A simple command line tool for dumping a source file using the Clang Index
Library.
"""
# TODO: important!
# environment MUST BE
# LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:/usr/local/lib64:/usr/lib64:/usr/local/lib/
# PYTHONPATH=/home/ngl/gitrepos/cloudFPGA/hccs_tvm//python:/home/ngl/gitrepos/cloudFPGA/hccs_tvm//topi/python:/home/ngl/gitrepos/cloudFPGA/hccs_tvm//nnvm/python:/home/ngl/gitrepos/cloudFPGA/hccs_tvm//vta/python:/home/ngl/gitrepos/llvm-project/clang/bindings/python:

def get_diag_info(diag):
    return { 'severity' : diag.severity,
             'location' : diag.location,
             'spelling' : diag.spelling,
             'ranges' : diag.ranges,
             'fixits' : diag.fixits }

def get_cursor_id(cursor, cursor_list = []):
    if not opts.showIDs:
        return None

    if cursor is None:
        return None

    # FIXME: This is really slow. It would be nice if the index API exposed
    # something that let us hash cursors.
    for i,c in enumerate(cursor_list):
        if cursor == c:
            return i
    cursor_list.append(cursor)
    return len(cursor_list) - 1

def get_info(node, depth=0):
    if "app_hw" not in str(node.extent.start.file):
        children = None
    elif opts.maxDepth is not None and depth >= opts.maxDepth:
        children = None
    else:
        children = [get_info(c, depth+1)
                    for c in node.get_children()]
    return { 'id' : get_cursor_id(node),
             'kind' : node.kind,
             'usr' : node.get_usr(),
             'spelling' : node.spelling,
             'location' : node.location,
             'extent.start' : node.extent.start,
             'extent.end' : node.extent.end,
             'is_definition' : node.is_definition(),
             'definition id' : get_cursor_id(node.get_definition()),
             'children' : children }

def main():
    from clang.cindex import Index
    from pprint import pprint

    from optparse import OptionParser, OptionGroup

    global opts

    parser = OptionParser("usage: %prog [options] {filename} [clang-args*]")
    parser.add_option("", "--show-ids", dest="showIDs",
                      help="Compute cursor IDs (very slow)",
                      action="store_true", default=False)
    parser.add_option("", "--max-depth", dest="maxDepth",
                      help="Limit cursor expansion to depth N",
                      metavar="N", type=int, default=None)
    parser.disable_interspersed_args()
    (opts, args) = parser.parse_args()

    if len(args) == 0:
        parser.error('invalid number arguments')

    index = Index.create()
    args2 = []
    args2.append('I/home/ngl/gitrepos/cloudFPGA/ZRLMPI/LIB/HW/hls/mpi_wrapperv1/src')
    args2.append('I/opt/Xilinx/Vivado/2017.4/include/')
    # tu = index.parse(None, args, options=0)
    tu = index.parse(args[0], args2, options=0)
    if not tu:
        parser.error("unable to load input")

    # pprint(('diags', map(get_diag_info, tu.diagnostics)))

    pprint(('nodes', get_info(tu.cursor)))

    # c_code = ""
    # newline_trigger = []
    # newline_trigger.append(';')
    # newline_trigger.append('#')
    # for t in tu.get_tokens(extent=tu.cursor.extent):
    #     #print(t.kind)
    #     if "COMMENT" in str(t.kind):
    #         continue
    #     c_code += t.spelling
    #     #if str(t.kind) == 'TokenKind.IDENTIFIER':
    #     #    c_code += ' '
    #     #for tr in newline_trigger:
    #     #    if tr in t.spelling:
    #     #        c_code += '\n'
    #     #        break

    # print(c_code)

if __name__ == '__main__':
    main()
