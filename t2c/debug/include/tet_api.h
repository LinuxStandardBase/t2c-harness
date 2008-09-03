#ifndef TET_DBG_API_H_INCLUDED
#define TET_DBG_API_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

// Standard result codes - may be passed to tet_result() 
#define TET_PASS        0
#define TET_FAIL        1
#define TET_UNRESOLVED  2
#define TET_NOTINUSE    3
#define TET_UNSUPPORTED 4
#define TET_UNTESTED    5
#define TET_UNINITIATED 6
#define TET_NORESULT    7

/////////////////////////////////////////////////////////////////////////////
// Public functions provided by the API
void 
tet_delete(int testno, char* reason);

char* 
tet_getvar(char* name);

// Outputs a line to stdout.
//void tet_infoline(char* line);
#define tet_infoline(line) printf("%s\n", line)

// Does nothing.
void 
tet_setcontext();

// Does nothing.
void 
tet_setblock();

// Same as printf
//int tet_printf(char* format, ...);
#define tet_printf printf

// Same as vprintf but adds a newline
int tet_vprintf(char* format, va_list ap);
//#define tet_vprintf(format, ap) {vprintf(format, ap); printf("\n");}

void 
tet_result(int result);

// Public data items provided by the API
extern char*    tet_pname;
extern int      tet_thistest;

/////////////////////////////////////////////////////////////////////////////
// The items below should be provided by the user

// the test case startup and cleanup functions
extern void (*tet_startup)();
extern void (*tet_cleanup)();

// the data structure describing the test purposes
struct tet_testlist 
{
    void (*testfunc)(); // test purpose function
    int icref;          // invocable component which the test purpose belongs to
};

// test purpose list
extern struct tet_testlist tet_testlist[];
/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* TET_DBG_API_H_INCLUDED */

