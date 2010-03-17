#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/libstr.h"
#include "../include/libmem.h"

#define HAVE_STRDUP 1

char*
replace_char_in_string (char* src, char what_to_replace, char replacement)
{
    char* ret_val = NULL;
    char *prval;

    if (!src)
    {
        return NULL;
    }

    ret_val = alloc_mem_for_string (ret_val, strlen (src));
    ret_val = strcpy (ret_val, src);

    prval = strchr (ret_val, what_to_replace);
    while (prval)
    {
        *(prval) = replacement;
        prval = strchr (prval + 1, what_to_replace);
    }

    return ret_val;
}

/*
 * Scans dest for the first occurence of str1
 * If str1 is found then replace it with str2
 * Returns string where all occurences of str1
 * replaced with str2 if it found in dest else 
 * returns NULL 
 */
char*
replace_substr_in_string (char* destination, const char* str1, const char* str2)
{
    char* pch = NULL;
    char* tailSave = NULL;
    char* dest = NULL;
    size_t headLen, tailLen, newDestLen;

    if ((destination == NULL) || (str1 == NULL)) { return (NULL);};

    dest = (char*) strdup(destination);
    if (str2 == NULL)
    {
        return (dest);
    }

    pch = strstr(dest, str1);
    if(!pch) {return (dest);};

    headLen = pch - dest;
    tailLen =  strlen(pch) - strlen(str1);
    newDestLen = strlen(dest) - strlen(str1) + strlen(str2);

    tailSave = (char*) calloc (tailLen, sizeof (char));
    memcpy(tailSave, pch + strlen(str1), tailLen);
    dest = (char*) realloc(dest, newDestLen * sizeof (char) + 1);
    memcpy (dest + headLen, str2, strlen (str2));
    memcpy (dest + headLen + strlen (str2), tailSave, tailLen);
    dest[newDestLen] = '\0';

    free (tailSave);
    return (dest);
}

/*
 * Scans dest for the all occurence of str1
 * If str1 is found then replace it with str2
 * Returns string where all occurences of str1
 * replaced with str2 
 */
char*
replace_all_substr_in_string(char* destination, const char* str1, const char* str2)
{
    char* pch = NULL;
    size_t headLen, tailLen, newDestLen;
    
    if ((destination == NULL) || (str1 == NULL)) { return (NULL);};

    if (str2 == NULL)
    {
        printf ("\nlibstr warning: arg3=NULL passed to replace_all_substr_in_string()\n\n");
        return (destination);
    }

    pch = strstr(destination, str1);

    while (pch != NULL)
    {
        char* tailSave = NULL;
    
        headLen = pch - destination;
        tailLen =  strlen(pch) - strlen(str1);
        newDestLen = strlen(destination) - strlen(str1) + strlen(str2);

        if (tailLen != 0)
        {
            tailSave = (char*) calloc (tailLen, sizeof (char));
        }

        memcpy(tailSave, pch + strlen(str1), tailLen);
        destination = (char*) realloc(destination, (newDestLen + 1) * sizeof (char));
        memcpy (destination + headLen, str2, strlen (str2));
        memcpy (destination + headLen + strlen (str2), tailSave, tailLen);
        destination[newDestLen] = '\0';

        pch = strstr(destination + headLen + strlen (str2), str1);
        if (tailSave != NULL)
        {
            free (tailSave);
        }
    }

    return destination;
}
                
/*
 * Removes white space from both ends of a string
 * Returns a string without spaces at the both ends
 */
char*
trim (char* str)
{
    size_t new_start_pos;
    size_t new_end_pos;
    size_t i;

    new_start_pos = 0;
    new_end_pos = strlen (str) - 1;

    if (!str)
    {
        return NULL;
    }

    if (!strlen (str))
    {
        return str;
    }

    for (i = 0; i < strlen (str); i++)
    {
        new_start_pos = i;
        if ((str[i] != ' ') && (str[i] != '\t'))
        {
            break;
        }
    }
    
    /* if a string containing spaces only is passed we should return "" */
    if ((new_start_pos == new_end_pos) 
        && ((str[new_start_pos] == ' ') || (str[new_start_pos] == '\t')))
    {
        str[0] = '\0';
        return str;
    }

    for (i = strlen (str) - 1; i > 0; i--)
    {
        new_end_pos = i;
        if ((str[i] != ' ') && (str[i] != '\t'))
        {
            break;
        }
    }

    memmove (str, str + new_start_pos, new_end_pos - new_start_pos + 1);
    *(str + new_end_pos - new_start_pos + 1) = '\0';
    
    return str;
}
       
