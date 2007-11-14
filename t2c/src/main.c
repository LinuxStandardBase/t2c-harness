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
This file is a part of the 'T2C' test generation framework.

The argv argument contains the following (the paths are considered relative to 
$T2C_SUITE_ROOT):
argv[0] - not used;
argv[1] - path to the main test suite dir (here the 'tet_scen' file resides),
    e.g. "TestSuites/my_suites";
argv[2] - path to the directory of a test suite to generate (<test_dir>), 
    e.g. "TestSuites/my_suites/myfirst-t2c" 
    The code generator will look for input files (*.t2c) in  <test_dir>/src/<...>/
    The C-files and makefiles for the tests will be created in 
    <test_dir>/tests/<section_name>/, scenario files - in <test_dir>/scenarios/.
argv[3] - [optional] path to the configuration file that contains additional 
    options of the generator (see the documentation for a more detailed description).

Example: 
    t2c my_suites my_suites/myfirst-t2c my_suites/conf/myfirst.cfg
******************************************************************************/
 
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/t2c_util.h"

/*******************************************************************************/
#define NCMD_PARAMS 3       // Number of mandatory command line parameters to be specified
#define ARGV_TSDIR_INDEX 2  // argv[ARGV_TSDIR_INDEX] is a path test suite dir to be processed.
#define ARGV_PARAM_INDEX 3  // argv[ARGV_PARAM_INDEX] is a path to the config file.

#define PATH_LEN 1024       // Maximum path length allowed

#define INPUT_EXT          ".t2c"        // input file extention
#define INPUT_EXT_OLD      ".ras"        // old input file extention

/************************************************************************/
// Known top-level tags.
static char* top_tag[] = {
    "GLOBAL",
    "STARTUP",
    "CLEANUP",    
    "BLOCK",
    NULL
};

// Top-level tag IDs:
#define ITOP_GLOBAL     0
#define ITOP_STARTUP    1
#define ITOP_CLEANUP    2
#define ITOP_BLOCK      3
    
// Know sub-tags in the <BLOCK> tag:
static char* bl_tag[] = {
    "TARGETS",
    "DEFINE",
    "FINALLY",    
    "CODE",
    "PURPOSE",
    NULL
};

// Block-level tag IDs:
#define IBL_TARGETS     0
#define IBL_DEFINE      1
#define IBL_FINALLY     2
#define IBL_CODE        3
#define IBL_PURPOSE     4

/************************************************************************/
#define CFG_PARAMS 5        // Number of parameters that can be specified in a .cfg-file.
#define CFG_COMPILER_POS    0
#define CFG_COMP_FLAGS_POS  1
#define CFG_LINK_FLAGS_POS  2
#define CFG_TET_RECORD_POS  3
#define CFG_WAIT_TIME_POS   4

// Names of the parameters from .cfg-file
static char* cfg_parm_names[CFG_PARAMS] = {
    "COMPILER",
    "COMPILER_FLAGS",
    "LINKER_FLAGS",
    "TET_SCEN_RECORD",
    "WAIT_TIME"
};

/* Values of the parameters from .cfg-file. Use set_defaults() to reset these to their
 * default values.
 */
static char* cfg_parm_values[CFG_PARAMS] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

// 1 corresponds to TET_SCEN_RECORD=yes in the config. file, 0 is for the 
// opposite case.
int bAddTetScenRecord = 1;

/************************************************************************/
#define COMMON_TEST_TPL    "test.tpl"    /* common test case template*/
#define COMMON_PURPOSE_TPL "purpose.tpl" /* common test purpose template*/
#define COMMON_TPL_DIR     "/t2c/src/templates/"

#define COMMON_TAGS_NUM     14
#define GROUP_NAME_POS      0
#define OBJECT_NAME_POS     1
#define TEST_PURPOSES_POS   2
#define TET_HOOKS_POS       3
#define YEAR_POS            4
#define LIBRARY_POS         5
#define LIBSECTION_POS      6
#define GLOBALS_POS         7
#define STARTUP_POS         8
#define CLEANUP_POS         9
#define FTEMPLATE_POS       10
#define TP_FUNCS_POS        11
#define PCF_FUNCS_POS       12
#define WAIT_TIME_POS       13

static char* common_tags[COMMON_TAGS_NUM] = { 
    "<%group_name%>",
    "<%object_name%>",
    "<%test_purposes%>",
    "<%tet_hooks%>",
    "<%year%>",
    "<%library%>",
    "<%libsection%>",
    "<%globals%>",
    "<%startup%>",
    "<%cleanup%>",
    "<%ftemplate%>",
    "<%tp_funcs%>",
    "<%pcf_funcs%>",
    "<%wait_time%>"
};
    
#define MAX_PARAMS_NUM  256
#define MAX_TARGETS_NUM 256
    
#define TAGS_NUM 6
#define COMMENT_TAG_POS  0
#define DEFINE_TAG_POS   1
#define CODE_TAG_POS     2
#define PURPNUM_TAG_POS  3
#define UNDEF_TAG_POS    4
#define TARGETS_TAG_POS  5
    
static char* tags[] = { 
    "<%comment%>",
    "<%define%>",
    "<%code%>",
    "<%purpose_number%>",
    "<%undef%>",
    "<%targets%>"
};

#define TAG_FINALLY_NAME "<%finally%>"
#define TAG_SUITE_SUBDIR_NAME "<%suite_subdir%>"

char* func_tests_scenario_file_name = "func_scen";

/* Number of params to be extracted from the header of a .t2c-file  */
#define HEADER_PARAMS_NUM    2     /*library, libsection*/
#define PARAM_LIB_POS        0
#define PARAM_SECTION_POS    1

char* hdr_param_value[HEADER_PARAMS_NUM] = { NULL, NULL };
char* hdr_param_name[HEADER_PARAMS_NUM] = { "library", 
                                            "libsection", 
                                          };

// The values of $T2C_ROOT and $T2C_SUITE_ROOT.
char* t2c_root = NULL;
char* t2c_suite_root = NULL;

// A path (relative to $T2C_SUITE_ROOT) to the test suite subdirectory 
// (e.g. "desktop-t2c/glib-t2c")
char* test_dir = NULL;

/***********************************************************************/

static void  
run_generator(const char* suite_root, const char* input_dir, 
              const char* output_dir, const char* scen_dir);
static char*  
gen_tests_group(const char* suite_root, const char* output_dir, 
                const char* group_path, const char* group_nme, 
                const char* scen_file);
static char*  
gen_test(const char* test_tpl, const char* purpose_tpl, 
         const char* input_path, const char* group_nme, const char* test_nme);

static void
gen_tp_arrays(int number, char** pTetHooks, char** pTpFuncs);

static void   
add_test_in_scen(const char* suite_root, const char* scen_file, 
                 const char* output_dir, const char* ftest_nme);

static void 
gen_makefile(const char* suite_root, const char* dir_path, 
             const char* ftest_nme);

static void 
gen_common_makefile(const char* suite_root_path, const char* output_dir_path);

static char* 
fgets_and_skip_comments(char* str, int num, FILE* stream, int* str_counter);

static void 
usage();

/* 
Extract data from the T2C-file header (library, libsection etc.) and
store in hdr_data. hdr_param_names contains parameter names.
Returns 0 if the specified file cannot be opened, 1 otherwise. If some 
parameter is not found in the file, an empty string is returned as its value.
*/
static int 
parse_header(const char* input_path);

