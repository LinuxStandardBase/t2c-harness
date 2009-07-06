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
#include "../include/param.h"

/*******************************************************************************/
#define NCMD_PARAMS 3       // Number of mandatory command line parameters to be specified
#define ARGV_TSDIR_INDEX 2  // argv[ARGV_TSDIR_INDEX] is a path test suite dir to be processed.
#define ARGV_PARAM_INDEX 3  // argv[ARGV_PARAM_INDEX] is a path to the config file.

#define PATH_LEN 1024       // Maximum path length allowed

#define INPUT_EXT          ".t2c"       // input file extention
#define MAKEFILE_TPL_EXT   ".tmk"       // extension of template makefiles

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
#define CFG_COMPILER_POS    0
#define CFG_COMP_FLAGS_POS  1
#define CFG_LINK_FLAGS_POS  2
#define CFG_TET_RECORD_POS  3
#define CFG_WAIT_TIME_POS   4
#define CFG_LANGUAGE_POS    5
#define CFG_SINGLE_POS      6
#define CFG_MK_TPL_POS      7

// Number of parameters that can be specified in a .cfg-file.
#define CFG_PARAMS (sizeof(cfg_parm_names)/sizeof(cfg_parm_names[0]))        

// Names of the parameters from .cfg-file
static char* cfg_parm_names[] = {
    "COMPILER",
    "COMPILER_FLAGS",
    "LINKER_FLAGS",
    "TET_SCEN_RECORD",
    
    "WAIT_TIME",        // Maximum time for a test to run (in seconds). Default: "30"
    
    "LANGUAGE",         // "CPP" for C++-code, any other value for plain C. Default: "C"
    
    "SINGLE_PROCESS",   // If "YES" or "yes", all tests are executed in the same process, 
                        // otherwise - in separate process each. Ignored in standalone (debug) mode.
                        // Default: "no".
    
    "MAKEFILE_TEMPLATE" // Template makefile for the tests in the subsuite. Default: ""
};

/* Values of the parameters from .cfg-file. Use set_defaults() to reset these to their
 * default values.
 */
static char* cfg_parm_values[CFG_PARAMS] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

// 1 corresponds to TET_SCEN_RECORD=yes in the config. file, 0 is for the 
// opposite case.
int bAddTetScenRecord = 1;

// 1 if C++ code is to be generated, 0 otherwise (i.e. plain C)
// See "LANGUAGE" option in the config file.
int bGenCpp = 0;

// 1 if all the tests should be executed in a single process, 0 otherwise.
// See "SINGLE_PROCESS" option in the config file.
int bSingleProcess = 0;

/************************************************************************/
#define COMMON_TEST_TPL      "test.tpl"    /* common test case template*/
#define COMMON_PURPOSE_TPL   "purpose.tpl" /* common test purpose template*/
#define DEFAULT_MAKEFILE_TPL "default.tmk" /* default makefile template */
#define COMMON_MAKEFILE_TPL  "common.tmk"  /* template of a common makefile */
#define COMMON_TPL_DIR       "t2c/src/templates/"

#define COMMON_TAGS_NUM     (sizeof(common_tags)/sizeof(common_tags[0]))
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
#define RCAT_NAMES_POS      14

static char* common_tags[] = { 
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
    "<%wait_time%>",
    "<%rcat_names%>"
};
    
#define MAX_PARAMS_NUM  256
#define MAX_TARGETS_NUM 256
    
#define TAGS_NUM 8
#define COMMENT_TAG_POS  0
#define DEFINE_TAG_POS   1
#define CODE_TAG_POS     2
#define PURPNUM_TAG_POS  3
#define UNDEF_TAG_POS    4
#define TARGETS_TAG_POS  5
#define	LSB_MIN_VER_POS	 6
#define	LSB_MAX_VER_POS  7
    
static char* tags[] = { 
    "<%comment%>",
    "<%define%>",
    "<%code%>",
    "<%purpose_number%>",
    "<%undef%>",
    "<%targets%>",
    "<%lsb_min_ver%>",
    "<%lsb_max_ver%>"
};

#define TAG_FINALLY_NAME "<%finally%>"
#define TAG_SUITE_SUBDIR_NAME "<%suite_subdir%>"
    
// Placeholders for makefile parameters
static char* mk_params[] = {
    "<%test_name%>",
    "<%test_dir_path%>",
    "<%test_suite_root%>",
    "<%test_tmp_basename%>"
};
// Positions of makefile parameter placeholders in mk_params[]
#define MK_PARAM_NUM    (sizeof(mk_params)/sizeof(mk_params[0]))
#define MK_NAME_POS         0
#define MK_DIR_PATH_POS     1
#define MK_SUITE_ROOT_POS   2
#define MK_TMP_BASENAME_POS 3

// Placeholders for parameters in common.mk
static char* common_mk_params[] = {
    "<%test_dir%>",
    "<%compiler%>",
    "<%add_cflags%>",
    "<%add_lflags%>",
    "<%test_std_cflags%>",
    "<%test_file_ext%>",
    "<%single_process_flag%>"
};
// Positions of makefile parameter placeholders in common_mk_params[]
#define CMK_PARAM_NUM    (sizeof(common_mk_params)/sizeof(common_mk_params[0]))
#define CMK_DIR_PATH_POS        0
#define CMK_COMPILER_POS        1
#define CMK_ADD_CFLAGS_POS      2
#define CMK_ADD_LFLAGS_POS      3
#define CMK_TEST_STD_FLAGS_POS  4
#define CMK_TEST_FILE_EXT_POS   5
#define CMK_SP_FLAG_POS         6

