#ifndef LIBFILE_H
#define LIBFILE_H

#define EXIT_CODE_CANT_CHDIR 101
#define EXIT_CODE_CANT_MKDIR 102
#define WRONG_EXTENSION_FILE_PASSED 103
#define EXIT_CODE_CANT_OPEN_FILE 104
#define EXIT_CODE_CANT_OPEN_DIR 105
#define EXIT_CODE_INPUT_PARSE_ERROR 106

#ifdef __cplusplus
extern "C"
{
#endif 

extern char* change_dir (char *dirname);
extern char* concat_paths (char* path1, char* path2);
extern void create_dir (char *dirname);
extern char* get_dir (char* path);
extern unsigned is_directory_exists (char *dir_name);
extern unsigned is_file_exists (const char* file_name);
extern FILE* open_file (const char* filename, const char *mode, 
                 const char* correct_extension);
extern void error_with_exit_code (int code,const char* tpl,...);
extern int fsize(FILE* pFile);
extern char* read_file_to_string (FILE* pFile);
extern char* shorten_path (char* path_to_shorten);

#ifdef	__cplusplus
}
#endif
    
#endif /* LIBFILE_H */