/*
Parse the specified file and save the extracted data in the strings (memory for those
will be allocated if necessary). Returns 1 on success, 0 in case of failure.
*/
static int
parse_file(const char* input_path, const char* purpose_tpl, int* purposes_number, const char* test_nme,
           char** pstrGlobals, char** pstrStartup,
           char** pstrCleanup, char** pstrPurposes,
           char** pcf_funcs);

/*
Return the contents of the global block. Returns NULL in case of error, the contents of
the block (as a string) otherwise.
size - max line size.
*/
static char*
parse_global(FILE* fl, int size);

static char*
parse_startup(FILE* fl, int size);

static char*
parse_cleanup(FILE* fl, int size);

/*
Return code for all purposes in the block, or NULL in case of error.
size - max line size.
*/
static char*
parse_block(FILE* fl, int size, const char* purpose_tpl, int* purposes_number,
            char** pcf_funcs, const char* pcf_name);

/*
Matches <TARGETS> section. On success, 1 is returned. If the section
is not found or is invalid (no closing tag, for example), 0 is returned.
*pstrTargets will be filled with target names, *pnTargets - with their
quantity.
*/
static int 
parse_targets(FILE* fl, int size, char** pstrTargets, int* pnTargets);

/*
Matches <DEFINES> section. On success, 1 is returned. If the section
is not found or is invalid (no closing tag, for example), 0 is returned.
On success, the contents of the section (as a string) are returned in *pstrDefines,
appropriate #undefs - in *pstrUndefs.
*pstrDefineNames will be filled with target names, *pnDefines - with their
quantity.
*/
static int
parse_defines(FILE* fl, int size, char** pstrDefines, char** pstrUndefs);

/* Reads the <CODE> section and returns its contents as a string. NULL is 
 * returned in case of error (if the section is invalid).
 */
static char*
parse_code(FILE* fl, int size);

/* Reads the <FINALLY> section and returns its contents as a string. NULL is 
 * returned in case of error (if the section is invalid).
 */
static char*
parse_finally(FILE* fl, int size);

/* Read a <PURPOSE> section and fill in the gaps of the template with
 * purpose number and parameter values. Returns the code of the purpose as
 * a string (on success), NULL in case of error.
 */
static char*
parse_purpose(FILE* fl, int size, const char* templ, const char* finally_code, int* purposes_number);

/*
 * Reset the parameters read from the configuration file 
 * to their default values.
 */
static void
set_defaults();

/*
 * Load data from the specified config file and fill the appropriate 
 * elements of cfg_parm_values array. If the loading fails, the execution of
 * the program will not be aborted, defaults will be used.
 * The path specified is considered relative to the current directory.
 */
static void 
load_config(const char* fpath);

/*
 * If the line has form "NAME = value", assign this value to the parameter
 * with the specified name (cfg_parm_values[...]).
 */
static void
load_parameter(const char* line);

static char*
replace_char(char* where, char from, char to)
{
    char* prval = strchr(where, from);
    while (prval)
    {
        *(prval) = to;
        prval = strchr(prval + 1, from);
    }
    
    return where;
}

/************************************************************************/
/* Main                                                                     */
/************************************************************************/
int 
main (int argc, char* argv[])
{
    int i;
    char* src_dir_end  = "src";
    char* out_dir_end  = "tests";
    char* scen_dir_end = "scenarios";    

    if (argc < NCMD_PARAMS)
    {
        usage();
        return 0;
    }
    else 
    {
        if (!t2c_root_defined())
        {
            fprintf(stderr, "T2C_ROOT environment variable is not defined. T2C is unable to continue.\n");
            return 1;
        }
        
        t2c_root = t2c_get_root();
        t2c_suite_root = t2c_get_suite_root();
        
        set_defaults();
        if (argc > NCMD_PARAMS)
        {
            /* process the specified config. file here */
            load_config(argv[ARGV_PARAM_INDEX]);
        }
    }
    
    test_dir = strdup(argv[ARGV_TSDIR_INDEX]);
    
    char* src_dir  = concat_paths(test_dir, src_dir_end);
    char* out_dir  = concat_paths(test_dir, out_dir_end);
    char* scen_dir = concat_paths(test_dir, scen_dir_end);
    
    run_generator(argv[1], src_dir, out_dir, scen_dir);
    
    free(src_dir);
    free(out_dir);
    free(scen_dir);

    for (i = 0; i < CFG_PARAMS; ++i)
    {
        free(cfg_parm_values[i]);
    }
    
    free(t2c_root);
    free(t2c_suite_root);
    free(test_dir);
    return 0;
}

/*******************************************************************************/

// Display a message on how to use this program.
static void
usage()
{
    fprintf(stderr, "\nThe T2C system generates C-sources for the tests from T2C templates.\n");
    fprintf(stderr, "T2C_ROOT environment variable should be defined before this program is executed.\n");
    fprintf(stderr, "\nUsage:\n");
    fprintf(stderr, "t2c <main_suite_dir> <test_dir> [cfg_path]\n");
    fprintf(stderr, "\n<main_suite_dir> - here the 'tet_scen' file resides\n");
    fprintf(stderr, "<test_dir> - path to the directory of a test suite to be processed,\n");
    fprintf(stderr, "   e.g. \"TestSuites/my_suites/myfirst-t2c\"\n");
    fprintf(stderr, "   The code generator will look for input files (*.t2c) in  <test_dir>/src/<...>/\n");
    fprintf(stderr, "   The C-files and makefiles for the tests will be created in\n");
    fprintf(stderr, "   <test_dir>/tests/<section_name>/, scenario files - in <test_dir>/scenarios/.\n");
    fprintf(stderr, "cfg_path - path to the configuration file (if not specified, the defaults will be used).\n");
    fprintf(stderr, "\nThe paths are relative to the $T2C_SUITE_ROOT directory.\n");
    fprintf(stderr, "If variable $T2C_SUITE_ROOT is not defined, $T2C_ROOT is used instead.\n");
    fprintf(stderr, "\nExample:\n    t2c my_suites my_suites/myfirst-t2c my_suites/conf/myfirst.cfg\n\n");
    return;
}

/*
 * Run gen_tests_group() for every tests group directory 
 * from inputdir_nme directory
 */