/*
 * Removes white spaces and newlines from both ends of a string
 * Returns a string without spaces and newlines at the both ends
 */
char*
trim_with_nl (char* str)
{
    size_t new_start_pos;
    size_t new_end_pos;
    size_t i;

    new_start_pos = 0;
    new_end_pos = strlen (str) - 1;

    if (!str)
    {
        return NULL;
    }

    if (!strlen (str))
    {
        return str;
    }

    for (i = 0; i < strlen (str); i++)
    {
        new_start_pos = i;
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
        {
            break;
        }
    }
    
    /* if a string containing spaces and '\n's only is passed we should return "" */
    if ((new_start_pos == new_end_pos) 
        && ((str[new_start_pos] == ' ' ) 
            || (str[new_start_pos] == '\n')
            || (str[new_start_pos] == '\t')))
    {
        str[0] = '\0';
        return str;
    }

    for (i = strlen (str) - 1; i > 0; i--)
    {
        new_end_pos = i;
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t')
        {
            break;
        }
    }

    memmove (str, str + new_start_pos, new_end_pos - new_start_pos + 1);
    *(str + new_end_pos - new_start_pos + 1) = '\0';
    
    return str;
}

/*
 * Appends one string to another
 * Returns str1 where old str1 and str2 are concatenated.
 */
char* 
str_append (char* str1, const char* str2)
{
    str1 = (char*) realloc (str1,
                           (strlen(str1) + strlen(str2) + 1) * sizeof(char));
    strcat(str1, str2);
    return str1;
}



/*
 * Concatenates two strings.
 * Returns string where str1 and str2 are concatenated.
 * If str1 is NULL then str2 is returned.
 * If str2 is NULL then str1 is returned.
 * If str1 and str2 are both NULL then NULL is returned.
 */
char* 
str_sum (const char* str1, const char* str2)
{
    char* sum = NULL;

    if (!str1 && !str2)
    {
        return NULL;
    }

    if (!str1)
    {
        sum = (char*) calloc(strlen(str2) + 1, sizeof(char));
        strcpy (sum, str2);
        return sum; 
    }

    if (!str2)
    {
        sum = (char*) calloc(strlen(str1) + 1, sizeof(char));
        strcpy (sum, str1);
        return sum; 
    }

    sum = (char*) calloc(strlen(str1) + strlen(str2) + 1, sizeof(char));
    strcpy(sum, str1);
    strcat(sum, str2);
    return sum;    
}

/*
 * Returns following  substring of source source: 
 * source[start_pos]...source[end_pos]
 */
char* 
get_substr (const char* source, long start_pos, long end_pos)
{
    long length;
    char* substr = NULL;
    if ((start_pos > end_pos) || (start_pos < 0) ||
        (end_pos > (strlen (source) - 1))) 
    {
        return NULL;
    }

    length = end_pos - start_pos + 1;
    substr = (char*) calloc (length + 1, sizeof(char));
    strncpy(substr, source + start_pos, length);
    substr[length] = '\0';
    return substr;    
}


char*
do_indent (char* source, int spaces_num)
{
    char* result = NULL;
    char* indent = NULL;
    char* pSave = NULL;

    if (source == NULL) return (NULL);
    result = (char*) strdup (source);
    if (spaces_num < 1) return (result);
    indent = (char*) calloc (spaces_num + 2, sizeof(char));
    memset (indent + 1, ' ', spaces_num);
    indent[0] = '\n';
    indent[spaces_num + 1] = '\0';
    result = replace_all_substr_in_string (result, "\n", indent);
    free (indent);

    return (result);
}

/*
 * Convert str into C style comment string   
 */

char*  
convert_to_comment (const char* str)
{
    char* result = NULL;
    char* pSave = NULL;

    result = str_sum ("/*\n", str);
    if (result[strlen (result) - 1] == '\n')
    {
        result[strlen (result) - 1] = ' ';
    }
    result = replace_all_substr_in_string (result, "\n", "\n * ");

    result = str_append (result, " \n */");
    pSave = result;
    result = str_sum ("\n", result);
    free (pSave);

    return (result);
}
