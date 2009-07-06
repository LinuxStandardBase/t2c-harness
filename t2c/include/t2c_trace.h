#ifndef _T2C_TRACE_H_INCLUDED_
#define _T2C_TRACE_H_INCLUDED_

// TRACE and TRACE0 macros for message output.
// 'bVerbose' variable should be available at the point where these macros
// are used.
#define TRACE(str, ...) {                   \
    t2c_printf(str, __VA_ARGS__);       \
    if(bVerbose) {                          \
        fprintf(stderr, str, __VA_ARGS__);  \
        fprintf(stderr, "\n");              \
    }                                       \
}

// A variant of TRACE with the format string only(no other parameters).
#define TRACE0(str) TRACE("%s", str)

// This macro performs the formatted output only to stderr in "verbose" mode.
// Nothing is sent to TET journal.
// If "verbose" mode is off, TRACE_NO_JOURNAL does nothing.
#define TRACE_NO_JOURNAL(str, ...) {        \
    if(bVerbose) {                          \
        fprintf(stderr, str, __VA_ARGS__);  \
        fprintf(stderr, "\n");              \
    }                                       \
}

#endif //_T2C_TRACE_H_INCLUDED_