static void 
run_generator (const char* suite_root_in, const char* input_dir,
               const char* output_dir_in, const char* scen_dir_in)
{
    DIR*  pDir;
    FILE* ftet_scen = NULL;
    FILE* ffunc_scen = NULL;
    struct dirent* ep;
    char* output_dir = NULL;
    //char* makefile_dir = NULL;
    char* output_dir1 = NULL;
    char* input_path = NULL;
    char* input_path1 = NULL;
    char* input_path2 = NULL;
    char* ftet_scen_path = NULL;
    char* ftet_scen_path1 = NULL;
    char* ftet_scen_path2 = NULL;
    unsigned ftet_scen_exists = 0;
    char* pSave = NULL;
    char* scenario_file = NULL;
    char* path1 = NULL;
    char* suite_root = NULL;
    char* suite_root1 = NULL;
    char* scen_dir = NULL;
    char* func_scen_path = NULL;
    char* output_dir_path = NULL;
     
    suite_root1 = concat_paths (t2c_suite_root, (char*) suite_root_in);
    suite_root = shorten_path (suite_root1);

    path1 = concat_paths (suite_root, (char*) scen_dir_in);
 
    scen_dir = (char*) shorten_path (path1);

    if (!is_directory_exists ((char*) scen_dir))
    {
        create_dir ((char*) scen_dir);
    }

    scenario_file = alloc_mem_for_string (scenario_file, 
                                          strlen (scen_dir));
    scenario_file = str_append (scenario_file, scen_dir);
    scenario_file = str_append (scenario_file, "/");
    scenario_file = str_append (scenario_file, func_tests_scenario_file_name);

    /*
    Clear the func_scen file.
    */
    ffunc_scen = fopen (scenario_file, "w");
    if (!ffunc_scen)
    {
        fprintf (stderr, "Unable to open local scenario file: \n %s\n", scenario_file);
    }
    fclose (ffunc_scen);

    output_dir1 = concat_paths (suite_root, (char*) output_dir_in);
    output_dir = shorten_path (output_dir1);
    
    input_path1 = alloc_mem_for_string (input_path1, strlen (input_dir));
    input_path1 = str_append(input_path1, input_dir);
    
    input_path2 = concat_paths (suite_root, input_path1);
    input_path = shorten_path (input_path2);

    if (input_path[strlen (input_path) - 1] != '/')
    {
        input_path = str_append (input_path, "/");
    }

    if (bAddTetScenRecord)
    {
        ftet_scen_path1 = alloc_mem_for_string (ftet_scen_path1, 
            strlen (suite_root));
        ftet_scen_path1 = str_append (ftet_scen_path1, suite_root);

        ftet_scen_path = concat_paths (ftet_scen_path1, "/tet_scen");

        ftet_scen_exists = is_file_exists (ftet_scen_path);
        ftet_scen = open_file (ftet_scen_path, "a+", NULL);
        if (!ftet_scen_exists)
        {
            fprintf (ftet_scen, "all\n");
        }

        ftet_scen_path2 = alloc_mem_for_string (ftet_scen_path2, 
            strlen (scen_dir_in) + 1);

        if (scen_dir_in[0] != '/')
        {
            ftet_scen_path2 = str_append (ftet_scen_path2, "/");
        }
        ftet_scen_path2 = str_append (ftet_scen_path2, scen_dir_in);


        func_scen_path = concat_paths (ftet_scen_path2, "/func_scen");

        fprintf (ftet_scen, "\t:include:%s\n", func_scen_path);

        fclose (ftet_scen);
    }

    /* read directory names from input_dir directory */
    pDir = opendir (input_path);
    if (pDir == NULL)
    {
        error_with_exit_code (EXIT_CODE_CANT_OPEN_DIR,
                              "%s: can't open directory %s", "test generator", 
                              input_path);
    } 
    
    ep = readdir (pDir);
    while (ep != NULL)
    {
        char* group_path = NULL;
        group_path = str_sum (input_path, ep->d_name);

        if (is_directory_exists (group_path) && strcmp (ep->d_name, ".") 
            && strcmp (ep->d_name, "..") && strcmp (ep->d_name, "CVS")
            && strcmp (ep->d_name, ".bzr"))
        {
            pSave = group_path;
            if (group_path[strlen (group_path) - 1] != '/')  
            {
                group_path = str_append (group_path, "/");
            }
            pSave = NULL;
            gen_tests_group (suite_root, output_dir, /*makefile_dir,*/
                             group_path, ep->d_name, 
                             scenario_file);
        }
        ep = readdir (pDir);
        free (group_path);
    }

    /* generate common and global makefiles */
    /* form output dir path */
    output_dir_path = alloc_mem_for_string (output_dir_path, 
        strlen (output_dir));
    output_dir_path = str_append (output_dir_path, output_dir);
    if (output_dir_path[strlen (output_dir_path) - 1] != '/')
    {
        output_dir_path = str_append (output_dir_path, "/");
    }

    gen_common_makefile(suite_root, output_dir_path);

    free (scenario_file);
    free (output_dir);
    free (output_dir1);
    free (output_dir_path);
    free (input_path);
    free (input_path1);
    free (input_path2);
    free (suite_root);
    free (suite_root1);
    free (ftet_scen_path);
    free (ftet_scen_path1);
    free (pDir);
    free (path1);
    free (scen_dir);
}

/*
 * Read input files from input_dir, generates tests and makefiles, 
 * create directory where the tests and makefiles will be placed, 
 * add corresponding record in scen_file.
 *
 */
static char* 
gen_tests_group (const char* suite_root, const char* output_dir, 
                 //const char* makefile_dir,
                 const char* group_path, const char* group_nme, 
                 const char* scen_file)
{
    DIR*  pDir;
    struct dirent* ep;
    FILE* pFile;

    char* pSave = NULL;
    char* tpl_path = NULL;
    char* output_dir_path = NULL;

    char* test = NULL;
    char* test_tpl = NULL;
    char* purpose_tpl = NULL;
    
    /* common test template*/
    tpl_path = str_sum (group_path, COMMON_TEST_TPL);
    
    if (is_file_exists (tpl_path))
    {
        pFile = open_file (tpl_path, "r", NULL);
    } else {
        char* tpl_path1 = NULL;
        char* tpl_path2 = NULL;

        free (tpl_path);
        tpl_path1 = alloc_mem_for_string (tpl_path1, strlen(t2c_root)); 
        tpl_path1 = str_append (tpl_path1, t2c_root);
        tpl_path2 = concat_paths (tpl_path1, 
                                  COMMON_TPL_DIR);
        tpl_path = concat_paths (tpl_path2, COMMON_TEST_TPL);
        
        pFile = open_file (tpl_path, "r", NULL);

        free (tpl_path1);
        free (tpl_path2);
    }
    free (tpl_path);
    test_tpl = read_file_to_string (pFile);
    fclose (pFile);
    
    /* common purpose template */
    tpl_path = concat_paths ((char*) group_path, COMMON_PURPOSE_TPL);

    if (is_file_exists (tpl_path))
    {
        pFile = open_file (tpl_path, "r", NULL);
    } else {
        char* tpl_path1 = NULL;
        char* tpl_path2 = NULL;
        
        free (tpl_path);
        tpl_path1 = alloc_mem_for_string (tpl_path1, strlen (t2c_root)); 
        tpl_path1 = str_append (tpl_path1, t2c_root);
        tpl_path2 = concat_paths (tpl_path1, 
                                  COMMON_TPL_DIR);
        tpl_path = concat_paths (tpl_path2, COMMON_PURPOSE_TPL);
        pFile = open_file (tpl_path, "r", NULL);
    }
    purpose_tpl = read_file_to_string (pFile);
    free (tpl_path);
    fclose (pFile);

    /* form output dir path */
    output_dir_path = alloc_mem_for_string (output_dir_path, 
                                            strlen (output_dir));
    output_dir_path = str_append (output_dir_path, output_dir);
    if (output_dir_path[strlen (output_dir_path) - 1] != '/')
    {
        output_dir_path = str_append (output_dir_path, "/");
    }
    
    if (!is_directory_exists (output_dir_path))
    {
        create_dir (output_dir_path);        
    }
    
    /* read file names from group_path directory */
    pDir = opendir (group_path);
    if (pDir == NULL)
    {
        error_with_exit_code(EXIT_CODE_CANT_OPEN_DIR,
                             "%s: can't open directory %s", "test generator", 
                             group_path);
    } 

    ep = readdir (pDir);
    while (ep != NULL)
    {
        char* ftest_src = NULL;
        char* ftest_nme = NULL;
        char* input_path = NULL;
        char* output_path = NULL;
        int bOK = 1;

        ftest_nme = get_substr (ep->d_name, 0, 
                                strstr (ep->d_name, INPUT_EXT) - (ep->d_name + 1));
        if ((strcmp (ep->d_name + strlen (ep->d_name) - strlen (INPUT_EXT), INPUT_EXT)) 
            || (ftest_nme == NULL)) 
        {
            if (ftest_nme)
            {
                free (ftest_nme);
            }
            
            /* check if the file has the old-style file extension */ 
            ftest_nme = get_substr (ep->d_name, 0, 
                strstr (ep->d_name, INPUT_EXT_OLD) - (ep->d_name + 1));
            if ((strcmp (ep->d_name + strlen (ep->d_name) - strlen (INPUT_EXT_OLD), INPUT_EXT_OLD)) 
                || (ftest_nme == NULL)) 
            {
                ep = readdir (pDir);
                if (ftest_nme)
                {
                    free (ftest_nme);
                }
                continue;
            }
        }
        /* generate test string */
        input_path = str_sum (group_path, ep->d_name);
        
        /* parse header of the .t2c-file here */
        bOK = parse_header(input_path);

        if (!bOK)        
        {
            fprintf (stderr, "warning: invalid header in %s\n", input_path);
            free (input_path);
            free (ftest_nme);
            continue;
        }
        
        test = gen_test (test_tpl, purpose_tpl, input_path, group_nme, ftest_nme);
                         
        if (test == NULL)
        {
            ep = readdir (pDir);
            fprintf (stderr, "warning: test generator is unable to generate test for %s\n", input_path);
            free (input_path);
            free (ftest_nme);
            continue;
        }

        output_path = str_sum(output_dir_path, ftest_nme);
        if (output_path[strlen(output_path) - 1] != '/')
        {
            output_path = str_append(output_path, "/");
        }
        pSave = NULL;
        
        if (!is_directory_exists (output_path))
        {
            create_dir (output_path);        
        }

        /* store test in the appropriate file */

        gen_makefile (suite_root, output_path, ftest_nme);
        ftest_src = str_sum (ftest_nme, ".c");
        output_path = str_append (output_path, ftest_src);
        pFile = open_file (output_path, "w", NULL);

        fputs (test, pFile);
        add_test_in_scen (suite_root, scen_file, output_dir, ftest_nme);
        
        free (test);
        free (ftest_src);
        free (ftest_nme);
        free (input_path);
        free (output_path);
        fclose (pFile);

        ep = readdir (pDir);
    }

    (void) closedir (pDir);

    free (test_tpl);
    free (output_dir_path);
    //free (makefile_dir_path);
    free (purpose_tpl);
    return NULL;
}

