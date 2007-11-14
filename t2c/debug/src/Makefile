# A makefile for the T2C debug support tools.

VPATH = ../lib:../include:../../src:../../include
LIB_SRC_PATH = ../../src

# Compiler
CC = lsbcc

DBG_INC_DIR = $(T2C_ROOT)/t2c/debug/include

T2C_UTIL = t2c_util_d

# Compiler options
CC_SPECIAL_FLAGS := $(shell (cat $(T2C_ROOT)/t2c/cc_special_flags) 2>/dev/null)
CFLAGS  = -g -DT2C_DEBUG -std=c99 -D"_POSIX_C_SOURCE=200112L" -I$(DBG_INC_DIR) $(CC_SPECIAL_FLAGS)

# Linker options
LFLAGS  = 
ADD_LIBS = 

all: dbgm.o $(T2C_UTIL).a

dbgm.o: dbg_main.c
	$(CC) -c $(CFLAGS) -o $@ $<
	mv $@ ../lib

$(T2C_UTIL).a: t2c_util.c t2c_fork.c libmem.c libstr.c libfile.c
	rm -f ../lib/$(T2C_UTIL).a
	$(CC) -c $(CFLAGS) -o $(T2C_UTIL).o $(LIB_SRC_PATH)/t2c_util.c
	$(CC) -c $(CFLAGS) -o t2c_fork.o $(LIB_SRC_PATH)/t2c_fork.c
	$(CC) -c $(CFLAGS) -o libmem.o $(LIB_SRC_PATH)/libmem.c
	$(CC) -c $(CFLAGS) -o libstr.o $(LIB_SRC_PATH)/libstr.c
	$(CC) -c $(CFLAGS) -o libfile.o $(LIB_SRC_PATH)/libfile.c	

	ar rcs $(T2C_UTIL).a $(T2C_UTIL).o t2c_fork.o libmem.o libstr.o libfile.o 

	mv $(T2C_UTIL).a ../lib

clean:
	rm -f *.o *.a ../lib/*.a ../lib/*.o

.PHONY: all clean  
