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

#include "../include/t2c_tet_support.h"
#include "../include/t2c_trace.h"
   
// Max number of symbols allowed to be inserted in the href template (see t2c_req_impl).
#define NSYM_FOR_HREF   1024

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

void 
t2c_checked_req_out(const char* r_id, int bVerbose)
{
    char* rlist = strdup(r_id); // make a working copy
    if (!rlist)
    {
        fprintf(stderr, "t2c_checked_req_out(): strdup(r_id) failed\n");
        return;
    }

    // split the list into IDs    
    char *seps   = " ;\t";
    char* rid = strtok(rlist, seps);
    while (rid != NULL)
    {
        TRACE("Checked requirement: {%s}", rid);      
        
        // Get the next ID.
        rid = strtok(NULL, seps);
    }

    free(rlist);
    return;
}



// A replacement for tet_printf that takes 'const char*' instead of 'char*'
int
t2c_printf(const char* format, ...)
{
    int res = -1;
    char* fmt = NULL;
    va_list ap;
    
    fmt = strdup(format);
        
    va_start(ap, format);
    res = tet_vprintf(fmt, ap);
    va_end(ap);
    
    free(fmt);
    
    return res;
}

// the end