// func_scen
char* func_tests_scenario_file_name = "func_scen";

// Number of params to be extracted from the header of a .t2c-file
#define HEADER_PARAMS_NUM    (sizeof(hdr_param_name)/sizeof(hdr_param_name[0]))     
#define PARAM_LIB_POS        0
#define PARAM_SECTION_POS    1
#define PARAM_RCAT_POS       2
#define PARAM_T2C_BASENAME_POS   3

char* hdr_param_value[] = { 
    NULL, 
    NULL,
    NULL,
    NULL
};
    
char* hdr_param_name[] = { 
    "library", 
    "libsection",
    "additional_req_catalogues",
    "t2c_basename"
};

// The values of $T2C_ROOT and $T2C_SUITE_ROOT.
char* t2c_root = NULL;
char* t2c_suite_root = NULL;

// A path (relative to $T2C_SUITE_ROOT) to the test suite subdirectory 
// (e.g. "desktop-t2c/glib-t2c")
char* test_dir = NULL;

// Default makefile template
char* def_mk_tpl = NULL;

// Makefile template for a subsuite
char* subsuite_mk_tpl = NULL;

// Number of current line in a T2C file
int ln_count = 0;

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
             const char* ftest_nme,  const char* makefile_tpl);

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
            char** pcf_funcs, const char* pcf_name, const char *lsb_min_ver, const char *lsb_max_ver);

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
            printf("Language is %s.\n", (bGenCpp ? "CPP" : "C"));
        }
    }
    
    test_dir = strdup(argv[ARGV_TSDIR_INDEX]);
    
    char* src_dir  = concat_paths(test_dir, src_dir_end);
    char* out_dir  = concat_paths(test_dir, out_dir_end);
    char* scen_dir = concat_paths(test_dir, scen_dir_end);
    
    if ((cfg_parm_values[CFG_MK_TPL_POS] != NULL) && 
        (strlen(cfg_parm_values[CFG_MK_TPL_POS]) != 0))
    {
        char* tmp_path = concat_paths(test_dir, cfg_parm_values[CFG_MK_TPL_POS]);
        free(cfg_parm_values[CFG_MK_TPL_POS]);
        cfg_parm_values[CFG_MK_TPL_POS] = tmp_path;

        printf("Makefile template for this test suite: $T2C_SUITE_ROOT/%s.\n", 
            cfg_parm_values[CFG_MK_TPL_POS]);
    }
    
    run_generator(argv[1], src_dir, out_dir, scen_dir);
    
    free(src_dir);
    free(out_dir);
    free(scen_dir);
    free(def_mk_tpl);
    free(subsuite_mk_tpl);

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
     
    suite_root1 = concat_paths(t2c_suite_root, (char*)suite_root_in);
    suite_root = shorten_path(suite_root1);

    path1 = concat_paths(suite_root, (char*)scen_dir_in);
 
    scen_dir = (char*)shorten_path(path1);

    if (!is_directory_exists((char*)scen_dir))
    {
        create_dir((char*)scen_dir);
    }

    scenario_file = alloc_mem_for_string(scenario_file, 
                                         strlen (scen_dir));
    scenario_file = str_append(scenario_file, scen_dir);
    scenario_file = str_append(scenario_file, "/");
    scenario_file = str_append(scenario_file, func_tests_scenario_file_name);

    /*
    Clear the func_scen file.
    */
    ffunc_scen = fopen(scenario_file, "w");
    if (!ffunc_scen)
    {
        fprintf (stderr, "Unable to open local scenario file: \n %s\n", scenario_file);
    }
    fclose(ffunc_scen);

    output_dir1 = concat_paths(suite_root, (char*) output_dir_in);
    output_dir = shorten_path(output_dir1);
    
    input_path1 = alloc_mem_for_string(input_path1, strlen (input_dir));
    input_path1 = str_append(input_path1, input_dir);
    
    input_path2 = concat_paths(suite_root, input_path1);
    input_path = shorten_path(input_path2);

    if (input_path[strlen (input_path) - 1] != '/')
    {
        input_path = str_append (input_path, "/");
    }

    if (bAddTetScenRecord)
    {
        ftet_scen_path1 = alloc_mem_for_string(ftet_scen_path1, 
            strlen(suite_root));
        ftet_scen_path1 = str_append(ftet_scen_path1, suite_root);

        ftet_scen_path = concat_paths(ftet_scen_path1, "/tet_scen");

        ftet_scen_exists = is_file_exists(ftet_scen_path);
        ftet_scen = open_file(ftet_scen_path, "a+", NULL);
        if (!ftet_scen_exists)
        {
            fprintf(ftet_scen, "all\n");
        }

        ftet_scen_path2 = alloc_mem_for_string(ftet_scen_path2, 
            strlen(scen_dir_in) + 1);

        if (scen_dir_in[0] != '/')
        {
            ftet_scen_path2 = str_append(ftet_scen_path2, "/");
        }
        ftet_scen_path2 = str_append(ftet_scen_path2, scen_dir_in);


        func_scen_path = concat_paths(ftet_scen_path2, "/func_scen");
        fprintf(ftet_scen, "\t:include:%s\n", func_scen_path);
        fclose(ftet_scen);
        
        free(func_scen_path);
        func_scen_path = NULL;
    }
    
    // load the default makefile template
    FILE *ftmk = NULL;
    char* tmk_path  = NULL;
    char* tpl_path1 = NULL;
    char* tpl_path2 = NULL;

    tpl_path1 = alloc_mem_for_string(tpl_path1, strlen(t2c_root)); 
    tpl_path1 = str_append(tpl_path1, t2c_root);
    tpl_path2 = concat_paths(tpl_path1, 
                             COMMON_TPL_DIR);
    tmk_path = concat_paths(tpl_path2, DEFAULT_MAKEFILE_TPL);
    
    ftmk = open_file(tmk_path, "r", NULL);
    if (ftmk != NULL)
    {
        def_mk_tpl = read_file_to_string(ftmk);
        fclose(ftmk);
    }
    
    free(tpl_path1);
    free(tpl_path2);
    free(tmk_path);
    
    if (def_mk_tpl == NULL)
    {
        fprintf(stderr, "Unable to read default makefile template.\n");
        def_mk_tpl = strdup("");
    }
    
    // load custom makefile template for this test suite (if specified)
    if ((cfg_parm_values[CFG_MK_TPL_POS] != NULL) && 
        (strlen(cfg_parm_values[CFG_MK_TPL_POS]) != 0))
    {
        FILE* fsmk = NULL;
        char* tmp = NULL;
        char* smk_path = NULL;
        
        tmp = concat_paths(suite_root, cfg_parm_values[CFG_MK_TPL_POS]);
        smk_path = (char*)shorten_path(tmp);
        free(tmp);
        
        fsmk = open_file(smk_path, "r", NULL);
        if (fsmk != NULL)
        {
            subsuite_mk_tpl = read_file_to_string(fsmk);
            fclose(fsmk);
        }
       
        free(smk_path);
    }
    
    // read directory names from input_dir directory 
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
    free (ftet_scen_path2);
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
        tpl_path1 = alloc_mem_for_string(tpl_path1, strlen(t2c_root)); 
        tpl_path1 = str_append(tpl_path1, t2c_root);
        tpl_path2 = concat_paths(tpl_path1, 
                                 COMMON_TPL_DIR);
        tpl_path = concat_paths(tpl_path2, COMMON_TEST_TPL);
        
        pFile = open_file(tpl_path, "r", NULL);

        free(tpl_path1);
        free(tpl_path2);
    }
    free(tpl_path);
    test_tpl = read_file_to_string(pFile);
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
        tpl_path1 = alloc_mem_for_string(tpl_path1, strlen(t2c_root)); 
        tpl_path1 = str_append(tpl_path1, t2c_root);
        tpl_path2 = concat_paths(tpl_path1, 
                                  COMMON_TPL_DIR);
        tpl_path = concat_paths(tpl_path2, COMMON_PURPOSE_TPL);
        pFile = open_file(tpl_path, "r", NULL);
        
        free(tpl_path1);
        free(tpl_path2);
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
        char* makefile_tpl = NULL;
        
        ftest_nme = get_substr (ep->d_name, 0, 
                                strstr (ep->d_name, INPUT_EXT) - (ep->d_name + 1));
        if ((strcmp (ep->d_name + strlen (ep->d_name) - strlen (INPUT_EXT), INPUT_EXT)) 
            || (ftest_nme == NULL)) 
        {
            ep = readdir (pDir);
            if (ftest_nme)
            {
                free (ftest_nme);
            }
            free(makefile_tpl);
            continue;
        }
        /* generate test string */
        input_path = str_sum(group_path, ep->d_name);
        
        char* tpl_mk_path = str_sum(group_path, ftest_nme);
        tpl_mk_path = str_append(tpl_mk_path, MAKEFILE_TPL_EXT);
        
        if (is_file_exists(tpl_mk_path))
        {   // template makefile exists, so we use it
            FILE* fd = open_file(tpl_mk_path, "r", NULL);
            makefile_tpl = read_file_to_string(fd);
            fclose(fd);
        }
        else if (subsuite_mk_tpl != NULL)
        {
            makefile_tpl = strdup(subsuite_mk_tpl);
        }
        else
        {   // template makefiles do not exist, so we use the default one
            makefile_tpl = strdup(def_mk_tpl);
        }
        
        free(tpl_mk_path);
        
        /* parse header of the .t2c-file here */
        bOK = parse_header(input_path);

        if (!bOK)        
        {
            fprintf (stderr, "warning: invalid header in %s\n", input_path);
            free (input_path);
            free (ftest_nme);
            free(makefile_tpl);
            continue;
        }
        
        test = gen_test (test_tpl, purpose_tpl, input_path, group_nme, ftest_nme);
                         
        if (test == NULL)
        {
            ep = readdir (pDir);
            fprintf (stderr, "Test generator is unable to generate test for %s\n", input_path);
            free (input_path);
            free (ftest_nme);
            free(makefile_tpl);
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

        gen_makefile(suite_root, output_path, ftest_nme, makefile_tpl);
        ftest_src = str_sum (ftest_nme, (bGenCpp ? ".cpp" : ".c"));
        output_path = str_append (output_path, ftest_src);
        pFile = open_file (output_path, "w", NULL);

        fputs (test, pFile);
        add_test_in_scen (suite_root, scen_file, output_dir, ftest_nme);
        
        free (test);
        free (ftest_src);
        free (ftest_nme);
        free (input_path);
        free (output_path);	
        free(makefile_tpl);   

        for (int i = 0; i < HEADER_PARAMS_NUM; ++i)
	    {
	        free(hdr_param_value[i]);
	    }
        fclose (pFile);

        ep = readdir (pDir);
    }

    (void) closedir (pDir);

    free (test_tpl);
    free (output_dir_path);
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
gen_makefile(const char* suite_root, const char* dir_path, const char* ftest_nme, const char* makefile_tpl)
{
    FILE* mf;
    char* makefile_path = NULL;

    makefile_path = str_sum (dir_path, "/Makefile");
    mf = open_file (makefile_path, "w+", NULL);

    if (!mf)
    {
        free (makefile_path);
        fprintf(stderr, "Unable to create local makefile in %s\n", dir_path);
        return;
    }
    
    char* mf_str = strdup(makefile_tpl);
    mf_str = replace_all_substr_in_string(mf_str, mk_params[MK_NAME_POS], ftest_nme);
    mf_str = replace_all_substr_in_string(mf_str, mk_params[MK_DIR_PATH_POS], dir_path);
    mf_str = replace_all_substr_in_string(mf_str, mk_params[MK_SUITE_ROOT_POS], suite_root);
    
    //-- (begin) - to be deprecated later
    char *t2c_basename = NULL;
	if(hdr_param_value[PARAM_T2C_BASENAME_POS] != NULL && hdr_param_value[PARAM_T2C_BASENAME_POS][0] != 0 )
    {
		t2c_basename = strdup(hdr_param_value[PARAM_T2C_BASENAME_POS]);
	}
    else
    {
		t2c_basename = strdup(ftest_nme);
	}
    mf_str = replace_all_substr_in_string(mf_str, mk_params[MK_TMP_BASENAME_POS], t2c_basename);
    free(t2c_basename);
    //-- (end)
    
    fputs(mf_str, mf);
    free(mf_str);
    
    fclose (mf);
    free (makefile_path);
}

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
        return NULL;
    }
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

    // Prepare the list of additional req catalogues
    common_tag_values[RCAT_NAMES_POS] = strdup("");
    {
        const char* delims = " ,;\t";
        char* token = NULL;
        
        token = strtok(hdr_param_value[PARAM_RCAT_POS], delims);
        while (token)
        {
            common_tag_values[RCAT_NAMES_POS] = str_append(common_tag_values[RCAT_NAMES_POS], "    \"");
            common_tag_values[RCAT_NAMES_POS] = str_append(common_tag_values[RCAT_NAMES_POS], token);
            common_tag_values[RCAT_NAMES_POS] = str_append(common_tag_values[RCAT_NAMES_POS], "\",\n");
            token = strtok(NULL, delims);
        }
    }   
    
    /* do all substitutions and deallocate memory */
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
    
    char* tstr = NULL;  // temporary
    char* str_t = NULL; // temporary
    
    char* beg_ln = NULL;
    
    FILE* ft2c_file = NULL;
    
    if (!is_file_exists(input_path))
    {
        return 0;
    }
    
    ft2c_file = open_file(input_path, "r", NULL);
    size = fsize(ft2c_file);
    str = alloc_mem_for_string(str, size + 1);
    
    for (i = 0; i < HEADER_PARAMS_NUM; ++i)
    {
        hdr_param_value[i] = strdup("");
    }
    
    do
    {
        fgets(str, size, ft2c_file);
        ++ln_count;
        tstr = trim_with_nl(str);
       
        for (i = 0; i < HEADER_PARAMS_NUM; ++i)
        {   
            if (tstr[0] == '#')
            {
                beg_ln = strstr(tstr + 1, hdr_param_name[i]);
                if (beg_ln == tstr + 1)
                {
                    beg_ln = beg_ln + strlen(hdr_param_name[i]);
                    if ((beg_ln[0] == ' ') || (beg_ln[0] == '\t'))
                    {
                        str_t = trim_with_nl(beg_ln);
                        
                        free(hdr_param_value[i]);
                        hdr_param_value[i] = strdup(str_t);
                    }
                    break;
                }
            }
        } /*end for*/
        
        if (feof(ft2c_file) || ferror(ft2c_file))
        {
            break;
        }
    }
    while ((tstr[0] == 0) || (tstr[0] == '#'));
    
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
    int size = 0;
    char* str = NULL;
    char* str_t = NULL;
    char* text = NULL;
    char* lsb_min_ver = NULL;
    char* lsb_max_ver = NULL;
    
    int nBlocks = 0;
    int isBad = 0;

    // reset line count
    ln_count = 0;
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
    char* bl_attr_name[] = {"parentControlFunction", "lsbMinVersion", "lsbMaxVersion", NULL};
    char* bl_attr_val[]  = {NULL, NULL, NULL, NULL};
    
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
            free(text);
            text = parse_global(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Line %d: Invalid GLOBAL section.\n", ln_count);
                break;
            }
            
            free(*pstrGlobals);
            *pstrGlobals = strdup(text);
           
            break;
        case ITOP_STARTUP:
            /* found STARTUP */
            free(text);
            text = parse_startup(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Line %d: Invalid STARTUP section.\n", ln_count);
                break;
            }
            
            free(*pstrStartup);
            *pstrStartup = strdup(text);
            
            break;
        case ITOP_CLEANUP:
            /* found CLEANUP */
            free(text);
            text = parse_cleanup(fl, size);
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, "Line %d: Invalid CLEANUP section.\n", ln_count);
                break;
            }
            
            free(*pstrCleanup);
            *pstrCleanup = strdup(text);
            
            break;
        case ITOP_BLOCK:
            // Parse attributes, extract PCF name (if present).
            if (!t2c_parse_attributes(attribs, bl_attr_name, bl_attr_val))
            {
                isBad = 1;
                fprintf (stderr, "Line %d: Invalid attribute specification for <BLOCK> section.\n", ln_count);
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
            
            if (bl_attr_val[1])
            {
                lsb_min_ver = bl_attr_val[1];
                bl_attr_val[1] = NULL;
            }
            else
            {
                lsb_min_ver = strdup("NULL");
            }
            
            if (bl_attr_val[2])
            {
                lsb_max_ver = bl_attr_val[2];
                bl_attr_val[2] = NULL;
            }
            else
            {
                lsb_max_ver = strdup("NULL");
            }
            
            free(text);
            
            int ln_beg = ln_count;
            text = parse_block(fl, size, purpose_tpl, purposes_number, pcf_funcs, pcf_name, lsb_min_ver, lsb_max_ver);
    
    		free (lsb_max_ver);
    		free (lsb_min_ver);
            free(pcf_name);
            
            if (text == NULL) 
            {
                isBad = 1;
                fprintf (stderr, 
                    "Line %d: The BLOCK section that begins at line %d is invalid.\n", 
                    ln_count, ln_beg);
                break;
            }
 
            *pstrPurposes = str_append(*pstrPurposes, text);
 
            ++nBlocks;
            break;
        default:
            fprintf(stderr, "Line %d: parseFile(): t2c_parse_open_tag() returned invalid index: %d.\n", 
                ln_count, tag_num);
            isBad = 1;
            break;
        }
        
        if (isBad) 
        {
            break;
        }
    }
    while (!feof(fl) && !ferror(fl));

    free(str);
    fclose(fl);
    free(text);
    
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
    
    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_GLOBAL]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));
    
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for GLOBAL section.\n", ln_count);
        isBad = 1;
    }
    
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

    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_STARTUP]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));
	
	if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for STARTUP section.\n", ln_count);
        isBad = 1;
    }    

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

    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, top_tag[ITOP_CLEANUP]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            text = str_append(text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));
    
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for CLEANUP section.\n", ln_count);
        isBad = 1;
    }

    free(str);
    if (isBad && text)
    {
        free(text);
    }

    return ((isBad) ? NULL : text);
}

