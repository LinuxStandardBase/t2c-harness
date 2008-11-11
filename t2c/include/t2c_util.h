#ifndef _T2C_UTIL_H_INCLUDED_
#define _T2C_UTIL_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmem.h"
#include "libstr.h"
#include "libfile.h"

///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif 
    
// Returns path to the main T2C directory (actually, the contents of 
// T2C_ROOT environment variable). If the variable is not defined, "/" is returned.
// If the path has no slash at the end, it is appended.
// The returned pointer should be freed when no longer needed.
char* 
t2c_get_root();

// Returns path to the test suite root directory (the one that contains tet_scen). 
// If T2C_SUITE_ROOT environment variable is defined, its value is returned. 
// If T2C_SUITE_ROOT is not defined, the result of t2c_get_root() is returned.
// If the path has no slash at the end, it is appended.
// The returned pointer should be freed when no longer needed.
char* 
t2c_get_suite_root();


// The function combines $T2C_SUITE_ROOT with the relative path specified
// as an argument and returns the corresponding absolute path.
// The returned path will have a slash at the end.
// The returned pointer should be freed when no longer needed.
char* 
t2c_get_path(const char* rel_path);

// Returns zero if "T2C_ROOT" environment variable is not defined,
// nonzero otherwise.
int 
t2c_root_defined();

// The function constructs the path to the specified test data and returns
// the result.
// The test data directory is $T2C_SUITE_ROOT/<suite_subdir>/testdata/<test_name>.
// 'rel_path' is a path to the data (relative to this directory).
// The function does not check if the resulting path exists.
//
// The returned pointer should be freed when no longer needed.
char*
t2c_get_data_path(const char* suite_subdir, const char* test_name, 
                   const char* rel_path);

//////////////////////////////////////////////////////////////////////////
// REQ catalogue support: parsing, loading, searching etc.
//////////////////////////////////////////////////////////////////////////

// REQ information: ID and text
typedef struct  
{
    char* rid;
    char* text;
} TReqInfo;

typedef TReqInfo* TReqInfoPtr;

// A list of TReqInfo structures
struct TReqInfoList_;
typedef struct TReqInfoList_
{
    TReqInfoPtr ri;
    struct TReqInfoList_* next;
} TReqInfoList;

//////////////////////////////////////////////////////////////////////////
// Traverse the list from the 'head' to its end and deallocate its elements. 
void 
t2c_req_info_list_clear(TReqInfoList* head);
//////////////////////////////////////////////////////////////////////////

// Parse the given string ('str') to find whether it is
// one of the known opening tags and retrieve a string with 
// tag attributes (if they are specified).
//
// Each opening tag has the following form:
//     <NAME [attr0="val0" [attr1="val1" ...]]>
// The search is case sensitive. Leading spaces and tabs in 'str' are ignored.
// Attribute value should be enclosed in single or double quotes.
// Spaces and tabs are used as separators and therefore ignored during
// the parsing.
//
// The known_tags[] array contains tag names the user considers 
// to be valid. The last element of the array should be NULL, so
// known_tags[] should contain at least one element.
// E.g. char* known_tags[] = {"BLOCK", "STARTUP", "CLEANUP", NULL};
// If the string has been parsed successfully and one of known tags
// has been recognized, the function returns its index in the 'known_tags'
// array. Otherwise (if the tag is syntactically invalid or unknown) -1
// is returned.
// 
// If one or more attributes are specified for the known tag, they are
// returned as a single string in *attr_str (if attr_str is not NULL). 
// Previous value of *attr_str shall be freed automatically. 
// If no attribute is specified, *attr_str is set to NULL.
// This string should be freed by the caller when no longer needed.
// 
// Example.
// Consider this tag: "<BLOCK    lsbMinVersion = "3.1" childProcess = "true">"
// The attribute string for this tag is 
// 'lsbMinVersion = "3.1" childProcess = "true"'
int 
t2c_parse_open_tag(const char* str, char* known_tags[], char** attr_str);


// Parse specified string and determine if it is a simplified form of
// xml header declaration (e.g. "<?xml version="1.0"?>").
// Returns nonzero if it is, 0 otherwise. The attributes are ignored as
// well as the leading and trailing whitespace chars.
int 
t2c_parse_decl(const char* str);

// Parse the specified attribute string ('str') and extract values for the attributes
// of interest. Typically the attribute string will be obtained from t2c_parse_open_tag().
// Names of the attributes of interest are specified in a NULL-terminated 'attr_names' array.
// Their values are returned in 'attr_values' array (it should contain enough elements).
// All previous contents of 'attr_values' shall be freed. 'attr_values' should be freed
// when no longer needed.
// If an attribute #i is not specified attr_values[i] will be NULL.
// The function returns 0 if 'str' is syntactically invalid, 1 otherwise (even if unknown
// attributes have been found - they are simply ignored in this case).
// If 'str' is NULL, the function returns 1 (OK) and does nothing.
int
t2c_parse_attributes(const char* str, char* attr_names[], char* attr_values[]);

// Parse the specified string ('str') to find out whether it is a closing tag
// with a given name (e.g. </BLOCK>).
// Returns 1 if it is, 0 otherwise.
int 
t2c_parse_close_tag(const char* str, const char* name);

// Replace "&quot;", "&apos;", "&lt;", "&gt;" and "&amp;" in a given string with 
// appropriate chars. 
// Returns: the modified string.
char*
t2c_unreplace_special_chars(char* str);

//////////////////////////////////////////////////////////////////////////
// Load the REQ catalogues with names specified in NULL-terminated 'rcat_names' array.
// The catalogue #i is expected to be in the following file
//      $T2C_SUITE_ROOT/<suite_subdir>/reqs/<rcat_names[i]>.xml
// The function returns T2C_RCAT_NO_RCAT if no catalogues from the list can be 
// found, T2C_RCAT_BAD_RCAT if some of them are corrupt or some unexpected error occurs. 
// Otherwise the catalogues will be loaded and T2C_RCAT_LOAD_OK will be returned.
// 
// The catalogue will be stored as a list of TReqInfo structures,
// the head of the list shall be returned in *phead. 
// Use t2c_req_info_list_clear(*phead) to deallocate the catalogue.
// The pointers to these TReqInfo structures are also stored in the (*preqs)[] array for
// future sorting & searching. Number of the elements in the array shall be returned in
// *nreq.
typedef enum
{
    T2C_RCAT_BAD_RCAT = 0,
    T2C_RCAT_LOAD_OK  = 1,
    T2C_RCAT_NO_RCAT  = 2
} ERcatLoadCode;

ERcatLoadCode 
t2c_rcat_load(const char* rcat_names[], const char* suite_subdir, 
    TReqInfoList** phead, TReqInfoPtr** preqs, int* nreq);

// Search the req catalogue ('reqs') for the requirement with the specified ID.
// 'reqs' - an array of pointers to the TReqInfos, nreq - number of elements in the array.
// 'rid' - an ID to look for.
// The function returns NULL if nothing is found or the operation cannot be completed.
// Otherwise the requirement text is returned.
// The returned pointer should not be freed manually.
const char* 
t2c_rcat_find(const char* rid, TReqInfoPtr reqs[], int nreq);

// Sort the specified array of pointers to the TReqInfos (req catalogue) in 
// the ascending order of IDs (alphabetic order).
// 'reqs' - an array to be sorted, 
// 'nreq' - number of elements in the array.
void 
t2c_rcat_sort(TReqInfoPtr reqs[], int nreq);

#ifdef	__cplusplus
}
#endif
    
//////////////////////////////////////////////////////////////////////////

#endif //_T2C_UTIL_H_INCLUDED_
