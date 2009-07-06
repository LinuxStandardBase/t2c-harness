#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/libfile.h"
#include "../include/libstr.h"
#include "../include/libmem.h"

#define TRUE 1
#define FALSE 0

/*
 * This library contains various functions for i/o operations processing
 */

static char* get_app_name ();


/* 
 * Change the working directory
 * Returns:
 *   old working directory
 */
char*
change_dir (char *dirname)
{
    int err;

    char* old_dir = NULL;
    old_dir = alloc_mem_for_string (old_dir, 1024);
    old_dir = getcwd (old_dir, 1024);

    err = chdir (dirname);
    if (err) 
    {
        error_with_exit_code (EXIT_CODE_CANT_CHDIR,
                              "%s: can't change a directory to %s",
                              get_app_name(), dirname);
        return NULL;
    }
    
    return old_dir;
}


/*
 * Concatenates two paths.
 * E.g. was 'dir1/dir2', 'dir3'; now 'dir1/dir2/dir3'
 * The return pointer should be freed.
 */
char* 
concat_paths (char* path1, char* path2)
{
    char* res = NULL;
    res = alloc_mem_for_string (res, strlen (path1));

    res = str_append (res, path1);
    if (strrchr (res, '/')  - res != strlen (res) - 1 
        && *path2 != '/')
    {
        res = str_append (res, "/");
    }
    res = str_append (res, path2);

    return res;
}

/* 
 * Create a directory
 */
void 
create_dir (char *dirname)
{
    int err;
    char* dir = NULL;

    dir = alloc_mem_for_string (dir, 1024);
    err = mkdir (dirname, /*S_ISVTX | */ S_IRWXU | S_IRWXG | S_IRWXO);

    if (err) 
    {
        error_with_exit_code (EXIT_CODE_CANT_MKDIR,
                              "%s: can't create a directory: %s\n%s: current working directory is: %s\n",
                              get_app_name(), dirname,
                              get_app_name(), getcwd (dir, 1024));
    }
    free (dir);
}

/*
 * Print an error message and exit with the exit code specified
 */
void 
error_with_exit_code (int code,const char* tpl, ...)
{
    va_list ap;
    va_start (ap,tpl);
    vfprintf(stderr,tpl,ap);
    va_end (ap);
    fprintf(stderr,"\n");
    exit(code);
}

static char* 
get_app_name ()
{
    return "libfile";
}

/*
 * Checks if a directory with the specified name exists
 */
unsigned
is_directory_exists (char *dir_name)
{
    int err;
    unsigned res = FALSE;
    char* dir = NULL;


    dir = alloc_mem_for_string (dir,1024);
    dir = getcwd(dir,1024);

    err = chdir (dir_name);

    if (err)
    {
        res = FALSE;
    } else {
        err = chdir(dir);
        res = TRUE;
    }

    free (dir);

    return res;
}

/*
 * Checks if a file with the specified name exists
 */
unsigned
is_file_exists (const char* file_name)
{
    FILE* pfile;

    pfile = fopen (file_name, "r");

    if (!pfile)
    {
        return FALSE;
    }
    else
    {
        fclose (pfile);
        return TRUE;
    }
}

/* 
 * Open a file.
 * Returns:
 *    opened file descriptor
 */

FILE* 
open_file (const char* filename, const char *mode, 
           const char* correct_extension)
{
    FILE* pfile;

    if (strlen (filename))
    {
        if (correct_extension)
        {
            if (strcmp (filename + strlen (filename) 
                        - strlen(correct_extension), correct_extension))
            {
                error_with_exit_code 
                    (WRONG_EXTENSION_FILE_PASSED,
                     "%s: wrong file extension passed (the correct one is %s)",
                     get_app_name(),correct_extension);
                return NULL;
            }
        }
    }
    
    pfile = fopen (filename, mode);
    
    if (!pfile)
    {
        error_with_exit_code (EXIT_CODE_CANT_OPEN_FILE,
                              "%s: can't open file %s\n",
                              get_app_name(),filename);
        return NULL;
    }

    return pfile;
}

/*
 * Get the directory name of the file with the path specified
 */
char*
get_dir (char* path)
{
    FILE *pfile;
    int i,j;
    char *tmp = NULL;
    int len;

    len = strlen(path);

    if(is_directory_exists(path)) 
        return path;
    else
    {
        pfile = fopen (path,"r");
        if (!pfile)
        {
            error_with_exit_code (EXIT_CODE_CANT_OPEN_FILE,
                                  "%s: this path doesn't exists  %s",
                                  get_app_name(),path);
            return NULL;
        }
        else
        {
            fclose(pfile);
            tmp = alloc_mem_for_string (tmp, 1024);
            i = len;
            while(path[i]!='/') i--;
            for(j=0;j<=i;j++)
                tmp[j] = path[j];
            return tmp;
        }
    }
}

/*
 * Return the size of the file
 *
 */
int 
fsize (FILE* pFile)
{
    long size, fposSave;
    fposSave = ftell(pFile);
    fseek (pFile, 0, SEEK_END);
    size = ftell(pFile);
    fseek (pFile, fposSave, SEEK_SET);
    return size;
}

/*
 * Read all data from the pFile and stores it in the 
 * specified buffer.
 * The returned pointer should be freed.
 */
char* 
read_file_to_string (FILE* pFile)
{
    long lSize, fposSave;
    char* buffer = NULL;
    if (pFile==NULL) { return (NULL);};

    /* obtain file size.*/
    fposSave = ftell (pFile);
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    /* allocate memory to contain the whole file. */
    buffer = alloc_mem_for_string (buffer, lSize + 1);
    if (buffer == NULL) { return (NULL);};

    /* copy the file into the buffer.*/
    fread (buffer,1,lSize,pFile);
    buffer[lSize] = '\0';
    fseek (pFile, fposSave, SEEK_SET);
    return (buffer);
}

/*
 * Converts the given path to be as short as possible
 * (e.g. was '/dir1/../dir2/dir3', now '/dir2/dir3'
 */ 

char*
shorten_path (char* path_to_short)
{
    char* pch;
    char* path;
    char* res = NULL;
    
    path = (char*) strdup (path_to_short);
    res = alloc_mem_for_string (res, strlen (path_to_short));

    pch = strtok (path, "/");
    while (pch)
    {
        if (!strcmp (pch, "."))
        {
            /* do nothing with '.', just skip */
        } else if (!strcmp (pch, "..")) {
            if (res && strlen (res) > 0)
            {
                /* the last symbol in res is '.' */
                if (*(res + strlen (res) - 1) == '.')
                {
                    res = str_append (res, "/");
                    res = str_append (res, pch);
                } else {
                    char* last_slash;
                    last_slash = strrchr (res, '/');
                    if (last_slash)
                    {
                        *last_slash = '\0';
                        res = alloc_mem_for_string (res, strlen (res) + 1);
                    } else {
                        *res = '\0';
                        res = alloc_mem_for_string (res, 0);
                    }
                }
            } else {
                res = str_append (res, pch);
            }
        } else {
            res = str_append (res, "/");
            res = str_append (res, pch);
        }
        pch = strtok (NULL, "/");
    }
                
    free (path);

    return res;
}
    
