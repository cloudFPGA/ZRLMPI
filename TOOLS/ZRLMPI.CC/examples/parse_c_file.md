Parse a C file with pycparser
===============================

## GCC

1. `gcc -std=c99 -pedantic -Wall -Wextra -Wconversion -Werror  -nostdinc -E -D'__attribute__(x)=' -I../../../../../pycparser/utils/fake_libc_include  -I/usr/local/include/ pingpong.c > app_parsed.cc` 
2. execute python, e.g. `c-to-c.py`

## Clang 

Build it from git!!

https://github.com/llvm/llvm-project#getting-the-source-code-and-building-llvm

https://llvm.org/docs/CMake.html#id6

```
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib:/usr/local/lib64:/usr/lib64
clang -I/home/ngl/gitrepos/cloudFPGA/ZRLMPI/LIB/HW/hls/mpi_wrapperv1/src -I/opt/Xilinx/Vivado/2017.4/include/ -Xclang -ast-dump -fsyntax-only app_hw.cpp
```
