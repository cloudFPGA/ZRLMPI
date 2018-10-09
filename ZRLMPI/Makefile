CC       = g++
CPPFLAGS = -D_XOPEN_SOURCE=700
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wconversion -Werror -pthread
CFLAGS  += -g -fPIC
#LIBS     = -lpthread
PROG     = zrlmpi_server
BUILD_TYPE = debug # oder production
SOURCES = MPI.cpp test.cpp zrlmpi_common.cpp
OBJS    = $(SOURCES:%.c=%.o)


.PHONY: all clean

all: $(PROG)

clean:
	rm -rf $(PROG) *.o 

$(PROG): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) 
	@# $(LIBS)
ifeq ($(BUILD_TYPE), production)
	strip $@
endif 

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@


$(PROG): MPI.hpp zrlmpi_common.hpp zrlmpi_int.hpp test.hpp
MPI.o: MPI.cpp test.hpp zrlmpi_common.hpp MPI.hpp

