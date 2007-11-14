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

/*#include "libmem.h"
#include "libstr.h"
#include "libfile.h"*/

#include "../include/t2c_util.h"
#include "../include/t2c_trace.h"
    
// Max number of symbols allowed to be inserted in the href template (see t2c_req_impl).
#define NSYM_FOR_HREF   1024

// Sends the specified error message from the T2C util library to stderr.
static void 
t2c_util_error(const char* msg);

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const char* t2c_env_name        = "T2C_ROOT";
const char* t2c_env_suite_name  = "T2C_SUITE_ROOT";

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
t2c_get_rcat_path(const char* suite_subdir, const char* test_name)
{
    char* path0 = concat_paths((char*) suite_subdir, "reqs");
    char* rcat_name = str_sum(test_name, ".xml");
    char* path1 = concat_paths(path0, rcat_name);
    
    char* res = t2c_get_path(path1);
    
    free(path0);
    free(path1);
    free(rcat_name);
    
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
t2c_rcat_load(const char* rcpath, TReqInfoList** phead, TReqInfoPtr** preqs, int* nreq)
{
    FILE* fd = fopen(rcpath, "r");
    if (!fd)
    {
        fprintf(stderr, "Unable to open file %s\n", rcpath);
        return 0;
    }

    *phead = t2c_req_info_list_append(NULL, "", "");    // create a fictive head of the list.
    if (*phead == NULL)
    {
        fprintf(stderr, "Unable to initialize the requirement list.\n");
        fclose(fd);
        return 0;
    }

    int bOK = 1;
    TReqInfoList* tail = *phead;

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
        if (ferror(fd))
        {
            bOK = 0;
            fprintf(stderr, "An error occured while reading from %s.\n", rcpath);
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
                    fprintf(stderr, "Unexpected header decl found: %s\n", pos);
                    break;
                }

                bOK = t2c_parse_decl(pos);
                if (!bOK)
                {
                    fprintf(stderr, "Invalid header decl: %s\n", pos);
                }
                else
                {
                    mode = T2C_RCAT_OPEN_TAG;
                }
                break;

            case T2C_RCAT_OPEN_TAG:  // open tag
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
                        fprintf(stderr, "No root tag found\n");
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
                        fprintf(stderr, "Invalid req tag found: %s\n", pos);
                        break;
                    }

                    if (attribs == NULL)
                    {
                        bOK = 0;
                        fprintf(stderr, "REQ id is not specified in %s\n", pos);
                        break;
                    }

                    bOK = t2c_parse_attributes(attribs, attr_id_name, attr_id_val);
                    if (!bOK || !attr_id_val[0])
                    {
                        bOK = 0;
                        fprintf(stderr, "REQ id is not specified in %s\n", pos);
                        break;
                    }

                    mode = T2C_RCAT_BODY;
                }
                break;

            case T2C_RCAT_BODY:      // body
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

                // Create a new list element
                attr_id_val[0] = t2c_unreplace_special_chars(attr_id_val[0]);

                rtext = strdup(pos);
                rtext = t2c_unreplace_special_chars(rtext);

                tail = t2c_req_info_list_append(tail, attr_id_val[0], rtext);
                free(rtext);

                mode = T2C_RCAT_CLOSE_TAG;
                break;

            case T2C_RCAT_CLOSE_TAG: // close tag
                bOK = t2c_parse_close_tag(pos, req_tag[0]);
                if (!bOK)
                {
                    fprintf(stderr, "Expected closing req tag, found: %s\n", pos);
                    mode = T2C_RCAT_STOP;
                }
                else
                {
                    mode = T2C_RCAT_OPEN_TAG;
                }
                break;

            default:
                // shouldn't get here
                fprintf(stderr, "Invalid mode: %d\n", mode);
                break;
            }
        }

        if (!bOK) break;

        //fgets(str, sz, fd);
    } while(!feof(fd));

    free(str);
    fclose(fd);

    if (bOK)
    {
        *preqs = t2c_req_info_list_to_array(*phead, nreq);
        if (!(*preqs))
        {
            bOK = 0;
            fprintf(stderr, "Unable to create the requirement list.\n");
        }
    }

    // cleanup
    int i;
    for (i = 0; i < sizeof(attr_id_val)/sizeof(attr_id_val[0]) - 1; ++i)
    {
        free(attr_id_val[i]);
    }

    free(attribs);

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

/////////////////////////////////////////////////////////////////////////////

void 
t2c_req_impl(const char* r_id, const char* r_comment, 
             TReqInfoPtr reqs[], int nreq,
             const char* fname, int line, 
             int bVerbose,
             int gen_hlinks,
             const char* href_tpl)
{
    const char* str_empty = "<<< REQ NOT FOUND >>>";
    const char* href_end = "</a>{/noreplace}";
    char* href_buf = NULL;
    
    href_buf = (char*)malloc((strlen(href_tpl) + strlen(href_end) + 1 + NSYM_FOR_HREF) * sizeof(char));
        
    int bApp = 0;
    
    TRACE_NO_JOURNAL("File: %s, line: %d.", fname, line);
    if (r_comment[0] != 0) 
    {
        TRACE0(r_comment);
    }

    if(!strchr(r_id, ';')) // if there is only one requirement in the list
    {
        TRACE0(" ");
        TRACE0("Requirement failed:");
        
    } 
    else // the list consists of 2 or more requirements
    {
        TRACE0(" ");
        TRACE0("At least one of the following requirements failed:");
        TRACE0(" ");
    }
    
    // split the list into single IDs and process them one by one.
    
    char* rlist = strdup(r_id); // a working copy
    char *seps   = " ;\t";
    
    char* rid = strtok(rlist, seps);
    while (rid != NULL)
    {
        // Process this ID.
        if (strstr(rid, "app.")) // application requirement
        {
            bApp = 1;
        }
        
        // Get the requirement text.
        const char* txt = t2c_rcat_find(rid, reqs, nreq);
        if (!txt) 
        {
            txt = str_empty;
        }
        
        if (gen_hlinks) 
        {
            sprintf(href_buf, href_tpl, "req", line);
            strcat(href_buf, rid);
            strcat(href_buf, "</a>{/noreplace}");
        }
        else
        {
            strcpy(href_buf, rid);
        }
        
        TRACE("{%s}", href_buf);
        TRACE("%s", txt);
        TRACE0(" ");
        
        // Get the next req.
        rid = strtok(NULL, seps);
    }
  
    if (bApp)   // there is at least one "app"-requirement
    {
        TRACE0("One or more application requirements failed.");
        TRACE0("There might be a bug in the test itself. Please report this to the test developer.");
        tet_result(TET_UNRESOLVED);
    }
    else
    {
        tet_result(TET_FAIL);
    }
    
    free(href_buf);
    free(rlist);
    return;
}            
// end
