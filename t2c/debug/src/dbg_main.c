// A simple implementation of TET functions - for debugging purposes
// and standalone (without TET) test execution.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/tet_api.h"

#define DBG_TET_RES_CODES 8
    
// Test suite-specific result code.
#define T2C_TIME_EXPIRED    65

// String representation of TET result codes.
char* dbg_result_string[] = {
    "PASS",
    "FAIL",
    "UNRESOLVED",
    "NOTINUSE",
    "UNSUPPORTED",
    "UNTESTED",
    "UNINITIATED",
    "NORESULT",
    "TIME EXPIRED"  // if res_code == T2C_TIME_EXPIRED then res_code := DBG_TET_RES_CODES.
};

// Current 'tet_result' value
//int dbg_result = TET_NORESULT;

// Verbose mode status (1 - on, 0 - off).
int verbose = 0;

// Public data items provided by the API
char* tet_pname = NULL;
int tet_thistest = -1;

// Comments for the test purposes. The reason to cancel goes here (see tet_delete()).
// Default value = NULL;
char** comment;

// Total number of test purposes.
size_t tp_count = 0;

// A pipe that will be used to transfer the result code.
int rc_pipe[2];

/////////////////////////////////////////////////////////////////////////////
// Helper functions & macros

// Result code transfer.
#define DBG_GET_RESULT_CODE(res_code) { \
    int pstat = -1;                     \
    int num = read(rc_pipe[0], &pstat, sizeof(int));                  \
    if (num == sizeof(int)) {           \
        res_code = pstat;               \
    } else {                            \
        fprintf(stderr, "Warning! Unable to read result code.");      \
    }                                   \
}

#define DBG_PUT_RESULT_CODE(res_code) { \
    int pstat = res_code;               \
    int num = write(rc_pipe[1], &pstat, sizeof(int));             \
    if (num != sizeof(int)) {           \
        fprintf(stderr, "Warning! Unable to write result code.");  \
    }                                   \
}

// Read previous result code (in should be the only data in the pipe)
// and write a new one (specified in 'res_code' argument).
#define DBG_RESET_RESULT_CODE(res_code) {   \
    int tmp_ = -1;                          \
    DBG_GET_RESULT_CODE(tmp_);              \
    DBG_PUT_RESULT_CODE(res_code);          \
}

// Resets the current 'tet_result' value (sets it to -1).
static void
dbg_reset_result();

// Returns a ptr to a string representation of the current 'tet_result' value
// ("PASS", "FAIL" etc.). 
// Do not attempt to modify or free the returned pointer or change the contents
// of this string.
static const char*
dbg_get_result_string(int result);

// Returns a comment for the test purpose #tpnum. The returned pointer can be
// NULL. It should not be freed or altered by the caller.
static const char*
dbg_get_tp_comment(size_t tpnum);

//////////////////////////////////////////////////////////////////////////
// other
static size_t
dbg_get_tp_count()
{
    size_t i = 0;
    while (tet_testlist[i].testfunc && tet_testlist[i].icref)
    {
        ++i;
    }
    return i;
}

static int 
dbg_init()
{
    tp_count = dbg_get_tp_count();
    comment = (char**)malloc(tp_count * sizeof(char*));
    
    if (!comment)
    {
        return 0;
    }

    size_t i;
    for (i = 0; i < tp_count; ++i)
    {
        comment[i] = NULL;
    }

    int rcOK = pipe(rc_pipe);
    if (rcOK != 0)
    {
        fprintf(stderr, "Unable to open a pipe for result code transfer.\n");
        return 0;
    }

    return 1;
}

static void
dbg_cleanup()
{
    size_t i;
    for (i = 0; i < tp_count; ++i)
    {
        free(comment[i]);
    }

    free(comment);
    
    close(rc_pipe[0]);
    close(rc_pipe[1]);

    return;
}

// Execute the specified test purpose. ('num' is its index, rather than
// invocable component number.)
static void
dbg_exec_tp(int tp_num);