/*
 *   Generates TET scen file
 */
static void 
add_test_in_scen (const char* suite_root, const char* scen_file, 
                  const char* output_dir, const char* ftest_nme)
{
    FILE* sf = NULL;
    char* scen_item = NULL;
    char* short_output_dir = NULL;

    if ( !scen_file || !output_dir || !ftest_nme)
    {
        return;
    }
    sf = open_file (scen_file, "a+", NULL);

    short_output_dir = replace_substr_in_string ((char*) output_dir, 
                                                 suite_root, "");
    scen_item = str_sum(scen_item, short_output_dir);
    if (short_output_dir[0] != '/')
    {
        char* p = scen_item;
        scen_item = str_sum("/", scen_item);
        free (p);
    }
    if (short_output_dir[strlen (short_output_dir) - 1] != '/')
    {
        scen_item = str_append (scen_item, "/");
    }
    scen_item = str_append (scen_item, ftest_nme);
    scen_item = str_append (scen_item, "/");
    scen_item = str_append (scen_item, ftest_nme);
    fprintf (sf, "\t%s\n", scen_item);
    
    free (scen_item);
    free (short_output_dir);
    fclose (sf);
}

/*
 * Generate a local makefile 
 */
static void 
gen_makefile (const char* suite_root, const char* dir_path, const char* ftest_nme)
{
    FILE* mf;
    char* makefile_path = NULL;
    //char* common_mk_path = NULL;

    makefile_path = str_sum (dir_path, "/Makefile");
    mf = open_file (makefile_path, "w+", NULL);

    if (!mf)
    {
        free (makefile_path);
        fprintf(stderr, "Unable to create local makefile in %s\n", dir_path);
        return;
    }
    
    //common_mk_path = strdup ("../common.mk");

    fprintf (mf, "include ../common.mk\n\n");

    fprintf (mf, "all: %s\n\n", ftest_nme);

    fprintf (mf, "%s: %s.c\n", ftest_nme, ftest_nme);
    fprintf (mf, "\t$(CC) $(CFLAGS) -o %s $< $(LFLAGS) $(LIBS)\n", ftest_nme);
    fprintf (mf, "\tchmod a+x %s\n\n", ftest_nme);    
    
    fprintf (mf, "debug: %s.c\n", ftest_nme);
    fprintf (mf, "\t$(CC) $(DBG_CFLAGS) -o %s $< $(DBG_LFLAGS) $(DBG_LIBS)\n", ftest_nme);
    fprintf (mf, "\tchmod a+x %s\n\n", ftest_nme);

    fprintf (mf, "clean:\n");    
    fprintf (mf, "\trm -rf %s.o %s \n", ftest_nme, ftest_nme);    
    fprintf (mf, "\n");    

    fclose (mf);
    free (makefile_path);
    //free (common_mk_path);
}

/*
 *  Generates string that contains test for finput_tpl_path file
 *
 */
static char* 
gen_test (const char* test_tpl, const char* purpose_tpl, 
          const char* input_path, const char* group_nme, const char* test_nme)
{
    int i;
    int purposes_num = 0;

    char* test = NULL;

    char* common_tag_values[COMMON_TAGS_NUM];
    
    struct tm * ltime = NULL;
    time_t tsec;
    int year = 2007;

    char strYear[8] = "YYYY";

    int bOK = 1;

    for (i = 0; i < COMMON_TAGS_NUM; i++)
    {
        common_tag_values[i] = NULL;
    }

    bOK = parse_file(input_path, purpose_tpl, &purposes_num, test_nme, 
        &(common_tag_values[GLOBALS_POS]), 
        &(common_tag_values[STARTUP_POS]),
        &(common_tag_values[CLEANUP_POS]),
        &(common_tag_values[TEST_PURPOSES_POS]),
        &(common_tag_values[PCF_FUNCS_POS]));

    if ((!bOK) || (test_nme == NULL)) 
    { 
        return (NULL);
    };
    test = strdup(test_tpl);

    common_tag_values[GROUP_NAME_POS]   = (char*)strdup(group_nme); 
    common_tag_values[OBJECT_NAME_POS]  = (char*)strdup(test_nme);
    common_tag_values[FTEMPLATE_POS]    = (char*)strdup(input_path);
    common_tag_values[WAIT_TIME_POS]    = (char*)strdup(cfg_parm_values[CFG_WAIT_TIME_POS]);
    
    gen_tp_arrays(purposes_num, &common_tag_values[TET_HOOKS_POS], &common_tag_values[TP_FUNCS_POS]);
        
    tsec = time(NULL);
    ltime = localtime(&tsec);
    if (ltime)
    {
        year = ltime->tm_year + 1900;
    }
    
    sprintf(strYear, "%d", year);
    
    common_tag_values[YEAR_POS] = strdup(strYear);
    common_tag_values[LIBRARY_POS] = strdup(hdr_param_value[PARAM_LIB_POS]);
    common_tag_values[LIBSECTION_POS] = strdup(hdr_param_value[PARAM_SECTION_POS]);
    
    replace_char(common_tag_values[LIBRARY_POS], '\n', ' ');
    replace_char(common_tag_values[LIBRARY_POS], '\r', ' ');
    replace_char(common_tag_values[LIBSECTION_POS], '\n', ' ');
    replace_char(common_tag_values[LIBSECTION_POS], '\r', ' ');
    
    /* do all sustitutions and deallocate memory allocated for tag_values [] */
    for (i = 0; i < COMMON_TAGS_NUM; i++)
    {
        test = replace_all_substr_in_string(test, common_tags[i], 
                                            common_tag_values[i]);
        free(common_tag_values[i]);
    }
    
    test = replace_all_substr_in_string(test, TAG_SUITE_SUBDIR_NAME, test_dir);
    
    return test;
}

