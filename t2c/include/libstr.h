#ifndef LIBSTR_H
#define LIBSTR_H

#ifdef __cplusplus
extern "C"
{
#endif 

extern char* trim (char* str);
extern char* trim_with_nl (char* str);
extern char* replace_char_in_string (char* src, char what_to_replace, char replacement);
extern char* replace_substr_in_string(char* destination, const char* str1, const char* str2);
extern char* replace_all_substr_in_string(char* destination, const char* str1, const char* str2);
extern char* str_append (char* str1, const char* str2);
extern char* str_sum (const char* str1, const char* str2);
extern char* get_substr (const char* source, long start_pos, long end_pos);
extern char* do_indent (char* source, int spaces_num);
extern char* convert_to_comment (const char* str);
extern char* mark_symbol (int n);

#ifdef	__cplusplus
}
#endif
    
#endif /* LIBSTR_H */
