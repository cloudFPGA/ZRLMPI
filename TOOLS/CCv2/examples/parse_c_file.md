Parse a C file with pycparser
===============================

1. `gcc -std=c99 -pedantic -Wall -Wextra -Wconversion -Werror  -nostdinc -E -D'__attribute__(x)=' -I../../../../../pycparser/utils/fake_libc_include  -I/usr/local/include/ pingpong.c > app_parsed.cc` 
2. execute python, e.g. `c-to-c.py`
