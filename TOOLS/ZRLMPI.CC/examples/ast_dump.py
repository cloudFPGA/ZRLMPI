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
from pprint import pprint


INDENT = 4
K = clang.cindex.CursorKind

def is_std_ns(node):
    return node.kind == K.NAMESPACE and node.spelling == 'std'


def vit(node: clang.cindex.Cursor, indent: int, saw):
    pre = ' ' * indent
    k = node.kind  # type: clang.cindex.CursorKind
    # skip printting UNEXPOSED_*
    if not k.is_unexposed():
        print(pre, end='')
        print(k.name, end=' ')
        if node.spelling:
            print('s:', node.spelling, end=' ')
            if node.type.spelling:
                print('t:', node.type.spelling, end=' ')
            # FIXME: print opcode or literal
        print()
    saw.add(node.hash)
    if node.referenced is not None and node.referenced.hash not in saw:
        vit(node.referenced, indent + INDENT, saw)
    # FIXME: skip auto generated decls
    skip = len([c for c in node.get_children()
                if indent == 0 and is_std_ns(c)])
    for c in node.get_children():
        if not skip:
            vit(c, indent + INDENT, saw)
        if indent == 0 and is_std_ns(c):
            skip -= 1
    saw.remove(node.hash)


def main():
    index = clang.cindex.Index.create()
    tu = index.parse(sys.argv[1])
    vit(tu.cursor, 0, set())
    # pprint("\n\n nodes", tu.cursor)

main()

