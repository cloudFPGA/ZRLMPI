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
#  *     Created: Sept 2020
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Resource Checker for ZRLMPI on cloudFPGA
#  *
#  *

__cF_resources__ = {'FMKU60': {'name': 'FMKU60', 'Themisto': {}}}
__cF_resources__['FMKU60']['Themisto']['name'] = "Role_Themisto"
__cF_resources__['FMKU60']['Themisto']['BRAM_kB'] = 743  # TODO: import from somewhere?

maximum_bram_buffer_size_bytes = 131072


def check_resources(cFp, buffer_size):
    """
    Check if the current found resources fit in the Role of a given cFp
    :param cfp: the cFp.json
    :param buffer_size: the found buffer description
    :return: 0 on OK, 1 on not OK
    """
    if cFp is None or buffer_size is None:
        # we know nothing, so also nothing bad
        return 0
    check_failed = False
    if cFp['cFpMOD'] in __cF_resources__:
        cur_mod = __cF_resources__[cFp['cFpMOD']]
        if cFp['cFpSRAtype'] in cur_mod:
            cur_role = cur_mod[cFp['cFpSRAtype']]
            cur_name = "{}/{}".format(cur_mod['name'], cur_role['name'])
            buffer_role_byte = cur_role['BRAM_kB'] * 1024
            if buffer_size > buffer_role_byte:
                print("WARNING: Found buffer of {} Bytes may exceed available resources in {}.".format(buffer_size, cur_name))
                check_failed = True
    if check_failed:
        return 1
    else:
        return 0

