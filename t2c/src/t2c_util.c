/******************************************************************************
Copyright (C) 2007 The Linux Foundation. All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
******************************************************************************/

/******************************************************************************
This file contains implementation of the utility functions used by  
the 'T2C' test generation framework.
******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/t2c_util.h"
//#include "../include/t2c_trace.h"

// Sends the specified error message from the T2C util library to stderr.
static void 
t2c_util_error(const char* msg);

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const char* t2c_env_name        = "T2C_ROOT";
const char* t2c_env_suite_name  = "T2C_SUITE_ROOT";

// Req cat. loading modes.
typedef enum
{
    T2C_RCAT_STOP = 0,
    T2C_RCAT_HDR_DECL,
    T2C_RCAT_OPEN_TAG,
    T2C_RCAT_BODY,
    T2C_RCAT_CLOSE_TAG
} T2CRcatMode;

//////////////////////////////////////////////////////////////////////////
// Create a new TReqInfo structure and fill it with the specified data.
// Returns NULL if unsuccessful.
TReqInfoPtr
t2c_req_info_new(const char* rid, const char* text);

// Deallocate the contents of the TReqInfo structure and then delete 
// the structure itself.
// The function does nothing if ri == NULL.
void 
t2c_req_info_delete(TReqInfoPtr ri);

// Load the REQ catalog from the specified file. The catalog 
// will be appended at the tail (*ptail) of a list of TReqInfo structures. 
// New tail of the list will be returned in '*ptail'.  
// The function returns 0 if the operation cannot be completed (e.g. if 
// the catalog is corrupt), nonzero otherwise.
int
t2c_rcat_load_impl(FILE* fd, TReqInfoList** ptail);

// This function constructs the path to the requirement catalog and returns
// the result.
// The catalog is expected to be found in 
//      $T2C_SUITE_ROOT/<suite_subdir>/reqs/<rcat_name>.xml
// The function does not check if the resulting path exists.
//
// The returned pointer should be freed when no longer needed.
char*
t2c_get_rcat_path(const char* suite_subdir, const char* rcat_name);

// Return the length of the list (including head). If 'head' is NULL, the function 
// shall return 0;
int 
t2c_req_info_list_length(TReqInfoList* head);

// Create a new TReqInfo structure for 'rid' and 'text' and append it to the list.
// The structure will be appended after the 'tail' node. A pointer to this structure
// (i.e. the new tail) will be returned.
// If 'tail' is NULL, the new node will be the head of a new list.
// The function returns NULL if the operation cannot be completed.
TReqInfoList* 
t2c_req_info_list_append(TReqInfoList* tail, const char* rid, const char* text);

// Allocate an array of 'TReqInfoPtr's and copy the contents of the list there.
// 'head' should be a fictive head of the list: the real data are expected to
// begin from head->next.
// Number of elements in the array is returned in *len.
// The caller is responsible to free the array when it is no longer needed.
// The function returns NULL if the operation cannot be completed.
TReqInfoPtr* 
t2c_req_info_list_to_array(TReqInfoList* head, int* len);

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////
static void 
t2c_util_error(const char* msg)
{
    fprintf(stderr, "T2C_Util error: %s\n", msg);
}

char* 
t2c_get_root()
{
    char* t2c_root = NULL;
    char* tmp = getenv(t2c_env_name);
    
    if (tmp)
    {
        int len = strlen(tmp);
        if ((len == 0) || (tmp[len - 1] != '/'))
        {
            t2c_root = str_sum(tmp, "/");
        }
        else
        {
            t2c_root = strdup(tmp);
        }
    }
    else // no such environment variable exists, root directory is assumed to be T2C_ROOT
    {
        t2c_root = strdup("/");
    }
      
    return t2c_root;  
}

char* 
t2c_get_suite_root()
{
    char* t2c_suite_root = NULL;
    char* tmp = getenv(t2c_env_suite_name);
    
    if (tmp)
    {
        int len = strlen(tmp);
        if ((len == 0) || (tmp[len - 1] != '/'))
        {
            t2c_suite_root = str_sum(tmp, "/");
        }
        else
        {
            t2c_suite_root = strdup(tmp);
        }
    }
    else // T2C_SUITE_ROOT is not defined, use T2C_ROOT (or '/' - see t2c_get_root) 
    {
        t2c_suite_root = t2c_get_root();
    }
      
    return t2c_suite_root;  
}

char* 
t2c_get_path(const char* rel_path)
{
    if (!rel_path)
    {
        t2c_util_error("Invalid relative path passed to t2c_get_path().");
        return NULL;
    }
    
    char* path = t2c_get_suite_root();
    path = str_append(path, rel_path);
    
    int len = strlen(path);
    if (path[len - 1] != '/')
    {
        path = str_append(path, "/");
    }
    
    char* res = shorten_path(path);
    free(path);
    
    return res;
}

int 
t2c_root_defined()
{
    return (getenv(t2c_env_name) != NULL);
}

char*
t2c_get_data_path(const char* suite_subdir, const char* test_name, 
                   const char* rel_path)
{
    char* path0 = concat_paths((char*) suite_subdir, "testdata");
    char* path1 = concat_paths(path0, (char*) test_name);
    char* path2 = concat_paths(path1, (char*) rel_path);
    
    char* res = t2c_get_path(path2);
    
    free(path0);
    free(path1);
    free(path2);
    
    return res;
}

char*
t2c_get_rcat_path(const char* suite_subdir, const char* rcat_name)
{
    char* path0 = concat_paths((char*) suite_subdir, "reqs");
    char* full_name = str_sum(rcat_name, ".xml");
    char* path1 = concat_paths(path0, full_name);
    
    char* res = t2c_get_path(path1);
    
    free(path0);
    free(path1);
    free(full_name);
    
    return res;
}

//////////////////////////////////////////////////////////////////////////
char*
t2c_first_not_of(char* str, const char* seps)
{
    char* pos = str;
    size_t nseps = strlen(seps);
    size_t len = strlen(str);

    int bSkip = 1;
    size_t i, j;
    for (i = 0; i < len; ++i)
    {
        bSkip = 0;
        for (j = 0; j < nseps; ++j)
        {
            if (str[i] == seps[j])
            {
                bSkip = 1;
                break;
            }
        }

        if (!bSkip)
        {
            break;
        }
        ++pos;
    }

    return pos;
}

char*
t2c_first_of(char* str, const char* seps)
{
    char* pos = str;
    size_t nseps = strlen(seps);
    size_t len = strlen(str);

    size_t i, j;
    for (i = 0; i < len; ++i)
    {
        int bSkip = 0;
        for (j = 0; j < nseps; ++j)
        {
            if (str[i] == seps[j])
            {
                bSkip = 1;
                break;
            }
        }

        if (bSkip)
        {
            break;
        }
        ++pos;
    }

    return pos;
}

//////////////////////////////////////////////////////////////////////////
// tag & attribute parsing - implementation
int 
t2c_parse_open_tag(const char* str, char* known_tags[], char** attr_str)
{
    char* tstr = strdup(str);
    char* wspace = " \t\n\r";
    char* seps1 = "> \t\n\r";

    int index = -1;
    if (attr_str)
    {
        free(*attr_str);
        *attr_str = NULL;
    }
    
    char* fin = NULL;
    char* pos = t2c_first_not_of(tstr, wspace);
    if (*pos != 0)
    {
        size_t len = strlen(pos);
        if ((len > 1) && (pos[0] == '<'))
        {
            pos = t2c_first_not_of(&pos[1], wspace);    // get to the tag's name
            if (*pos != 0)  // tag's name found
            {
                fin = t2c_first_of(pos, seps1);
                if (*fin != 0) // at least '>' should exist
                {
                    char finChar = *fin;
                    *fin = 0;   // get the name ('pos' now holds it)
                    
                    // check if we know this name
                    int bKnown = 0;
                    int i;
                    for (i = 0; known_tags[i] != NULL; ++i)
                    {
                        if (!strcmp(pos, known_tags[i]))
                        {
                            bKnown = 1;
                            index = i;
                            break;
                        }
                    }
                    
                    if (bKnown)
                    {
                        // Known tag found
                        if (finChar != '>')
                        {
                            pos = t2c_first_not_of(&fin[1], wspace);
                            if (*pos != '>')
                            {
                                //fin = t2c_first_of(pos, ">");
                                fin = strchr(pos, '>');
                                if (fin == NULL)
                                {
                                    index = -1; // No '>' found => ERROR!
                                }
                                else
                                {   
                                    // check if smth remains behind '>'
                                    char* p1 = t2c_first_not_of(&fin[1], wspace);
                                    if (*p1 != 0)
                                    {
                                        index = -1;
                                    }
                                    else
                                    {
                                        if (attr_str)
                                        {
                                            *fin = 0;
                                            *attr_str = strdup(pos);
                                        }
                                    }

                                }
                            }
                        }
                        else
                        {
                            // check if smth remains behind '>'
                            char* p1 = t2c_first_not_of(&fin[1], wspace);
                            if (*p1 != 0)
                            {
                                index = -1;
                            }
                        }
                    }
                }
            }
        }
    }

    free(tstr);
    return index;
}

// Returns index if attribute in 'attr_names' if the attribute inis known, -1 otherwise.
int
t2c_get_attr_index(const char* name, char* attr_names[])
{
    int ind = -1;
    int i;
    for (i = 0; attr_names[i] != NULL; ++i)
    {
        if (!strcmp(name, attr_names[i]))
        {
            ind = i;
            break;
        }
    }

    return ind;
}

int 
t2c_parse_attributes(const char* str, char* attr_names[], char* attr_values[])
{
    if (!str)
    {
        return 1; // OK, nothing to do.
    }
    
    int bOK = 1;
    char* wspace = " \t\n\r";
    char* seps1  = "= \t";

    // first clean attr_values.
    int i;
    for (i = 0; attr_names[i] != NULL; ++i)
    {
        free(attr_values[i]);
        attr_values[i] = NULL;
    }
    
    char* tstr = strdup(str); // we operate on a copy of the string
    
    char* pos = t2c_first_not_of(tstr, wspace);
    char* fin = NULL;
    while (*pos != 0)
    {
        fin = t2c_first_of(pos, seps1);
        char finChar = *fin;

        if (finChar == 0) // unexpected EOL
        {
            bOK = 0;
            break;
        }

        // got attribute name
        *fin = 0;
        int ind = t2c_get_attr_index(pos, attr_names); // we'll need it later
        *fin = finChar;

        pos = t2c_first_not_of(fin, wspace); // must be '='
        if (*pos != '=')
        {
            bOK = 0;
            break;
        }

        pos = t2c_first_not_of(&pos[1], wspace); // must be '\"' or '\''
        char quot = *pos;
        if ((quot != '\"') && (quot != '\''))
        {
            bOK = 0;
            break;
        }

        //&pos[1] now points to the value of the attribute
        pos = &pos[1];

        char strQuot[2];
        strQuot[0] = quot;
        strQuot[1] = 0;
        
        // look for a closing quote of the same kind
        fin = strchr(pos, quot);
        if (*fin == 0)
        {
            bOK = 0;
            break;
        }
        
        *fin = 0;
        if (ind != -1)
        {
            attr_values[ind] = strdup(pos); 
        }

        pos = t2c_first_not_of(&fin[1], wspace);
    }

    free(tstr);
    return bOK;
}

int 
t2c_parse_close_tag(const char* str, const char* name)
{
    int bOK = 0;

    char* tstr = strdup(str);
    char* wspace = " \t\n\r";
    
    char* pos = t2c_first_not_of(tstr, wspace);
    char* fin = NULL;
    
    if ((pos[0] == '<') && (pos[1] == '/') && (pos[2] != '>'))
    {
        pos = &pos[2];
        fin = strchr(pos, '>');
        
        if (fin != NULL)
        {
            *fin = 0;
            if (!strcmp(pos, name))
            {
                pos = t2c_first_not_of(&fin[1], wspace);
                if (*pos == 0)
                {
                    bOK = 1;
                }
            }
        }
    }

    free(tstr);
    return bOK;
}

int 
t2c_parse_decl(const char* str)
{
    int bOK = 0;

    char* tstr = strdup(str);
    char* wspace = " \t\n\r";

    char* pos = t2c_first_not_of(tstr, wspace);

    if ((pos[0] == '<') && (pos[1] == '?'))
    {
        pos = t2c_first_not_of(&pos[2], wspace);
        if ((pos[0] == 'x') && (pos[1] == 'm') && (pos[2] == 'l'))
        {
            pos = strchr(&pos[3], '>');
            if (pos && (*(pos - 1) == '?'))
            {
                pos = t2c_first_not_of(&pos[1], wspace);
                if (*pos == 0)
                {
                    bOK = 1;
                }
            }
        }
    }
    
    free(tstr);

    return bOK;
}

char*
t2c_unreplace_special_chars(char* str)
{
    char* rwhat[] = {"&quot;", "&apos;", "&lt;", "&gt;", "&amp;"};
    char* rwith[] = {"\"", "\'", "<", ">", "&"};
    
    int i;
    for (i = 0; i < sizeof(rwhat) / sizeof(rwhat[0]); ++i)
    {
        str = replace_all_substr_in_string(str, rwhat[i], rwith[i]);
    }

    return str;
}

//////////////////////////////////////////////////////////////////////////
TReqInfoPtr
t2c_req_info_new(const char* rid, const char* text)
{
    if (!rid || !text)
    {
        return NULL;
    }

    TReqInfoPtr ri = (TReqInfoPtr)malloc(sizeof(TReqInfo));
    if (!ri)
    {
        return NULL;
    }

    if (!(ri->rid = strdup(rid)))
    {
        free(ri);
        return NULL;
    }

    if (!(ri->text = strdup(text)))
    {
        free(ri->rid);
        free(ri);
        return NULL;
    }

    return ri;
}

void
t2c_req_info_delete(TReqInfoPtr ri)
{
    if (!ri) 
    {
        return;
    }

    free(ri->rid);
    free(ri->text);
    free(ri);
}

//////////////////////////////////////////////////////////////////////////
void
t2c_req_info_list_clear(TReqInfoList* head)
{
    TReqInfoList* p = head;
    for (; p != NULL; head = p)
    {
        p = head->next;
        t2c_req_info_delete(head->ri);
        free(head);
    }
}

int
t2c_req_info_list_length(TReqInfoList* head)
{
    int len = 0;
    for (; head != NULL; head = head->next)
    {
        ++len;
    }
    return len;
}

TReqInfoList* 
t2c_req_info_list_append(TReqInfoList* tail, const char* rid, const char* text)
{
    TReqInfoPtr ri = t2c_req_info_new(rid, text);
    if (!ri)
    {
        return NULL;
    }

    TReqInfoList* node = (TReqInfoList*)malloc(sizeof(TReqInfoList));
    if (!node)
    {
        t2c_req_info_delete(ri);
        return NULL;
    }

    node->ri = ri;
    node->next = NULL;

    if (tail)
    {
        tail->next = node;
    }
    return node;
}

TReqInfoPtr* 
t2c_req_info_list_to_array(TReqInfoList* head, int* len)
{
    if (!head || !len)
    {
        return NULL;
    }

    TReqInfoList* lst = head->next;
    int n = t2c_req_info_list_length(lst);
    *len = n;
    
    TReqInfoPtr* arr = (TReqInfoPtr*)malloc(n * sizeof(TReqInfoPtr));
    
    int i;
    for (i = 0; i < n; ++i)
    {
        arr[i] = lst->ri;
        lst = lst->next;
    }

    return arr;
}

int
t2c_rcat_load_impl(FILE* fd, TReqInfoList** ptail)
{
    int bOK = 1;
    int nLine = 0;  // current line of file
    TReqInfoList* tail = *ptail;

    char* root_tag[] = {"requirements", NULL};
    char* req_tag[]  = {"req", NULL};

    char* attr_id_name[] = {"id", NULL};
    char* attr_id_val[]  = {NULL, NULL};

    char* attribs = NULL;

    int mode = T2C_RCAT_HDR_DECL;
    
    // allocate a buffer string 
    int sz = fsize(fd);
    char* str = NULL;
    str = alloc_mem_for_string(str, sz + 1);

    int root_opened = 0;
    int root_closed = 0;

    char* rtext = NULL;

    char* pos;
    
    do
    {
        fgets(str, sz, fd);
        ++nLine;
        if (ferror(fd))
        {
            bOK = 0;
            fprintf(stderr, "Line %d: An unspecified error occured while reading the file.\n", nLine);
            break;
        }
                
        // Process the line here
        pos = t2c_first_not_of(str, " \t\n\r");

        bOK = 1;
        size_t num;
        if (*pos != 0)
        {
            switch (mode)
            {
            case T2C_RCAT_STOP:
                break;

            case T2C_RCAT_HDR_DECL: // header decl
                if (root_opened || root_closed)
                {
                    bOK = 0;
                    fprintf(stderr, "Line %d: Unexpected header decl found: %s\n", nLine, pos);
                    break;
                }

                bOK = t2c_parse_decl(pos);
                if (!bOK)
                {
                    fprintf(stderr, "Line %d: Invalid header decl: %s\n", nLine, pos);
                }
                else
                {
                    mode = T2C_RCAT_OPEN_TAG;
                }
                break;

            case T2C_RCAT_OPEN_TAG:  // open tag
                free(rtext);
                rtext = strdup("");  // we'll append text to this string later
                    
                if (root_closed)
                {
                    bOK = 0;
                    fprintf(stderr, "Unexpected tag found after the closing root tag: %s\n", pos);
                    break;
                }

                if (!root_opened)
                {
                    int ind = t2c_parse_open_tag(pos, root_tag, NULL);
                    if (ind != 0)
                    {
                        bOK = 0;
                        fprintf(stderr, "Line %d: No root tag found\n", nLine);
                        break;
                    }

                    root_opened = 1;
                    mode = T2C_RCAT_OPEN_TAG;
                }
                else
                {
                    // first try to parse it as the closing root tag.
                    root_closed = t2c_parse_close_tag(pos, root_tag[0]);
                    if (root_closed)
                    {
                        mode = T2C_RCAT_STOP;
                        break;
                    }

                    int ind = t2c_parse_open_tag(pos, req_tag, &attribs);
                    if (ind != 0)
                    {
                        bOK = 0;
                        fprintf(stderr, "Line %d: Invalid req tag found: %s\n", nLine, pos);
                        break;
                    }

                    if (attribs == NULL)
                    {
                        bOK = 0;
                        fprintf(stderr, "Line %d: REQ id is not specified in %s\n", nLine, pos);
                        break;
                    }

                    bOK = t2c_parse_attributes(attribs, attr_id_name, attr_id_val);
                    if (!bOK || !attr_id_val[0])
                    {
                        bOK = 0;
                        fprintf(stderr, "Line %d: REQ id is not specified in %s\n", nLine, pos);
                        break;
                    }

                    mode = T2C_RCAT_BODY;
                }
                break;

            case T2C_RCAT_BODY:      // body
                // First check if we are at the beginning of the new tag
                if (pos[0] == '<')
                {
                    // Create a list item with text accumulated in rtext
                    if (strlen(rtext) == 0)
                    {
                        fprintf(stderr, "Line %d: WARNING: Text is empty for the \"%s\" requirement\n", nLine, attr_id_val[0]);
                    }
                    attr_id_val[0] = t2c_unreplace_special_chars(attr_id_val[0]);
                    rtext = t2c_unreplace_special_chars(rtext);
                    tail = t2c_req_info_list_append(tail, attr_id_val[0], rtext);
                    
                    // Try to parse a closing tag
                    bOK = t2c_parse_close_tag(pos, req_tag[0]);
                    if (!bOK)
                    {
                        fprintf(stderr, "Line %d: Expected closing req tag, found: %s\n", nLine, pos);
                        mode = T2C_RCAT_STOP;
                    }
                    else
                    {
                        mode = T2C_RCAT_OPEN_TAG;
                    }
                    break;
                }
                else
                {
                // Append the string to rtext
                    // Remove trailing newline
                    num = strlen(pos);
                    if ((num > 0) && ((pos[num - 1] == '\n') || (pos[num - 1] == '\r')))
                    {
                        pos[num - 1] = 0;
                    }

                    if ((num > 1) && ((pos[num - 2] == '\n') || (pos[num - 2] == '\r')))
                    {
                        pos[num - 2] = 0;
                    }

                    rtext = str_append(rtext, pos);
                    rtext = str_append(rtext, " ");
                    
                    // Mode is not changed: mode = T2C_RCAT_BODY;
                }
        
                break;

            default:
                // shouldn't get here
                fprintf(stderr, "Invalid mode: %d\n", mode);
                break;
            }
        }

        if (!bOK) break;
    } while(!feof(fd));

    free(rtext);
    free(str);

    // cleanup
    int i;
    for (i = 0; i < sizeof(attr_id_val)/sizeof(attr_id_val[0]) - 1; ++i)
    {
        free(attr_id_val[i]);
    }

    free(attribs);
    
    *ptail = tail;
    
    return bOK;
}

// A comparator function for pointers to TReqInfo structures.
int
t2c_req_info_compare(const void* lhs, const void* rhs)
{
    TReqInfoPtr pleft  = *((TReqInfoPtr *)lhs);
    TReqInfoPtr pright = *((TReqInfoPtr *)rhs);

    return strcmp(pleft->rid, pright->rid);
}

const char* 
t2c_rcat_find(const char* rid, TReqInfoPtr reqs[], int nreq)
{
    if (!reqs || !rid)
    {
        return NULL;
    }

    TReqInfo* tmp = t2c_req_info_new(rid, "");
    if (!tmp)
    {
        return NULL;
    }
    
    TReqInfoPtr* pri = (TReqInfoPtr *)bsearch(
        (void *)(&tmp), 
        (void *)reqs, 
        nreq, 
        sizeof(TReqInfoPtr), 
        t2c_req_info_compare);
    
    t2c_req_info_delete(tmp);

    return (pri) ? (*pri)->text : NULL;
}

void 
t2c_rcat_sort(TReqInfoPtr reqs[], int nreq)
{
    if (!reqs)
    {
        return;
    }

    qsort((void *)reqs, 
        nreq, 
        sizeof(TReqInfoPtr), 
        t2c_req_info_compare);

    return;
}

ERcatLoadCode 
t2c_rcat_load(const char* rcat_names[], const char* suite_subdir, 
    TReqInfoList** phead, TReqInfoPtr** preqs, int* nreq)
{
    int bOK = 1; // no error by default
    if (!phead || !preqs || !nreq || !rcat_names ||!suite_subdir) 
    {
        fprintf(stderr, "t2c_rcat_load(): invalid parameters passed\n");
        exit(1);
    }
    
    if (rcat_names[0] == NULL)
    {
        fprintf(stderr, "t2c_rcat_load(): the list of req catalogs is empty\n");
        exit(1);
    }
    
    *phead = t2c_req_info_list_append(NULL, "", "");    // create a head of the list.
    if (*phead == NULL)
    {
        fprintf(stderr, "t2c_rcat_load(): unable to initialize requirement list.\n");
        return T2C_RCAT_BAD_RCAT;
    }
    
    // Walk the list of names and load appropriate req catalogs
    TReqInfoList* tail = *phead;
    int found = 0;
    for (int i = 0; rcat_names[i] != NULL; ++i)
    {   
        char* rcpath = t2c_get_rcat_path(suite_subdir, rcat_names[i]);
        FILE* fd = fopen(rcpath, "r");
        if (!fd)
        {
            //fprintf(stderr, "Unable to open file %s\n", rcpath);
            free(rcpath);
            continue;
        }
        
        found = 1;  // a file has been found and opened.
        fprintf(stderr, "Loading requirement catalog: %s\n", rcpath);
        int iret = t2c_rcat_load_impl(fd, &tail);
        if (!iret)
        {
            bOK = T2C_RCAT_BAD_RCAT;
            fprintf(stderr, "Invalid requirement catalog: %s\n", rcpath);
            free(rcpath);
            fclose(fd);    
            break;
        }
        else
        {
            bOK = T2C_RCAT_LOAD_OK;
        }
        
        free(rcpath);
        fclose(fd);
    }
    
    if (!found)
    {
        bOK = T2C_RCAT_NO_RCAT;
//        fprintf(stderr, "None of the files containing requirement catalogs could be opened.\n");
    }
    
    if (bOK == T2C_RCAT_LOAD_OK)
    {
        *preqs = t2c_req_info_list_to_array(*phead, nreq);
        if (!(*preqs))
        {
            bOK = T2C_RCAT_BAD_RCAT;
            fprintf(stderr, "Unable to create the requirement list.\n");
        }
    }
    
    return bOK;
}

// the end
