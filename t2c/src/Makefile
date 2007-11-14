# A makefile for the T2C test code generator.

VPATH = ../bin:../lib:../include

# Program name
PNAME = t2c

# Compiler
CC = lsbcc

TET_INC_DIR = $(TET_ROOT)/inc/tet3
TET_LIB_DIR = $(TET_ROOT)/lib/tet3

T2C_UTIL = t2c_util

# Compiler options

CC_SPECIAL_FLAGS := $(shell (cat $(T2C_ROOT)/t2c/cc_special_flags) 2>/dev/null)
CFLAGS  = -O2 -std=c99 -D"_POSIX_C_SOURCE=200112L" -I$(TET_INC_DIR) $(CC_SPECIAL_FLAGS) 

# Linker options
LFLAGS  = 
ADD_LIBS = ../lib/$(T2C_UTIL).a $(TET_LIB_DIR)/libapi.a

all: $(PNAME) 

$(PNAME): main.o $(T2C_UTIL).a
	$(CC) -o $(PNAME) main.o $(LFLAGS) $(ADD_LIBS)
	chmod a+x $(PNAME)
	mv $(PNAME) ../bin

main.o: main.c
	$(CC) -c $(CFLAGS) -o main.o main.c

$(T2C_UTIL).a: $(T2C_UTIL).c libmem.c libstr.c libfile.c t2c_fork.c
	rm -f ../lib/$(T2C_UTIL).a
	$(CC) -c $(CFLAGS) -o $(T2C_UTIL).o $(T2C_UTIL).c
	$(CC) -c $(CFLAGS) -o t2c_fork.o t2c_fork.c
	$(CC) -c $(CFLAGS) -o libmem.o libmem.c
	$(CC) -c $(CFLAGS) -o libstr.o libstr.c
	$(CC) -c $(CFLAGS) -o libfile.o libfile.c	

	ar rcs $(T2C_UTIL).a $(T2C_UTIL).o t2c_fork.o libmem.o libstr.o libfile.o 

	mv $(T2C_UTIL).a ../lib

clean:
	rm -f *.o *.a ../bin/$(PNAME) ../lib/$(T2C_UTIL).a

.PHONY: all clean  
