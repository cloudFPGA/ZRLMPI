SRC_FILES=test.cpp test.hpp
MPI_FLAVOR=.mpich
CFLAGS=-DDEBUG -DZRLMPI_SW_ONLY
#MPI_FLAVOR=.openmpi

.PHONY: all run clean 
all: jacobi

run: jacobi
	@#mpirun -np 4 jacobi
	@#mpirun$(MPI_FLAVOR) -np 3 --hostfile hostfile --rankfile rankfile ./$^
	mpirun$(MPI_FLAVOR) -np 17 ./$^


jacobi: $(SRC_FILES)
	mpicc$(MPI_FLAVOR) -o $@ $^ -lm $(CFLAGS)


clean:
	/bin/rm -f jacobi jacobi.o jacobi.log
