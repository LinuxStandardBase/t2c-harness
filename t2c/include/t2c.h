#ifndef _T2C_H_INCLUDED_
#define _T2C_H_INCLUDED_

#include "t2c_trace.h"
#include "t2c_util.h"

#ifndef FALSE
#define FALSE 0
#define TRUE (!FALSE)
#endif

// REQ macro definition.
// Define CHECK_EXT_REQS before #include <t2c.h> to enable checking
// of the external requirements(those with the "ext"-prefixed IDs.)

#define IS_TODO_REQ_EXPR(s_expr) (strstr(s_expr, "TODO_REQ"))

#define HAS_EXT_PREFIX(str_id) ((strstr(str_id, "ext.") == (str_id)) || strstr(str_id, ".ext."))

#ifdef  CHECK_EXT_REQS
#define REQ_CHECKABLE(str_id) (TRUE)
#else
#define REQ_CHECKABLE(str_id) (!HAS_EXT_PREFIX(str_id))
#endif

// This macro is used in RETURN and in the test templates.
#define TP_RETURN T2C_RESET_STATUS(test_passed_flag); return;

// Use this macro instead of the "return" statement in your test cases.
#define RETURN                      \
if (tp_in_finally == 0)             \
{                                   \
    goto purpose_finally_section_;  \
}                                   \
else                                \
{                                   \
    TP_RETURN;                      \
}    

// If T2C_IGNORE_RCAT is defined and the appropriate requirement 
// catalogue is not found or loading fails or the catalogue is incomplete, 
// it is not considered an error. Empty text will be used as the requirement text.
// Otherwise the execution of the tests will stop.
#define REQ(r_id, r_comment, r_expr) {                         \
    if(REQ_CHECKABLE(r_id) && !IS_TODO_REQ_EXPR(#r_expr)) {    \
        if(!(r_expr)) {                                        \
            t2c_req_impl(r_id, r_comment, reqs_, nreq_, __FILE__, __LINE__, bVerbose, gen_hlinks, t2c_href_tpl_); \
            test_passed_flag = FALSE;                          \
            RETURN;                                            \
        } else {                                               \
            t2c_checked_req_out(r_id, bVerbose);               \
        }                                                      \
    }                                                          \
}

#define TODO_REQ() (TRUE)

// Use this macro in the <STARTUP> section to indicate that the test case 
// initialization has failed.
#define INIT_FAILED(msg) {          \
    init_failed = TRUE;             \
    reason_to_cancel = msg;         \
}

// This macro aborts test execution and sets the result of the test to 'FAIL'.
// Usually one does not need this because test failures are detected by REQ 
// macro. 
// This macro is provided for those rare cases in which using REQ is
// inconvenient or even inappropriate. 
// 'msg' should describe the situation (can be a constant string).
// [NB] Code in the <FINALLY> section will be executed anyway unless this macro
// is called from the <FINALLY> section.
#define TEST_FAILED(msg) {    \
    TRACE0(msg);              \
    test_passed_flag = FALSE; \
    tet_result(TET_FAIL);     \
    RETURN;                   \
}

// Use this macro to abort test purpose execution if something wrong happens
// with its local data(e.g. memory allocation failure etc.)
// [NB] Code in the <FINALLY> section will be executed anyway unless this macro
// is called from the <FINALLY> section.
#define ABORT_TEST_PURPOSE(msg) {   \
    TRACE("Error: %s", msg);        \
    test_passed_flag = FALSE;       \
    tet_result(TET_UNRESOLVED);     \
    RETURN;                         \
}

// Use this macro in the test purpose to abort execution if an OPTIONAL feature to be 
// checked is not supported by the system under test. 
// The specified message goes to the journal and should describe the situation,
// e.g. "Dynamic loading of modules is not supported."
// [NB] Code in the <FINALLY> section will be executed anyway unless this macro
// is called from the <FINALLY> section.
#define ABORT_UNSUPPORTED(msg) {    \
    TRACE0(msg);                    \
    tet_result(TET_UNSUPPORTED);    \
    RETURN;                         \
}

// Use this macro in the test purpose to abort execution if a MANDATORY feature to be 
// checked is not supported by the system under test. 
// The specified message goes to the journal and should describe the situation.
// [NB] Code in the <FINALLY> section will be executed anyway unless this macro
// is called from the <FINALLY> section.
#define ABORT_UNTESTED(msg) {   \
    TRACE0(msg);                \
    test_passed_flag = FALSE;   \
    tet_result(TET_UNTESTED);   \
    RETURN;                     \
}

// These constructs mark the beginning and the end of the "finally" section contents
// for this test purpose.
#define BEGIN_FINALLY_SECTION purpose_finally_section_:
#define END_FINALLY_SECTION
    
// This macro constructs the path to the specified test data and returns
// the result.
// The test data directory is $T2C_ROOT/<suite_subdir>/testdata/<test_name_>.
// 'rel_path' is a path to the data(relative to this directory).
// The macro does not check if the resulting path exists.
//
// The returned pointer should be freed when no longer needed.
#define T2C_GET_DATA_PATH(rel_path) (t2c_get_data_path(suite_subdir_, test_name_, rel_path))

// This macro constructs the path to the executable file for the current test
// and returns the result.
// The returned pointer should be freed when no longer needed.
#define T2C_GET_TEST_PATH() ((char*)strdup(tet_pname))
    
#endif //_T2C_H_INCLUDED_
