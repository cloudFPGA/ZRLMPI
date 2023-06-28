#!/usr/bin/env python

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

""" Usage: call with <filename> <typename>
"""

import sys

sys.path.append("path/to/llvm-project/clang/bindings/python")
import clang.cindex


def find_typerefs(node, typename):
    """ Find all references to the type named 'typename'
    """
    if node.kind.is_reference():
        ref_node = clang.cindex.Cursor_ref(node)
        if ref_node.spelling == typename:
            print('Found %s [line=%s, col=%s]' % (
                typename, node.location.line, node.location.column))
    # Recurse for children of this node
    for c in node.get_children():
        find_typerefs(c, typename)


index = clang.cindex.Index.create()
tu = index.parse(sys.argv[1])
print('Translation unit:', tu.spelling)
find_typerefs(tu.cursor, sys.argv[2])

