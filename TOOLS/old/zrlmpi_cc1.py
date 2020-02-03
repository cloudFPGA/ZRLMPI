import re
import os
import sys
import subprocess

__match_regex__ = []
__replace_hw__ = []
__replace_sw__ = []
__SKIP_STRING__ = "CONTINUE"

# include "mpi"
__match_regex__.append('#include "mpi.h"')
__replace_hw__.append('#include "MPI.hpp"')
__replace_sw__.append('#include "MPI.hpp"')

# include <mpi>
__match_regex__.append('#include <mpi.h>')
__replace_hw__.append('#include "MPI.hpp"')
__replace_sw__.append('#include "MPI.hpp"')

# Main
__match_regex__.append('int main( int argc, char **argv )')
__replace_hw__.append('int app_main(\n    // ----- MPI_Interface -----\n    stream<MPI_Interface> *soMPIif,\n    stream<Axis<8> > *soMPI_data,\n    stream<Axis<8> > *siMPI_data\n    )')
__replace_sw__.append('int app_main()')

#MPI Init
__match_regex__.append('MPI_Init( &argc, &argv )')
__replace_hw__.append('MPI_Init()')
__replace_sw__.append('MPI_Init()')

#MPI Send
__match_regex__.append('MPI_Send(')
__replace_hw__.append('MPI_Send(soMPIif, soMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)

#MPI Recv
__match_regex__.append('MPI_Recv(')
__replace_hw__.append('MPI_Recv(soMPIif, siMPI_data, ')
__replace_sw__.append(__SKIP_STRING__)


def zrlmpi_cc_v0(inputSWOnly, inputHW, hw_out_file, sw_out_file):
    hw_out = inputHW
    sw_out = inputSWOnly

    for i in range(0, len(__match_regex__)):
        if __replace_hw__[i] != __SKIP_STRING__:
            hw_out = re.sub(re.escape(__match_regex__[i]), __replace_hw__[i], hw_out)
        if __replace_sw__[i] != __SKIP_STRING__:
            sw_out = re.sub(re.escape(__match_regex__[i]), __replace_sw__[i], sw_out)

    hw_out_file.write(hw_out)
    sw_out_file.write(sw_out)


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("USAGE: {0} mpi_input hw_output_file sw_output_file".format(sys.argv[0]))
        exit(1)

    own_dir = os.path.dirname(os.path.realpath(__file__))
    sw_pre = subprocess.run(["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(sys.argv[1]), "-DZRLMPI_SW_ONLY", "-UDEBUG"], stdout=subprocess.PIPE, cwd=os.getcwd())
    hw_pre = subprocess.run(["{0}/unifdef/unifdef".format(own_dir), "./{0}".format(sys.argv[1]), "-UZRLMPI_SW_ONLY", "-UDEBUG"], stdout=subprocess.PIPE, cwd=os.getcwd())

    # substitute #include statements
    file_name = os.path.basename(sys.argv[1])[:-4]
    __match_regex__.append('#include "{0}.hpp"'.format(file_name))
    __replace_hw__.append('#include "app_hw.hpp"')
    __replace_sw__.append('#include "app_sw.hpp"')


    #with open(sys.argv[1], "r") as inputFile, open(sys.argv[2], "w") as hw_out_file, open(sys.argv[3], "w") as sw_out_file:
    with open(sys.argv[2], "w") as hw_out_file, open(sys.argv[3], "w") as sw_out_file:
        zrlmpi_cc_v0(sw_pre.stdout.decode('utf-8'), hw_pre.stdout.decode('utf-8'), hw_out_file, sw_out_file)
