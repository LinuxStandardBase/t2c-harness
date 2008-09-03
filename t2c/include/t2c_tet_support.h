#ifndef _T2C_TET_SUPPORT_H_INCLUDED_
#define _T2C_TET_SUPPORT_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmem.h"
#include "libstr.h"
#include "libfile.h"

#include <tet_api.h>
#include <t2c_util.h>

// Test suite-specific result code.
#define T2C_TIME_EXPIRED    65

///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif 

// A replacement for tet_printf that takes 'const char*' instead of 'char*'
int t2c_printf(const char* format, ...);
    
//////////////////////////////////////////////////////////////////////////
// REQ implementation
void 
t2c_req_impl(const char* r_id,    // requirement ID
    const char* r_comment,        // a comment specified in REQ as the 2nd arg.
    TReqInfoPtr reqs[], int nreq, // needed for req lookup
    const char* fname, int line,  // usually: __FILE__ and __LINE__
    int bVerbose,                  // message output mode (0 - journal only, 1 - journal & stderr)
    int gen_hlinks,               // 0 - output plain IDs, != 0 - produce a hypertext reference for each ID
    const char* href_tpl          // hypertext reference template.
);

// Utility function that displays the list of IDs of checked requirements 
// (one ID per line).
void 
t2c_checked_req_out(const char* r_id,   // requirement ID
    int bVerbose                        // message output mode (0 - journal only, 1 - journal & stderr)
);
    
//////////////////////////////////////////////////////////////////////////
// t2c_fork & its special stuff

// Parent control function type.
typedef int (*TParentControlFunc)(pid_t, int*);

// Child function type.
typedef void (*TChildFunc)();

// User-defined startup func type.
typedef void (*TUserStartup)(int*, char**);

// User-defined cleanup func type.
typedef void (*TUserCleanup)();

// t2c_fork() executes specified function ('childfunc') in a child process
// while the parent process waits (using waitpid).
//
// [!!!] This function is intended to be called by other T2C functions only, not 
// by the user.
//
// If waittime is positive, the child process is allowed to run for only
// 'waittime' seconds. After that it will be terminated and the test result 
// code set to T2C_TIME_EXPIRED.
//
// If waittime is zero or negative, the parent process will wait indefinitely 
// for the child to terminate.
//
// If the parent waits, the "parent control function" will be called after it's finished.
// ('pcf' is used as a parent control function if it is not NULL, otherwise
// default function is used.)
// The "parent control function" (PCF) desides whether the child process has completed
// successfully (in this case the function returns nonzero, zero otherwize). 
// The PCF receives the value returned by waitpid() and a pointer to the status info
// (see waitpid() documentation for details).
// If the PCF reports failure, the test result will be set to UNRESOLVED.
// 
// After the child terminates, parent should be able to read the test execution
// status from pcc_pipe_[0]: 1 - PASS, 0 - everything else. This status will become
// the return value of t2c_fork(). If the status cannot be read, the result 
// will be set to UNRESOLVED and -1 will be returned.
//
// -1 is also returned in case of any other failure.
//
// 'ustartup' (if not NULL) is called in the child process before 'childfunc'. 
// Typically it contains user-defined startup instructions.
// If the 1st arg of 'ustartup' is TRUE after that (init_failed), childfunc will not
// be executed. The message pointed to by the 2nd arg (reason_to_cancel) will 
// be output, the result will be set to UNINITIATED and no-PASS status will be 
// sent to the parent through the pipe.
//
// 'ucleanup' (if not NULL) will be called after 'childfunc' (or instead of it
// if 'ustartup' indicates failure).
// Typically it contains user-defined cleanup instructions.
int 
t2c_fork(TChildFunc childfunc, TParentControlFunc pcf, int waittime,
    TUserStartup ustartup, TUserCleanup ucleanup);

// t2c_fork_dbg() is used instead of t2c_fork in the standalone tests
// (those that do not use TET). This function actually does not fork(),
// it just executes the test in the same process. This can be useful for 
// debugging, especially taking into account that a developer usually 
// investigates and debugs one test purpose at a time.
// 
// The meaning of the arguments and the return value is the same as for 
// t2c_fork, except pcf and waittime are ignored. 

// There is no time limit for a standalone test.
int 
t2c_fork_dbg(TChildFunc childfunc, TParentControlFunc pcf, int waittime,
    TUserStartup ustartup, TUserCleanup ucleanup);

// A special pipe for transfering parent-child control data. 
// [!!!] The pipe must be created BEFORE t2c_fork is called.
// Usually it will be created in the global TET startup function.
extern int pcc_pipe_[2];

// Macros: reading test status from and writing it to the parent-child 
// control pipe.
// Argument: int status;
#define T2C_GET_STATUS(status) {        \
    int pstat = -1;                     \
    int num = read(pcc_pipe_[0], &pstat, sizeof(int));      \
    if (num == sizeof(int)) {           \
        status = pstat;                 \
    } else {                            \
        TRACE0("Warning! Unable to read test status.");     \
    }                                   \
}

#define T2C_PUT_STATUS(status) {        \
    int pstat = status;                 \
    int num = write(pcc_pipe_[1], &pstat, sizeof(int));     \
    if (num != sizeof(int)) {           \
        TRACE0("Warning! Unable to write test status.");    \
    }                                   \
}

// Read previous test status (in should be the only data in the pipe)
// and write a new one (specified in 'status' argument).
#define T2C_RESET_STATUS(status) {      \
    int tmp_ = -1;                      \
    T2C_GET_STATUS(tmp_);               \
    T2C_PUT_STATUS(status);             \
}

// Type of the test purpose function pointer.
typedef void (*TTestPurposeType)();

#ifdef	__cplusplus
}
#endif
    
//////////////////////////////////////////////////////////////////////////

#endif //_T2C_TET_SUPPORT_H_INCLUDED_