static char*
parse_block(FILE* fl, int size, const char* purpose_tpl, int* purposes_number,
            char** pcf_funcs, const char* pcf_name, const char *lsb_min_ver, const char *lsb_max_ver)
{

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
    int ln_beg = ln_count;
        
    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }
    
    for (i = 0; i < MAX_TARGETS_NUM; ++i)
    {
        targets[i] = NULL;
    }
    
    for (i = 0; i < TAGS_NUM; ++i)
    {
        tag_values[i] = NULL;
    }
    tag_values[PURPNUM_TAG_POS] = strdup("<%purpose_number%>"); /* to be replaced with itself */
    tag_values[COMMENT_TAG_POS] = strdup("// Target interfaces:\n");
    tag_values[TARGETS_TAG_POS] = strdup("Target interface(s):\\n");
    
    if (strcmp (lsb_min_ver, STRING_NULL) == 0)
    {
    	tag_values[LSB_MIN_VER_POS] = strdup (lsb_min_ver);
		tag_values[LSB_MIN_VER_POS] = str_append (tag_values[LSB_MIN_VER_POS], STRING_SEMICOLON);
    }
	else
	{
    	tag_values[LSB_MIN_VER_POS] = strdup (STRING_APOSTR);
		tag_values[LSB_MIN_VER_POS] = str_append (tag_values[LSB_MIN_VER_POS], lsb_min_ver);
		tag_values[LSB_MIN_VER_POS] = str_append (tag_values[LSB_MIN_VER_POS], STRING_APOSTR);
		tag_values[LSB_MIN_VER_POS] = str_append (tag_values[LSB_MIN_VER_POS], STRING_SEMICOLON);
	}
    
    if (strcmp (lsb_max_ver, STRING_NULL) == 0)
    {
    	tag_values[LSB_MAX_VER_POS] = strdup (lsb_max_ver);
		tag_values[LSB_MAX_VER_POS] = str_append (tag_values[LSB_MAX_VER_POS], STRING_SEMICOLON);
    }
	else
	{
    	tag_values[LSB_MAX_VER_POS] = strdup (STRING_APOSTR);
		tag_values[LSB_MAX_VER_POS] = str_append (tag_values[LSB_MAX_VER_POS], lsb_max_ver);
		tag_values[LSB_MAX_VER_POS] = str_append (tag_values[LSB_MAX_VER_POS], STRING_APOSTR);
		tag_values[LSB_MAX_VER_POS] = str_append (tag_values[LSB_MAX_VER_POS], STRING_SEMICOLON);
	}
    
    text = strdup("");
    finally_code = strdup("");
    str = alloc_mem_for_string(str, size + 1);

    char * comment = NULL;
    char* purp = NULL;

    do
    {
        int old_purp_num = 0;
        fgets_and_skip_comments(str, size, fl, &ln_count);
        
        if (t2c_parse_close_tag(str, top_tag[ITOP_BLOCK]))
        {
        	
            if (!isCodeFound)
            {
                isBad = 1;
                fprintf(stderr, 
                    "Line %d: No CODE section is specified in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }

            if (!isPurposeFound) // no purpose found => assume there is one with no parameters.
            {
                if (!templ)
                {
                    isBad = 1;
                    fprintf(stderr, 
                        "Line %d: Code template is invalid in the BLOCK beginning at line %d.\n",
                        ln_count, ln_beg);
                    break;
                }
                
                char* purp = strdup (templ);
                char strTmp[16] = "";
                
                ++(*purposes_number);
                                        
                isPurposeFound = 1;

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
                fprintf(stderr, 
                    "Line %d: Invalid TARGET section in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
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
                fprintf(stderr, 
                    "Line %d: Multiple DEFINE sections found in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }

            isDefineFound = 1;
            
            isBad = !parse_defines(fl, size, 
                &(tag_values[DEFINE_TAG_POS]), &(tag_values[UNDEF_TAG_POS]));
            if (isBad)
            {
                fprintf(stderr, 
                    "Line %d: Invalid DEFINE section in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }

            break;
        case IBL_FINALLY:
            free (finally_code);
            finally_code = parse_finally (fl, size);
            
            if (!finally_code)
            {
                isBad = 1;
                fprintf(stderr, 
                    "Line %d: Invalid FINALLY section found in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }
            
            break;
        case IBL_CODE:
            if (!isTargetFound || (nTargets == 0))
            {
                isBad = 1;
                fprintf(stderr, 
                    "Line %d: No target interfaces are specified in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }

            if (isCodeFound)
            {
                isBad = 1;
                fprintf(stderr, 
                    "Line %d: Multiple CODE sections found in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
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
                fprintf(stderr, 
                    "Line %d: The CODE section is invalid in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
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
                fprintf (stderr, 
                    "Line %d: Found PURPOSE section before CODE section in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }
            
            isPurposeFound = 1;
            
            old_purp_num = *purposes_number;
            purp = parse_purpose(fl, size, templ, finally_code, purposes_number);
            
            // add proper items to the array of parent control func ptrs
            // (the same for all newly parsed test purposes)
            for (i = old_purp_num; i < *purposes_number; ++i)
            {
                *pcf_funcs = str_append(*pcf_funcs, "    ");
                *pcf_funcs = str_append(*pcf_funcs, pcf_name);
                *pcf_funcs = str_append(*pcf_funcs, ",\n");
            }

            if (purp == NULL)
            {
                isBad = 1;
                fprintf (stderr, 
                    "Line %d: Invalid PURPOSE section found in the BLOCK beginning at line %d.\n", 
                    ln_count, ln_beg);
                break;
            }

            text = str_append(text, purp);
            free(purp);
            
            break;
        default:
            fprintf(stderr, 
                "Line %d: parseBlock(): t2c_parse_open_tag() returned invalid index: %d.\n",
                ln_count, tag_num);
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
        fprintf(stderr, "Line %d: End of the BLOCK that begins at line %d is not found.\n", 
            ln_count, ln_beg);
        isBad = 1;
    }

    free(str);
    free(templ);
    free(finally_code);
    for (i = 0; i < TAGS_NUM; ++i)
    {
        free(tag_values[i]);
    }
    
    for (i = 0; i < nTargets; ++i)
    {
        free(targets[i]);
    }
    
    if (isBad)
    {
        free(text);
    }

    return ((isBad) ? NULL : text);
}

static int
parse_targets(FILE* fl, int size, char** pstrTargets, int* pnTargets)
{
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

    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);
        str_t = trim_with_nl (str);

        if (t2c_parse_close_tag(str, bl_tag[IBL_TARGETS]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            if (strlen(str_t) > 0)
            {
                
                if (*pnTargets >= MAX_TARGETS_NUM-1)
                {
                    fprintf(stderr, "Line %d: Too many target interfaces in a single BLOCK.\n",
                        ln_count);
                    break;
                }

                pstrTargets[*pnTargets] = strdup(str_t);
                ++(*pnTargets);
            }
        }
    }
    while (!feof(fl) && !ferror(fl));
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for TARGETS section.\n", ln_count);
        isBad = 1;
    }
    
    free(str);

    return (!isBad);
}

static int
parse_defines(FILE* fl, int size, char** pstrDefines, char** pstrUndefs)
{
    char* str = NULL;
    char* str_t = NULL;
    int isBad = 0;

    if (!fl)
    {
        return 0;
    }

    if (feof(fl) || ferror(fl))
    {
        return 0;
    }

    *pstrDefines = strdup("");
    *pstrUndefs = strdup("");
    str = alloc_mem_for_string(str, size + 1);

    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments(str, size, fl, &ln_count);
        if (t2c_parse_close_tag(str, bl_tag[IBL_DEFINE]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            str_t = trim(str);
            *pstrDefines = str_append(*pstrDefines, str_t);

            if (strstr (str, "#define"))
            {
                char* delims = " \t\n\r";    
                char* token = NULL;

                token = strtok(str, delims); /* to #define */
                token = strtok(NULL, delims); /* to NAME */

                if (token == NULL)
                {
                    fprintf(stderr, "Line %d: Invalid #define directive.\n", ln_count);
                    isBad = 1;
                    break;
                }
                
                char* bpos = strchr(token, '(');
                if (bpos != NULL)
                {
                    *bpos = 0;
                }
                
                *pstrUndefs = str_append(*pstrUndefs, "#undef ");
                *pstrUndefs = str_append(*pstrUndefs, token);
                *pstrUndefs = str_append(*pstrUndefs, "\n");
            }
        }
    }
    while (!feof(fl) && !ferror(fl));
    
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for DEFINE section.\n", ln_count);
        isBad = 1;
    }
    
    free (str);

    return (!isBad);
}

static char*
parse_code(FILE* fl, int size)
{
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

    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, bl_tag[IBL_CODE]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            text = str_append (text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));
    
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for CODE section.\n", ln_count);
        isBad = 1;
    }

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
    
    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments (str, size, fl, &ln_count);

        if (t2c_parse_close_tag(str, bl_tag[IBL_FINALLY]))
        {
            close_tag_found = 1;
            break;
        }
        else
        {
            text = str_append (text, str);
        }
    }
    while (!feof(fl) && !ferror(fl));
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for FINALLY section.\n", ln_count);
        isBad = 1;
    }

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
    char* str = NULL;
    char* str_t = NULL;
    int isBad = 0;
    char* text = NULL;
    int i = 0;
    int	j;
    char *warning_text = NULL;           
	TPurpose *purpose = NULL;

	purpose = (TPurpose *)alloc_mem (purpose, 2, sizeof (TPurpose));
	purpose->nLines = 0;
	warning_text = alloc_mem_for_string (warning_text, MAX_STRING_LEGTH);

    if (!fl)
    {
        return NULL;
    }

    if (feof(fl) || ferror(fl))
    {
        return NULL;
    }

    str = alloc_mem_for_string(str, size + 1);
    
    int close_tag_found = 0;
    do
    {
        fgets_and_skip_comments(str, size, fl, &ln_count);
        
        if (t2c_parse_close_tag(str, bl_tag[IBL_PURPOSE]))
        {
            close_tag_found = 1;
            free(text);
            text = strdup(templ);
            if (purpose->nLines == 0)
            {
			    ++(*purposes_number);   
	            text = replace_all_substr_in_string(text, "<%params%>", "//    none\n");
	            sprintf(str, "%d", *purposes_number);
    	        text = replace_all_substr_in_string(text, tags[PURPNUM_TAG_POS], str);
	            text = replace_all_substr_in_string(text, TAG_FINALLY_NAME, finally_code);
    	        break;
            }
            
			char 	*text1 = NULL;
			int 	*purp_num = NULL;
			long 	*purp_array = NULL;

			purp_array = alloc_mem (purp_array, MAX_LINES, sizeof (long));
			purp_num = alloc_mem (purp_num, 2, sizeof (int));
			*purp_num = 1;
			text1 = strdup (text);
	
			parameter_and_text_generator (purpose, &text, text1, finally_code, purp_array, *purposes_number, 0, purp_num);

			*purposes_number += *purp_num - 1;	

			free (text1);
			free (purp_num);
			free (purp_array);
            break;
        }
        else
        {
            str_t = trim_with_nl (str);
            if (strlen(str_t) > 0)
            {
                if (purpose->nLines >= MAX_PARAMS_NUM)
                {
                    isBad = 1;
                    fprintf (stderr, "Line %d: Too many parameters are specified for the test purpose.\n",
                        ln_count);
                    break;
                }

				purpose->Lines[purpose->nLines] = NULL;
                purpose->Lines[purpose->nLines] = (TLine *)alloc_mem (purpose->Lines[purpose->nLines], 2, sizeof (TLine));
                parse_purpose_line (str_t, purpose->Lines[purpose->nLines], &warning_text);                
                purpose->nLines++;

                if (strcmp (warning_text, WARNING_OK) != 0)
                {
                	fprintf (stderr, "Line %d: %s", ln_count, WARNING_PREFIX);
                	fprintf (stderr, "%s\n", warning_text);
                }
            }
        }
    }
    while (!feof(fl) && !ferror(fl));
    
    if (!close_tag_found)
    {
        fprintf (stderr, "Line %d: No close tag found for PURPOSE section.\n", ln_count);
        isBad = 1;
    }

    free(str);
    free(warning_text);
    for (i = 0; i < purpose->nLines; i++)    
    {
    	for (j = 0; j < purpose->Lines[i]->nComponents; j++)
    	{
    		if (purpose->Lines[i]->Components[j]->Type == COMPONENT_TYPE_STRING)
    		{
    			free (purpose->Lines[i]->Components[j]->str);
    		}
    		free (purpose->Lines[i]->Components[j]);
    	}
    	free (purpose->Lines[i]);
    }
    free (purpose);
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
    char* common_mk_data = NULL;
    char* main_mk_path = NULL;
    
    char* path1 = NULL;
    char* path2 = NULL;

    char* suite_inc = NULL;
    
    char* tmk_path  = NULL;
    char* tpl_path1 = NULL;
    char* tpl_path2 = NULL;

    tpl_path1 = alloc_mem_for_string(tpl_path1, strlen(t2c_root)); 
    tpl_path1 = str_append(tpl_path1, t2c_root);
    tpl_path2 = concat_paths(tpl_path1, COMMON_TPL_DIR);
    tmk_path = concat_paths(tpl_path2,  COMMON_MAKEFILE_TPL);
    
    // Load the template for common.mk first
    FILE* tmk = open_file(tmk_path, "r", NULL);
    if (!tmk)
    {
        fprintf(stderr, "Unable to open file: %s\n", tmk_path);
        free(tpl_path1);
        free(tpl_path2);
        free(tmk_path);
        return;
    }
    
    common_mk_data = read_file_to_string(tmk);
    if (!common_mk_data)
    {
        fprintf(stderr, "Unable to read from file: %s\n", tmk_path);
        free(tpl_path1);
        free(tpl_path2);
        free(tmk_path);
        return;
    }
    fclose(tmk);
    
    FILE* mf = NULL;   // common.mk

    common_mk_path = str_sum(output_dir_path, "common.mk");
        
    mf = open_file(common_mk_path, "w", NULL);

    if (!mf)
    {
        free (common_mk_path);
        free(tpl_path1);
        free(tpl_path2);
        free(tmk_path);
        fprintf(stderr, "Unable to create common makefile in %s\n", output_dir_path);
        return;
    }

    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_DIR_PATH_POS],
        test_dir);
    
    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_COMPILER_POS],
        cfg_parm_values[CFG_COMPILER_POS]);
    
    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_ADD_CFLAGS_POS],
        cfg_parm_values[CFG_COMP_FLAGS_POS]);

    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_ADD_LFLAGS_POS],
        cfg_parm_values[CFG_LINK_FLAGS_POS]);

    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_TEST_STD_FLAGS_POS],
        (bGenCpp ? "TEST_STD_CFLAGS_CPP" : "TEST_STD_CFLAGS_C"));

    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_TEST_FILE_EXT_POS],
        (bGenCpp ? "cpp" : "c"));

    common_mk_data = replace_all_substr_in_string(
        common_mk_data, 
        common_mk_params[CMK_SP_FLAG_POS],
        (bSingleProcess ? "-DT2C_SINGLE_PROCESS" : ""));
    
    fputs(common_mk_data, mf);
       
    free(common_mk_path);
    free(common_mk_data);
    fclose(mf);
    
    free(tpl_path1);
    free(tpl_path2);
    free(tmk_path);

    //------------------------------------------------------------------
    // Generate main makefile (it will invoke makefiles in each subdir).
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

    fprintf (mf, "EXCLUDE = Makefile common.mk objs\n\n");

    fprintf (mf, "SUBDIRS := $(filter-out $(EXCLUDE), $(wildcard *))\n");
    fprintf (mf, "BUILDDIRS = $(SUBDIRS)\n");
    fprintf (mf, "CLEANDIRS = $(addprefix _clean_,$(SUBDIRS))\n");
    fprintf (mf, "STB_DIRS = $(addprefix _stb_,$(SUBDIRS)) \n\n");

    fprintf (mf, "all:    $(BUILDDIRS)\n\n");

    fprintf (mf, "$(BUILDDIRS):\n");
    fprintf (mf, "\t$(MAKE) -C $@\n\n");

    fprintf (mf, "clean: $(CLEANDIRS)\n\n");
    fprintf (mf, "$(CLEANDIRS):\n");
    fprintf (mf, "\t$(MAKE) -C $(subst _clean_,,$@) clean\n\n");
    
    fprintf (mf, "standalone: $(STB_DIRS)\n\n");    
    fprintf (mf, "$(STB_DIRS):\n");
    fprintf (mf, "\t$(MAKE) -C $(subst _stb_,,$@) debug\n\n");

    fprintf (mf, ".PHONY:    all $(BUILDDIRS) $(CLEANDIRS) clean standalone $(STB_DIRS)\n\n");

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
        free(cfg_parm_values[i]);
    }
    
    cfg_parm_values[CFG_COMPILER_POS]   = (char *)strdup("gcc");
    cfg_parm_values[CFG_COMP_FLAGS_POS] = (char *)strdup("");
    cfg_parm_values[CFG_LINK_FLAGS_POS] = (char *)strdup("");
    cfg_parm_values[CFG_TET_RECORD_POS] = (char *)strdup("yes");
    cfg_parm_values[CFG_WAIT_TIME_POS]  = (char *)strdup("30");
    cfg_parm_values[CFG_LANGUAGE_POS]   = (char *)strdup("C");
    cfg_parm_values[CFG_SINGLE_POS]     = (char *)strdup("no");
    cfg_parm_values[CFG_MK_TPL_POS]     = (char *)strdup("");
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
            strcmp(cfg_parm_values[CFG_TET_RECORD_POS], "YES")) // value is not "yes"
        {
            bAddTetScenRecord = 0;
        }
        
        int waittime = atoi(cfg_parm_values[CFG_WAIT_TIME_POS]);
        if (waittime == 0)
        {
            sprintf(cfg_parm_values[CFG_WAIT_TIME_POS], "0");
        }
        
        if (!strcmp(cfg_parm_values[CFG_LANGUAGE_POS], "cpp") ||
            !strcmp(cfg_parm_values[CFG_LANGUAGE_POS], "CPP"))
        {
            bGenCpp = 1;
        }
        
        if (!strcmp(cfg_parm_values[CFG_SINGLE_POS], "yes") ||
            !strcmp(cfg_parm_values[CFG_SINGLE_POS], "YES")) // value is "yes"
        {
            bSingleProcess = 1;
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
                free(cfg_parm_values[i]);
                cfg_parm_values[i] = (char*)strdup(str_value);
                break;
            }
        }
    }

    free(str);
}

/*******/

