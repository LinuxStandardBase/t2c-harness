# A makefile for the T2C test code generator.

VPATH = ../bin:../include:../debug/src:../debug/include

# Program name
PNAME = t2c

# Compiler
CC = lsbcc

TET_INC_DIR = $(TET_ROOT)/inc/tet3
TET_LIB_DIR = $(TET_ROOT)/lib/tet3

T2C_UTIL = t2c_util
T2C_UTIL_D =     t2c_util_d
T2C_TET_SUPP =   t2c_tet_support
T2C_TET_SUPP_D = t2c_tet_support_d
STANDALONE_MAIN = st_main
DEBUG_MAIN = dbgm
# Compiler options

COMMON_INC_DIR = $(T2C_ROOT)/t2c/include
DBG_INC_DIR = $(T2C_ROOT)/t2c/debug/include 
SECONDARY_INC_DIR = $(T2C_ROOT)/t2c/debug/include 
CC_SPECIAL_FLAGS := $(shell (cat $(T2C_ROOT)/t2c/cc_special_flags) 2>/dev/null)
COMMON_CFLAGS = -std=c99 -D"_POSIX_C_SOURCE=200112L" $(CC_SPECIAL_FLAGS) -I$(COMMON_INC_DIR)
CFLAGS  = -O2 -I$(TET_INC_DIR) $(COMMON_CFLAGS)
DBGFLAGS  = -g -DT2C_DEBUG -I$(DBG_INC_DIR) $(COMMON_CFLAGS)

DBGMAIN_SRC = $(T2C_ROOT)/t2c/debug/src/dbg_main.c


all: $(PNAME) $(DEBUG_MAIN).o $(T2C_UTIL).a $(T2C_UTIL_D).a $(T2C_TET_SUPP).a $(T2C_TET_SUPP_D).a

$(PNAME): main.o param.o $(T2C_UTIL).a 
	$(CC) -o $(PNAME) main.o param.o ../lib/$(T2C_UTIL).a
	chmod a+x $(PNAME)
	mv $(PNAME) ../bin

main.o: main.c
	$(CC) -c $(CFLAGS) -o main.o main.c

param.o: param.c 
	$(CC) -c $(CFLAGS) -o param.o param.c

$(DEBUG_MAIN).o: $(DBGMAIN_SRC)
	$(CC) -c $(DBGFLAGS) -o $(DEBUG_MAIN).o $(DBGMAIN_SRC)
	mv $(DEBUG_MAIN).o ../debug/lib

$(STANDALONE_MAIN).o: $(DBGMAIN_SRC)  
	$(CC) -c $(CFLAGS) -o $(STANDALONE_MAIN).o $(DBGMAIN_SRC)
	mv $(STANDALONE_MAIN).o ../lib
$(T2C_UTIL).a: $(T2C_UTIL).c libmem.c libstr.c libfile.c
	$(CC) -c $(CFLAGS) -o libmem.o libmem.c
	$(CC) -c $(CFLAGS) -o libstr.o libstr.c
	$(CC) -c $(CFLAGS) -o libfile.o libfile.c	
	$(CC) -c $(CFLAGS) -o $(T2C_UTIL).o $(T2C_UTIL).c
	ar rcs $(T2C_UTIL).a $(T2C_UTIL).o libmem.o libstr.o libfile.o 

	mv $(T2C_UTIL).a ../lib

$(T2C_UTIL_D).a: $(T2C_UTIL).c libmem.c libstr.c libfile.c
	$(CC) -c $(DBGFLAGS) -o libmem.o libmem.c
	$(CC) -c $(DBGFLAGS) -o libstr.o libstr.c
	$(CC) -c $(DBGFLAGS) -o libfile.o libfile.c 	
	$(CC) -c $(DBGFLAGS) -o $(T2C_UTIL).o $(T2C_UTIL).c
	ar rcs $(T2C_UTIL_D).a $(T2C_UTIL).o libmem.o libstr.o libfile.o 
	mv $(T2C_UTIL_D).a ../debug/lib
$(T2C_TET_SUPP).a: t2c_fork.c $(T2C_TET_SUPP).c
	$(CC) -c $(CFLAGS) -o t2c_fork.o t2c_fork.c
	$(CC) -c $(CFLAGS) -o $(T2C_TET_SUPP).o $(T2C_TET_SUPP).c
	ar rcs $(T2C_TET_SUPP).a $(T2C_TET_SUPP).o t2c_fork.o 
	mv $(T2C_TET_SUPP).a ../lib
$(T2C_TET_SUPP_D).a: t2c_fork.c $(T2C_TET_SUPP).c
	$(CC) -c $(DBGFLAGS) -o t2c_fork.o t2c_fork.c
	$(CC) -c $(DBGFLAGS) -o $(T2C_TET_SUPP).o $(T2C_TET_SUPP).c
	ar rcs $(T2C_TET_SUPP_D).a $(T2C_TET_SUPP).o t2c_fork.o 
	mv $(T2C_TET_SUPP_D).a ../debug/lib
clean:
	rm -f *.o *.a ../bin/$(PNAME) ../lib/* ../debug/lib/*

.PHONY: all clean  