static int 
parse_header(const char* input_path)
{
    int i;
    int known = 0;
    
    int size = 0;
    char* str = NULL;
    char* str_t = NULL;
    char* beg_ln = NULL;
    
    FILE* ft2c_file = NULL;
    
    if (!is_file_exists(input_path))
    {
        return 0;
    }
    
    ft2c_file = open_file (input_path, "r", NULL);
    size = fsize(ft2c_file);
    str = alloc_mem_for_string (str, size + 1);
    
    for (i = 0; i < HEADER_PARAMS_NUM; ++i)
    {
        hdr_param_value[i] = strdup("");
    }
    
    do
    {
        fgets (str, size, ft2c_file);
       
        for (i = 0; i < HEADER_PARAMS_NUM; ++i)
        {
            beg_ln = strstr (str, hdr_param_name[i]);
            if (beg_ln)
            {
                beg_ln = beg_ln + strlen(hdr_param_name[i]);
                str_t = trim_with_nl (beg_ln);
                hdr_param_value[i] = strdup (str_t);

                break;
            }
        } /*end for*/
        
        if (feof(ft2c_file) || ferror(ft2c_file))
        {
            break;
        }
    }
    while (str[0] == '#');

    free (str);
    fclose (ft2c_file);
    return 1;
}

/*
 * Reads strings from stream and stores them in str,
 * while not comment string is reached, and increases str_counter.
 */
static char*
fgets_and_skip_comments (char* str, int num, FILE* stream, int* str_counter)
{
    char* macros_str[] = { 
        "#define", "#undef",
        "#include",
        "#if", "#elif", "#else", "#endif",
        "#ifdef", "#ifndef",
        "#error",
        "#import",
        "#pragma",
        "#line",
        NULL
    };

    int i;
    int macro = 0;
    /*char* tstr = NULL;*/

    if ((feof(stream)) || (ferror(stream)))
    {
        return NULL;
    }

    do
    {
        fgets (str, num, stream);
        (*str_counter)++;
        i = 0;
        while (macros_str[i] != NULL)
        {
            int len = strlen (macros_str[i]);
            if ((!strncmp (str, macros_str[i], len)) && 
                ((str[len] == ' ') || (str[len] == '\t') || (str[len] == '\n') || (str[len] == '\r')))
            {
                macro = 1;
                break;
            }
            i++;
        }
    }
    while ((str[0] == '#') && !macro && (!feof(stream)) && (!ferror(stream)));

    /*free (tstr);*/

    if ((feof(stream)) || (ferror(stream)))
    {
        return NULL;
    } else {
        return (str);
    }
}

static int
parse_file(const char* input_path, const char* purpose_tpl, int* purposes_number, const char* test_nme,
           char** pstrGlobals, char** pstrStartup,
           char** pstrCleanup, char** pstrPurposes,
           char** pcf_funcs)
{
    FILE* fl = NULL;
    int ln_count = 0;
    int size = 0;
    char* str = NULL;
    char* str_t = NULL;
    char* text = NULL;
    
    int nBlocks = 0;
    int isBad = 0;

    printf ("Parsing file %s\n", input_path);
    *purposes_number = 0;

    if (!is_file_exists(input_path))
    {
        return 0;
    }

    fl = open_file (input_path, "r", NULL);
    size = fsize (fl);
    
    if (size <= 2)
    {
        return 0;
    }

    str = alloc_mem_for_string (str, size + 1);
    
    *pstrGlobals = strdup("");
    *pstrStartup = strdup("");
    *pstrCleanup = strdup("");
    *pstrPurposes = strdup("");
    *pcf_funcs = strdup("");
    
    char* attribs = NULL; 
    char* bl_attr_name[] = {"parentControlFunction", NULL};
    char* bl_attr_val[]  = {NULL, NULL};
    
    char* pcf_name = NULL;
    
    do
    {
        if (fgets_and_skip_comments (str, size, fl, &ln_count) == NULL)
        {
            break; /* EOF has been reached */
        }
        
        int tag_num = t2c_parse_open_tag(str, top_tag, &attribs);
        switch (tag_num)
        {
        case -1:    // not a tag
            break;
        case ITOP_GLOBAL:
            /* found GLOBAL */
            text = parse_global(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Invalid global block in %s\n", input_path);
                break;
            }

            *pstrGlobals = text;
            
            break;
        case ITOP_STARTUP:
            /* found STARTUP */
            text = parse_startup(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Invalid startup block in %s\n", input_path);
                break;
            }

            *pstrStartup = text;
            
            break;
        case ITOP_CLEANUP:
            /* found CLEANUP */
            text = parse_cleanup(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Invalid cleanup block in %s\n", input_path);
                break;
            }

            *pstrCleanup = text;
            
            break;
        case ITOP_BLOCK:
            // Parse attributes, extract PCF name (if present).
            if (!t2c_parse_attributes(attribs, bl_attr_name, bl_attr_val))
            {
                isBad = 1;
                fprintf (stderr, "Invalid attribute specification for <BLOCK> in %s\n", input_path);
                break;
            }
            
            if (bl_attr_val[0])
            {
                pcf_name = bl_attr_val[0];
                bl_attr_val[0] = NULL;
            }
            else
            {
                pcf_name = strdup("NULL");
            }
            
            text = parse_block(fl, size, purpose_tpl, purposes_number, pcf_funcs, pcf_name);
            free(pcf_name);
            
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Invalid <BLOCK> block in %s\n", input_path);
                break;
            }
 
            *pstrPurposes = str_append(*pstrPurposes, text);
 
            ++nBlocks;
            break;
        default:
            fprintf(stderr, "parseFile(): t2c_parse_open_tag() returned invalid index: %d.\n");
            isBad = 1;
            break;
        }
        
        if (isBad) 
        {
            break;
        }
    }
    while (!feof(fl) && !ferror(fl));

    free (str);
    fclose (fl);
    
    int i;
    for (i = 0; i < sizeof(bl_attr_val)/sizeof(bl_attr_val[0]) - 1; ++i)
    {
        free(bl_attr_val[i]);
    }

    free(attribs);

    return ((!isBad) && (nBlocks > 0));
}

static char*
parse_global(FILE* fl, int size)
{
    int ln_count = 0;
    char* str = NULL;
    char* text = NULL;
    int isBad = 0;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    text = strdup("");
    str = alloc_mem_for_string (str, size + 1);
    
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_GLOBAL]))
        {
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    if (isBad && text)
    {
        free(text);
    }
    return ((isBad) ? NULL : text);
}

static char*
parse_startup(FILE* fl, int size)
{
    int ln_count = 0;
    char* str = NULL;
    char* text = NULL;
    int isBad = 0;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    text = strdup("");
    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_STARTUP]))
        {
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    if (isBad && text)
    {
        free(text);
    }
    return ((isBad) ? NULL : text);
}