/////////////////////////////////////////////////////////////////////////////
// Main
int
main(int argc, char* argv[])
{
    if (!dbg_init())
    {
        return 1;
    }
    
    int arg_pos = 1;
    if (argc > 1)
    {
        if (!strcmp(argv[1], "-v"))
        {
            verbose = 1;
            arg_pos = 2;
            printf("Verbose mode is ON.\n");
        }
        else
        {
            verbose = 0;
        }
    }

    int ic_num = -1;
    if (argc > arg_pos)
    {
        // the user has specified which invocable component to execute.
        ic_num = atoi(argv[arg_pos]);
        printf("Invocable component #%d will be executed.\n", ic_num);
    }

    tet_pname = argv[0];
    tet_thistest = 0;
    if (tet_startup != NULL)
    {
        tet_startup();
    }

    // Execute the test purposes.
    int found = 0;
    size_t i;
    for (i = 0; i < tp_count; ++i)
    {
        tet_thistest = (int)i + 1;
        if ((ic_num == -1) || (ic_num == tet_testlist[i].icref))
        {
            found = 1;
            printf("TP #%d begins\n", tet_thistest);
            
            dbg_reset_result();

            if (tet_testlist[i].testfunc == NULL)
            {
                printf("The test has been canceled. Reason:\n%s\n", 
                    comment[i]);
                tet_result(TET_UNINITIATED);
            }
            else
            {
                tet_testlist[i].testfunc();
            }
            
            // Read result code (dbg_result) from the pipe.
            int dbg_result = -1;
            DBG_GET_RESULT_CODE(dbg_result);
                            
            if (dbg_result == -1) dbg_result = TET_NORESULT;

            printf("TP #%d ends. Result: %s\n\n", tet_thistest, 
                dbg_get_result_string(dbg_result));
        }
    }

    if (!found)
    {
        fprintf(stderr, "No test purposes found for invocable component #%d.\n",
            ic_num);
    }

    tet_thistest = 0;
    if (tet_cleanup != NULL)
    {
        tet_cleanup();
    }
    
    // Clean up
    dbg_cleanup();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Public functions provided by the API
void 
tet_delete(int testno, char* reason)
{
    size_t num = (size_t)testno - 1;
    if (num >= tp_count)
    {
        fprintf(stderr, "tet_delete: invalid test number.\n");
        return;
    }

    if (tet_testlist[num].testfunc)
    {
        tet_testlist[num].testfunc = NULL;
        comment[num] = strdup(reason);
    }

    return;
}

char* 
tet_getvar(char* name)
{
    static char* str_yes = "yes";
    static char* str_no = "no";

    if (!strcmp(name, "VERBOSE"))
    {
        return (verbose ? str_yes : str_no);
    }
    else
    {
        return NULL;
    }
}

/*void 
tet_infoline(char* line)
{
    fprintf(stderr, "%s\n", line);
    return;
}

int 
tet_printf(char* format, ...)
{
    // Does nothing.
    return 0;
}*/

int 
tet_vprintf(char* format, va_list ap)
{
    int res = vprintf(format, ap);
    printf("\n");
    return res;
}

void 
tet_setcontext()
{
    return;
}

void 
tet_setblock()
{
    return;
}

static int 
dbg_get_result(int result, int prev)
{
    int real_result = prev;
    
    // 1. FAIL
    if (result == TET_FAIL)
    {
        real_result = TET_FAIL;
    }

    if (real_result == TET_FAIL)
    {
        return real_result;
    }

    // 2. UNRES, UNINIT
    if ((result == TET_UNRESOLVED) || 
        (result == TET_UNINITIATED))
    {
        if ((real_result != TET_UNRESOLVED) && 
            (real_result != TET_UNINITIATED))
        {
            real_result = result;
        }
    }

    if ((real_result == TET_UNRESOLVED) || 
        (real_result == TET_UNINITIATED))
    {
        return real_result;
    }

    // 3. NORES
    if (result == TET_NORESULT)
    {
        real_result = result;
    }

    if (real_result == TET_NORESULT)
    {
        return real_result;
    }
    
    // 4. TIME_EXPIRED
    if (result == T2C_TIME_EXPIRED)
    {
        real_result = DBG_TET_RES_CODES;
    }

    if (real_result == DBG_TET_RES_CODES)
    {
        return real_result;
    }

    // 5. UNSUP, UNTEST, NOTIN
    if ((result == TET_UNSUPPORTED) || 
        (result == TET_UNTESTED) || 
        (result == TET_NOTINUSE))
    {
        if ((real_result != TET_UNSUPPORTED) && 
            (real_result != TET_UNTESTED) && 
            (real_result != TET_NOTINUSE))
        {
            real_result = result;
        }   
    }

    if ((real_result == TET_UNSUPPORTED) ||
        (real_result == TET_UNTESTED) ||
        (real_result == TET_NOTINUSE))
    {
        return real_result;
    }   

    // 6. PASS
    if (result == TET_PASS)
    {
        real_result = result;
    }

    if (real_result == TET_PASS)
    {
        return real_result;
    }

    // 7. Unknown result code
    real_result = TET_NORESULT;
    return real_result;
}

void 
tet_result(int result)
{
    int prev = -1;
    DBG_GET_RESULT_CODE(prev);
    DBG_PUT_RESULT_CODE(dbg_get_result(result, prev));
}

/////////////////////////////////////////////////////////////////////////////
static void
dbg_reset_result()
{
    DBG_PUT_RESULT_CODE(-1);
    return;
}

static const char*
dbg_get_result_string(int result)
{
    if ((result < 0) || (result > DBG_TET_RES_CODES))
    {
        return NULL;
    }
    else
    {
        return dbg_result_string[result];
    }
}

static const char*
dbg_get_tp_comment(size_t tpnum)
{
    if (tpnum >= tp_count)
    {
        return NULL;
    }

    return comment[tpnum];
}

static void
dbg_exec_tp(int tp_num)
{

    return;
}

///////////////////////
