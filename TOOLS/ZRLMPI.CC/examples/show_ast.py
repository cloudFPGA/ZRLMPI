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

import sys
import clang.cindex

def verbose(*args, **kwargs):
    '''filter predicate for show_ast: show all'''
    return True
def no_system_includes(cursor, level):
    '''filter predicate for show_ast: filter out verbose stuff from system include files'''
    return (level!= 1) or (cursor.location.file is not None and not cursor.location.file.name.startswith('/usr/include'))

# A function show(level, *args) would have been simpler but less fun
# and you'd need a separate parameter for the AST walkers if you want it to be exchangeable.
class Level(int):
    '''represent currently visited level of a tree'''
    def show(self, *args):
        '''pretty print an indented line'''
        print('\t'*self + ' '.join(map(str, args)))
    def __add__(self, inc):
        '''increase level'''
        return Level(super(Level, self).__add__(inc))

def is_valid_type(t):
    '''used to check if a cursor has a type'''
    return t.kind != clang.cindex.TypeKind.INVALID

def qualifiers(t):
    '''set of qualifiers of a type'''
    q = set()
    if t.is_const_qualified(): q.add('const')
    if t.is_volatile_qualified(): q.add('volatile')
    if t.is_restrict_qualified(): q.add('restrict')
    return q

def show_type(t, level, title):
    '''pretty print type AST'''
    level.show(title, str(t.kind), ' '.join(qualifiers(t)))
    if is_valid_type(t.get_pointee()):
        show_type(t.get_pointee(), level+1, 'points to:')

def show_ast(cursor, filter_pred=verbose, level=Level()):
    '''pretty print cursor AST'''
    if filter_pred(cursor, level):
        level.show(cursor.kind, cursor.spelling, cursor.displayname, cursor.location)
        if is_valid_type(cursor.type):
            show_type(cursor.type, level+1, 'type:')
            show_type(cursor.type.get_canonical(), level+1, 'canonical type:')
        for c in cursor.get_children():
            show_ast(c, filter_pred, level+1)

if __name__ == '__main__':
    clang.cindex.Config.set_library_path('/usr/lib64/clang-private/')
    index = clang.cindex.Index.create()
    args2 = []
    args2.append("-I/home/ngl/gitrepos/cloudFPGA/ZRLMPI/LIB/HW/hls/mpi_wrapperv1/src")
    args2.append("-I/opt/Xilinx/Vivado/2017.4/include")
    #args2.append("-fsyntax-only")
    #tu = index.parse(sys.argv[1], args=args2, options=0)
    tu = index.read(sys.argv[1])
    print('Translation unit:', tu.spelling)
    for f in tu.get_includes():
        print('\t'*f.depth, f.include.name)
    show_ast(tu.cursor, no_system_includes)