static char*
parse_cleanup(FILE* fl, int size)
{
    int ln_count = 0;
    char* str = NULL;
    char* text = NULL;
    int isBad = 0;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    text = strdup("");
    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_CLEANUP]))
        {
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    if (isBad && text)
    {
        free(text);
    }
    return ((isBad) ? NULL : text);
}

static char*
parse_block(FILE* fl, int size, const char* purpose_tpl, int* purposes_number,
            char** pcf_funcs, const char* pcf_name)
{
    int ln_count = 0;
    char* str = NULL;
    char* str_t = NULL;    
    char* text = NULL;
    int isBad = 0;
    
    char* targets[MAX_TARGETS_NUM];            

    int nTargets = 0;    

    int isCodeFound = 0;
    int isEndFound = 0;
    int isDefineFound = 0;
    int isTargetFound = 0;
    int isPurposeFound = 0;

    char* templ = NULL;
    char* finally_code = NULL;
    char* tag_values[TAGS_NUM];

    int i = 0;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    for (i = 0; i < TAGS_NUM; ++i)
    {
        tag_values[i] = NULL;
    }
    tag_values[PURPNUM_TAG_POS] = strdup ("<%purpose_number%>"); /* to be replaced with itself */
    tag_values[COMMENT_TAG_POS] = strdup ("// Target interfaces:\n");
    tag_values[TARGETS_TAG_POS] = strdup ("Target interface(s):\\n");
    
    text = strdup ("");
    finally_code = strdup ("");
    str = alloc_mem_for_string (str, size + 1);

    char * comment = NULL;
    char* purp = NULL;

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);
        
        if (t2c_parse_close_tag(str, top_tag[ITOP_BLOCK]))
        {
            if (!isCodeFound)
            {
                isBad = 1;
                fprintf(stderr, "No code section is specified in the block.\n");
                break;
            }

            if (!isPurposeFound) // no purpose found => assume there is one with no parameters.
            {
                if (!templ)
                {
                    isBad = 1;
                    fprintf(stderr, "Code template is NULL.\n");
                    break;
                }
                
                char* purp = strdup (templ);
                char strTmp[16] = "";
                
                ++(*purposes_number);
                                        
                isPurposeFound = 1;

                //purp = parse_purpose (fl, size, templ, finally_code, purposes_number);
                purp = replace_all_substr_in_string (purp, "<%params%>", "//    none\n");
                
                sprintf (strTmp, "%d", *purposes_number);
                purp = replace_all_substr_in_string (purp, tags[PURPNUM_TAG_POS], strTmp);
                
                purp = replace_all_substr_in_string (purp, TAG_FINALLY_NAME, finally_code);
                
                text = str_append (text, purp);
                free (purp); 
                
                // add a proper item to the array of parent control func ptrs
                *pcf_funcs = str_append(*pcf_funcs, "    ");
                *pcf_funcs = str_append(*pcf_funcs, pcf_name);
                *pcf_funcs = str_append(*pcf_funcs, ",\n"); 
            }

            /* End of the block */
            isEndFound = 1;

            break;            
        }
        
        // Parse block-level tags.
        int tag_num = t2c_parse_open_tag(str, bl_tag, NULL);
        switch (tag_num)
        {
        case -1:    // not a tag
            break;   
        case IBL_TARGETS:
            isTargetFound = 1;
            
            isBad = !parse_targets(fl, size, targets, &nTargets);
            if (isBad)
            {
                fprintf(stderr, "Invalid target section\n");
                break;
            }

            for (i = 0; i < nTargets; ++i)
            {
                tag_values[COMMENT_TAG_POS] = str_append (tag_values[COMMENT_TAG_POS], "//    ");
                tag_values[COMMENT_TAG_POS] = str_append (tag_values[COMMENT_TAG_POS], targets[i]);
                tag_values[COMMENT_TAG_POS] = str_append (tag_values[COMMENT_TAG_POS], "\n");
                
                replace_char(targets[i], '\n', ' ');
                replace_char(targets[i], '\r', ' ');
                replace_char(targets[i], '\"', ' ');
    
                tag_values[TARGETS_TAG_POS] = str_append (tag_values[TARGETS_TAG_POS], "    ");
                tag_values[TARGETS_TAG_POS] = str_append (tag_values[TARGETS_TAG_POS], targets[i]);
                tag_values[TARGETS_TAG_POS] = str_append (tag_values[TARGETS_TAG_POS], "\\n");
            }
            tag_values[COMMENT_TAG_POS] = str_append (tag_values[COMMENT_TAG_POS], "//");

            tag_values[COMMENT_TAG_POS] = str_append (tag_values[COMMENT_TAG_POS], 
                "\n// Parameters:\n<%params%>");

            break;
        case IBL_DEFINE:
            if (isDefineFound)
            {
                isBad = 1;
                fprintf(stderr, "There must be only one <DEFINE> section.\n");
                break;
            }

            isDefineFound = 1;
            
            isBad = !parse_defines(fl, size, 
                &(tag_values[DEFINE_TAG_POS]), &(tag_values[UNDEF_TAG_POS]));
            if (isBad)
            {
                fprintf(stderr, "Invalid <DEFINE> section\n");
                break;
            }

            break;
        case IBL_FINALLY:
            free (finally_code);
            finally_code = parse_finally (fl, size);
            
            if (!finally_code)
            {
                isBad = 1;
                fprintf(stderr, "Invalid <FINALLY> section found.\n");
                break;
            }
            
            break;
        case IBL_CODE:
            if (!isTargetFound)
            {
                isBad = 1;
                fprintf(stderr, "No target interfaces are specified.\n");
                break;
            }

            if (isCodeFound)
            {
                isBad = 1;
                fprintf(stderr, "There must be only one <CODE> section.\n");
                break;
            }
            
            isCodeFound = 1;

            if (tag_values[DEFINE_TAG_POS] == NULL)
            {
                tag_values[DEFINE_TAG_POS] = strdup ("");
            }

            if (tag_values[UNDEF_TAG_POS] == NULL)
            {
                tag_values[UNDEF_TAG_POS] = strdup ("");
            }

            tag_values[CODE_TAG_POS] = parse_code (fl, size);

            if (tag_values[CODE_TAG_POS] == NULL)
            {
                isBad = 1;
                fprintf(stderr, "The <CODE> section is invalid.\n");
                break;
            }

            /************************************************************************/
            /* Generate purpose template with comments, defines, undefs, code, and  */
            /* a 'finally' section.                                                 */
            /************************************************************************/
            templ = strdup (purpose_tpl);
            for (i = 0; i < TAGS_NUM; ++i)
            {
                templ = replace_all_substr_in_string (templ, tags[i], tag_values[i]);
            }
            break;
        case IBL_PURPOSE:
            
            if (!isCodeFound)
            {
                isBad = 1;
                fprintf (stderr, "Found <PURPOSE> section before <CODE>\n");
                break;
            }
            
            isPurposeFound = 1;
            
            // add a proper item to the array of parent control func ptrs
            *pcf_funcs = str_append(*pcf_funcs, "    ");
            *pcf_funcs = str_append(*pcf_funcs, pcf_name);
            *pcf_funcs = str_append(*pcf_funcs, ",\n");
            
            purp = parse_purpose (fl, size, templ, finally_code, purposes_number);
            if (purp == NULL)
            {
                isBad = 1;
                fprintf (stderr, "Invalid <PURPOSE> section found.\n");
                break;
            }

            text = str_append (text, purp);
            free (purp);
            
            break;
        default:
            fprintf(stderr, "parseBlock(): t2c_parse_open_tag() returned invalid index: %d.\n");
            isBad = 1;
            break;        
        }
        if (isBad) 
        {
            break;
        }
    }
    while (!feof(fl) && !ferror(fl));

    if (!isEndFound)
    {
        fprintf(stderr, "End of the block is not found\n");
        isBad = 1;
    }

    free (str);
    free (templ);
    free (finally_code);
    for (i = 0; i < TAGS_NUM; ++i)
    {
        free(tag_values[i]);
    }

    if (isBad)
    {
        free (text);
    }
    
    return ((isBad) ? NULL : text);
}

