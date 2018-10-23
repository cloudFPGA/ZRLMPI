SRC_FILES=jacobi.c jacobi_common.h
MPI_FLAVOR=.mpich
#MPI_FLAVOR=.openmpi

.PHONY: all run clean 
all: jacobi

run: jacobi
	@#mpirun -np 4 jacobi
	@#mpirun$(MPI_FLAVOR) -np 3 --hostfile hostfile --rankfile rankfile ./$^
	mpirun$(MPI_FLAVOR) -np 3 ./$^


jacobi: $(SRC_FILES)
	mpicc$(MPI_FLAVOR) -o $@ $^ -lm 


clean:
	/bin/rm -f jacobi jacobi.o jacobi.log