static int
parse_targets(FILE* fl, int size, char** pstrTargets, int* pnTargets)
{
    int ln_count = 0;
    char* str = NULL;
    char* str_t = NULL;
    int isBad = 0;
    int tmp = 0;

    *pnTargets = 0;

    if (!fl)
    {
        return 0;
    }

    if (feof(fl) || ferror(fl))
    {
        return 0;
    }

    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);
        str_t = trim_with_nl (str);

        if (t2c_parse_close_tag(str, bl_tag[IBL_TARGETS]))
        {
            break;
        }
        else
        {
            if (strlen(str_t) > 0)
            {
                
                if (*pnTargets >= MAX_TARGETS_NUM-1)
                {
                    fprintf(stderr, "Too many target interfaces in a single block.\n");
                    break;
                }

                pstrTargets[*pnTargets] = strdup(str_t);
                ++(*pnTargets);
            }
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    return (!isBad);
}

static int
parse_defines(FILE* fl, int size, char** pstrDefines, char** pstrUndefs)
{
    int ln_count = 0;
    char* str = NULL;
    char* str_t = NULL;
    char* text = NULL;
    int isBad = 0;

    if (!fl)
    {
        return 0;
    }

    if (feof(fl) || ferror(fl))
    {
        return 0;
    }

    text = strdup ("");
    *pstrDefines = strdup ("");
    *pstrUndefs = strdup ("");
    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);
        if (t2c_parse_close_tag(str, bl_tag[IBL_DEFINE]))
        {
            break;
        }
        else
        {
            str_t = trim (str);
            *pstrDefines = str_append (*pstrDefines, str_t);

            if (strstr (str, "#define"))
            {
                char* delims = " \t\n\r";    
                char* token = NULL;

                token = strtok (str, delims); /* to #define */
                token = strtok (NULL, delims); /* to NAME */

                if (token == NULL)
                {
                    fprintf(stderr, "Invalid #define.\n");
                    isBad = 1;
                    break;
                }
                *pstrUndefs = str_append (*pstrUndefs, "#undef ");
                *pstrUndefs = str_append (*pstrUndefs, token);
                *pstrUndefs = str_append (*pstrUndefs, "\n");
            }
        }
    }
    while (!feof(fl) && !ferror(fl));

    free (str);
    return (!isBad);
}

static char*
parse_code(FILE* fl, int size)
{
    int ln_count = 0;
    char* str = NULL;
    int isBad = 0;
    char* text = NULL;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    text = strdup ("");
    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, bl_tag[IBL_CODE]))
        {
            break;
        }
        else
        {
            text = str_append (text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    if (isBad)
    {
        free (text);
        return NULL;
    }
    return text;
}

static char*
parse_finally (FILE* fl, int size)
{
    int ln_count = 0;
    char* str = NULL;
    int isBad = 0;
    char* text = NULL;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    text = strdup ("");
    str = alloc_mem_for_string (str, size + 1);

    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, bl_tag[IBL_FINALLY]))
        {
            break;
        }
        else
        {
            text = str_append (text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    if (isBad)
    {
        free (text);
        return NULL;
    }
    return text;
}

static char*
parse_purpose(FILE* fl, int size, const char* templ, const char* finally_code, int* purposes_number)
{
    int ln_count = 0;
    char* str = NULL;
    char* str_t = NULL;
    int isBad = 0;
    char* text = NULL;

    int i = 0;

    char* params[MAX_PARAMS_NUM];    
    int nParams = 0;                

    char* toComment = NULL;

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    ++(*purposes_number);    /* another purpose */

    text = strdup("");
    toComment = strdup("");
    str = alloc_mem_for_string(str, size + 1);
    
    do
    {
        fgets_and_skip_comments(str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, bl_tag[IBL_PURPOSE]))
        {
            char strTmp[16] = "";

            /* fill in the gaps in the template */
            text = strdup(templ);
            if (nParams == 0)
            {
                toComment = str_append(toComment, "//    none\n");
            }
            
            text = replace_all_substr_in_string(text, "<%params%>", toComment);
            
            sprintf(strTmp, "%d", *purposes_number);
            text = replace_all_substr_in_string(text, tags[PURPNUM_TAG_POS], strTmp);
            
            text = replace_all_substr_in_string(text, TAG_FINALLY_NAME, finally_code);
            
            /* param values */

            for (i = 0; i < nParams; ++i)
            {
                sprintf (strTmp, "<%%%d%%>", i);
                text = replace_all_substr_in_string(text, strTmp, params[i]);
            }

            break;
        }
        else
        {
            str_t = trim_with_nl (str);
            if (strlen(str_t) > 0)
            {
                if (nParams >= MAX_PARAMS_NUM)
                {
                    isBad = 1;
                    fprintf (stderr, "Too many parameters are specified for the test purpose.\n");
                    break;
                }

                /* found another parameter */
                params[nParams] = strdup(str_t);
                ++nParams;

                toComment = str_append(toComment, "//    ");
                toComment = str_append(toComment, str_t);
                toComment = str_append(toComment, "\n");
            }
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    free(toComment);
    if (isBad)
    {
        free(text);
        return NULL;
    }
    return text;
}

static void
gen_tp_arrays(int number, char** pTetHooks, char** pTpFuncs)
{
    int i;
    char iStr[8]; 
    
    char* tet_hook_tpl = "    {tp_launcher, <%purpose_number%>},\n"; 
    char* t2c_func_tpl = "    test_purpose_<%purpose_number%>,\n"; 
    
    char* tstr = NULL;
    
    *pTetHooks = strdup("");
    *pTpFuncs = strdup("");
    
    for(i = 1; i <= number; ++i)
    {
        sprintf(iStr, "%d", i);
        
        tstr = strdup(tet_hook_tpl);
        tstr = replace_all_substr_in_string(tstr, "<%purpose_number%>", iStr);
        *pTetHooks = str_append(*pTetHooks, tstr);
        free(tstr);
        
        tstr = strdup(t2c_func_tpl);
        tstr = replace_all_substr_in_string(tstr, "<%purpose_number%>", iStr);
        *pTpFuncs = str_append(*pTpFuncs, tstr);
        free(tstr);
    }
    return;
}

static void 
gen_common_makefile (const char* suite_root_path, const char* output_dir_path)
{
    char* common_mk_path = NULL;
    char* common_dbg_mk_path = NULL;
    char* main_mk_path = NULL;
    
    char* path1 = NULL;
    char* path2 = NULL;

    char* inc   = "$(T2C_ROOT)/t2c/include";
    char* lib   = "$(T2C_ROOT)/t2c/lib";
    char* inc_d = "$(T2C_ROOT)/t2c/debug/include";
    char* lib_d = "$(T2C_ROOT)/t2c/debug/lib";
    
    char* tmp_incl = "/include";
    
    char* suite_inc = NULL;

    FILE* mf;   // common.mk

    common_mk_path = str_sum (output_dir_path, "common.mk");
        
    mf = open_file (common_mk_path, "w", NULL);

    if (!mf)
    {
        free (common_mk_path);
        fprintf(stderr, "Unable to create common makefile in %s\n", output_dir_path);
        return;
    }

    fprintf (mf, "CC = %s\n\n", cfg_parm_values[CFG_COMPILER_POS]);

    fprintf (mf, "TET_INC_DIR = $(TET_ROOT)/inc/tet3\n");
    fprintf (mf, "TET_LIB_DIR = $(TET_ROOT)/lib/tet3\n\n");
    
    fprintf(mf, "DBG_INC_DIR = %s\n", inc_d);
    fprintf(mf, "DBG_LIB_DIR = %s\n\n", lib_d);
    
    path1 = strdup("$(T2C_SUITE_ROOT)");
    
    path2 = concat_paths(path1, (char*)test_dir);
    suite_inc = concat_paths(path2, tmp_incl);

    fprintf(mf, "T2C_INC_DIR = %s\n", inc);
    fprintf(mf, "T2C_LIB_DIR = %s\n", lib);
    fprintf(mf, "SUITE_INC_DIR = %s\n\n", suite_inc);
    
    fprintf(mf, "CC_SPECIAL_FLAGS := $(shell (cat $(T2C_SUITE_ROOT)/cc_special_flags) 2>/dev/null)\n");
    fprintf(mf, "ADD_CFLAGS = %s $(CC_SPECIAL_FLAGS)\n", cfg_parm_values[CFG_COMP_FLAGS_POS]); 
    fprintf(mf, "ADD_LFLAGS = %s\n\n", cfg_parm_values[CFG_LINK_FLAGS_POS]); 

    fprintf(mf, "COMMON_CFLAGS = -D\"_POSIX_C_SOURCE=200112L\" -std=c99 -I$(T2C_INC_DIR) -I$(SUITE_INC_DIR) $(ADD_CFLAGS)\n");
        
    fprintf(mf, "CFLAGS = -O2 $(COMMON_CFLAGS) -I$(TET_INC_DIR)\n");
    fprintf(mf, "DBG_CFLAGS = -g -DT2C_DEBUG $(COMMON_CFLAGS) -I$(DBG_INC_DIR)\n\n");
    
    fprintf(mf, "LFLAGS = $(ADD_LFLAGS)\n");
    fprintf(mf, "DBG_LFLAGS = $(ADD_LFLAGS)\n\n");
    
    fprintf(mf, "LIBS = $(TET_LIB_DIR)/tcm.o $(TET_LIB_DIR)/libapi.a $(T2C_LIB_DIR)/t2c_util.a\n");
    fprintf(mf, "DBG_LIBS = $(DBG_LIB_DIR)/dbgm.o $(DBG_LIB_DIR)/t2c_util_d.a\n\n");
    
    free(common_mk_path);
    fclose(mf);
    
    /* Generate main makefile (it will invoke makefiles in each subdir). */
    main_mk_path = str_sum (output_dir_path, "Makefile");

    mf = open_file (main_mk_path, "w", NULL);

    if (!mf)
    {
        free (main_mk_path);
        free (path1);     
        free (path2);
        free (suite_inc);   
        fprintf(stderr, "Unable to create main makefile in %s\n", output_dir_path);
        return;
    }

    fprintf (mf, "EXCLUDE = Makefile common.mk \n\n");

    fprintf (mf, "SUBDIRS := $(filter-out $(EXCLUDE), $(wildcard *))\n");
    fprintf (mf, "BUILDDIRS = $(SUBDIRS)\n");
    fprintf (mf, "CLEANDIRS = $(addprefix _clean_,$(SUBDIRS))\n\n");

    fprintf (mf, "all:    $(BUILDDIRS)\n\n");

    fprintf (mf, "$(BUILDDIRS):\n");
    fprintf (mf, "\t$(MAKE) -C $@\n\n");

    fprintf (mf, "clean: $(CLEANDIRS)\n\n");

    fprintf (mf, "$(CLEANDIRS):\n");
    fprintf (mf, "\t$(MAKE) -C $(subst _clean_,,$@) clean\n\n");

    fprintf (mf, ".PHONY:    all $(BUILDDIRS) $(CLEANDIRS) clean\n\n");

    free (main_mk_path);

    fclose (mf);
    free (path1);
    free (path2);
    free (suite_inc);   
    return;
}

static void
set_defaults ()
{
    int i;
    for (i = 0; i < CFG_PARAMS; ++i)
    {
        free (cfg_parm_values[i]);
    }
    
    cfg_parm_values[CFG_COMPILER_POS] = (char *)strdup ("gcc");
    cfg_parm_values[CFG_COMP_FLAGS_POS] = (char *)strdup ("");
    cfg_parm_values[CFG_LINK_FLAGS_POS] = (char *)strdup ("");
    cfg_parm_values[CFG_TET_RECORD_POS] = (char *)strdup ("yes");
    cfg_parm_values[CFG_WAIT_TIME_POS] = (char *)strdup ("30");
}

static void
load_config(const char* fpath)
{
    char* cfg_path = NULL;
    char* cfg_path1 = NULL;
    FILE* fd;
    
    cfg_path1 = concat_paths(t2c_suite_root, (char*) fpath);
    cfg_path = (char*)shorten_path(cfg_path1);

    fd = open_file(cfg_path, "r", NULL);
    if (fd != NULL)
    {
        int size = 0;
        char* line = NULL;

        size = fsize(fd);
        line = alloc_mem_for_string(line, size + 1);

        if ((size > 2) && (!feof(fd)) && (!ferror(fd)))
        {
            do
            {
                fgets(line, size, fd);
                if (ferror(fd))
                {
                    break;
                }
                load_parameter(line);
            }
            while (!feof(fd));
        }

        if (strcmp(cfg_parm_values[CFG_TET_RECORD_POS], "yes") &&
            strcmp(cfg_parm_values[CFG_TET_RECORD_POS], "YES")) /* value is not "yes" */
        {
            bAddTetScenRecord = 0;
        }
        
        int waittime = atoi(cfg_parm_values[CFG_WAIT_TIME_POS]);
        if (waittime == 0)
        {
            sprintf(cfg_parm_values[CFG_WAIT_TIME_POS], "0");
        }
        
        fclose (fd);
        free (line);
    }
    else
    {
        fprintf (stderr, "Unable to open configuration file: \n%s\n", cfg_path);
    }

    free (cfg_path);
    free (cfg_path1);
}

static void
load_parameter(const char* line)
{
    char *str = NULL;
    char *str_name = NULL;
    char *str_value = NULL;
    char *ceq = NULL;

    if ((line == NULL) || (line[0] == '\0') || line[0] == '=')
    {
        return;
    }

    str = (char*)strdup(line);
    ceq = strchr (str, '=');
    
    if (ceq)
    {
        int i = 0;

        *ceq = '\0';
        str_name = trim(str);
                
        str_value = (char*)(ceq + 1);
        str_value = trim_with_nl(str_value);

        for (i = 0; i < CFG_PARAMS; ++i)
        {
            if (!strcmp(str_name, cfg_parm_names[i]))
            {
                cfg_parm_values[i] = (char*)strdup(str_value);
                break;
            }
        }
    }

    free(str);
}

/*******/

