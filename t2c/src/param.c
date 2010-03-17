#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/libmem.h"
#include "../include/libstr.h"
#include "../include/param.h"

int
char_in_string (const char c, const char *s)
{
    if (index (s, c))
    {
        return TRUE;
    }

    return FALSE;
}

void
add_char_to_string (const char c, char *s)
{
    int         len;
    
    len = strlen (s);
    s[len] = c;
    s[len + 1] = NULL_TERMINATOR;
}

int
string_is_number (char *s)
{
    int         i;
    
    for (i = 0; i < strlen (s); i++)
    {
        if (!(char_in_string (s[i], NUMBERS)))
        {
            return FALSE;
        } 
    }
    
    return TRUE;
}

void
parse_purpose_line (const char *string, TLine *line, char **warning)
{
    int         state, prev_bslash_state, prev_apostr_state;
    char        ch = NULL_TERMINATOR;
    char        next_ch = NULL_TERMINATOR;
    char        *str = NULL;
    char        *str_ = NULL;
    char        *comp = NULL;
    int         continue_for;
    int         mb_indicator;
    int         set_len, res_len;
    int         is_set;
    int         is_interval;
    int         interval_available;
    int         wo_close_bracket;
    int         numbers_began, numbers_ended;
    int         is_warning;
    int         i;
    int			in_apostr = 0;
            
    size_t tlen = 0;
    state = STATE_INIT;
    line->nComponents = -1;
    str_ = strdup (string);
    for (i = 0; i < strlen (str_); i++)
    {
    	if (i > 0)
    	{
	    	if ((str_[i] == SHARP) && (str_[i - 1] != BSLASH) && (!in_apostr) && ((str_[i - 1] != APOSTR) || (str_[i + 1] != APOSTR)))
    		{
	    		str_[i] = NULL_TERMINATOR;
    			str_ = trim (str_);
    			break;
    		}
    		if ((str_[i] == QUOT) && (str_[i - 1] != BSLASH))
    		{
	    		in_apostr = 1 - in_apostr;
    		}
    	}
    	else
    	{
	    	if (str_[i] == SHARP)
    		{
	    		str_[i] = NULL_TERMINATOR;
    			str_ = trim (str_);
    			break;
    		}
    		if (str_[i] == QUOT)
    		{
	    		in_apostr = 1 - in_apostr;
    		}
    	}	
    }
    
    str = str_;
    tlen = strlen (str);
    comp = alloc_mem_for_string (comp, tlen + 1);
    mb_indicator = -1;
    wo_close_bracket = TRUE;
    interval_available = TRUE;
    is_warning = FALSE;
    strcpy (*warning, WARNING_OK);
    set_len = strlen (SET);
    res_len = strlen (RES);
    
    continue_for = TRUE;
    char last_ch = (tlen > 0) ? str[tlen - 1] : 0;
    for (ch = *str; continue_for;)
    {
        if (ch == NULL_TERMINATOR)
        {
            continue_for = FALSE;
        }
        else
        {
            next_ch = str[1];
        }
        
        switch (state)
        {
            case STATE_INIT:
            {
                if (char_in_string (ch, SPTAB))
                {
                    break;
                }
                 
                if ((ch != *SET) && (ch != *RES))
                {
                    state = STATE_COMMON;
                    break;
                }
                
                if (ch == *SET)
                {
                    mb_indicator = 0;
                    state = STATE_MB_SET;
                    break;
                }
                
                if (ch == *RES)
                {
                    mb_indicator = 0;
                    state = STATE_MB_RES;
                    break;
                }
                
                break;
            }
            
            case STATE_COMMON:
            {
                if (is_warning)
                {
                    for (i = 0; i < line->nComponents; i++)
                    {
                        if (line->Components[i]->Type == COMPONENT_TYPE_STRING)
                        {
                            free (line->Components[i]->str);
                        }
                        free (line->Components[i]);
                    }
                }
                
                line->Type = LINE_TYPE_COMMON; 
                line->nComponents = 1;
                
                line->Components[0] = NULL;
                line->Components[0] = alloc_mem (line->Components[0], 2, sizeof (TComponent));
                line->Components[0]->Type = COMPONENT_TYPE_STRING;
                line->Components[0]->str = NULL;
                line->Components[0]->str = alloc_mem_for_string (line->Components[0]->str, strlen (str_) + 1);
                strcpy (line->Components[0]->str, str_);
                line->Components[0]->n = 1;
                
                wo_close_bracket = FALSE;
                continue_for = FALSE;               
                break;
            }
            
            case STATE_MB_SET:
            {
                mb_indicator++;
                
                if (mb_indicator < set_len)
                {
                    if (ch != SET[mb_indicator])
                    {
                        state = STATE_COMMON;
                    }
                }
                else
                {
                    if (ch == BRACKET_OPEN)
                    {
                        state = STATE_SET_RES;
                        
                        if (last_ch == BRACKET_CLOSE)
                        {
                            wo_close_bracket = FALSE;
                        }
                        
                        is_set = TRUE;
                        is_interval = FALSE;
                        strcpy (comp, EMPTY_STRING);
                        line->Type = LINE_TYPE_SET; 
                        line->nComponents = 0;
                        break;
                    }
                    
                    if (char_in_string (ch, SPTAB))
                    {
                        break;
                    }
                    
                    state = STATE_COMMON;  
                }
                
                break;
            }
            
            case STATE_MB_RES:
            {
                mb_indicator++;
                
                if (mb_indicator < res_len)
                {
                    if (ch != RES[mb_indicator])
                    {
                        state = STATE_COMMON;
                    }
                }
                else
                {
                    if (ch == BRACKET_OPEN)
                    {
                        state = STATE_SET_RES;
                        
                        if (last_ch == BRACKET_CLOSE)
                        {
                            wo_close_bracket = FALSE;
                        }
                        
                        is_set = FALSE;
                        is_interval = FALSE;
                        strcpy (comp, EMPTY_STRING);
                        line->Type = LINE_TYPE_RES; 
                        line->nComponents = 0;
                        break;
                    }
                    
                    if (char_in_string (ch, SPTAB))
                    {
                        break;
                    }
                    
                    state = STATE_COMMON;  
                }
                break;
            }
            
            case STATE_SET_RES:
            {
                if ((ch == SEMICOLON) || ((ch == COLON) && (line->Type == LINE_TYPE_RES)) || 
                    ((ch == BRACKET_CLOSE) && (next_ch == NULL_TERMINATOR)))
                {
                    if (is_interval)
                    {
                        trim (comp);
                        if (!(string_is_number (comp)))
                        {
                            is_warning = TRUE;
                            strcpy (*warning, WARNING_INTERVAL);
                            state = STATE_COMMON;
                            break;              
                        }
                        if (strlen (comp) > MAX_INT_LENGTH_IN_STR_FORM)
                        {
                            is_warning = TRUE;
                            strcpy (*warning, WARNING_MAX_INT);
                            state = STATE_COMMON;
                            break;              
                        }
                        
                        is_interval = FALSE;
                        line->Components[line->nComponents]->b = atoi (comp);
                        line->nComponents++;
                    }
                    else
                    {
                        line->Components[line->nComponents] = NULL;
                        line->Components[line->nComponents] = alloc_mem (line->Components[line->nComponents], 2, sizeof (TComponent));
                        line->Components[line->nComponents]->Type = COMPONENT_TYPE_STRING;
                        line->Components[line->nComponents]->str = NULL;
                        line->Components[line->nComponents]->str = alloc_mem_for_string (line->Components[line->nComponents]->str, strlen (comp) + 1);
                        strcpy (line->Components[line->nComponents]->str, comp);
                        line->Components[line->nComponents]->n = 1;
                        line->nComponents++;
                    }
                    
                    if (ch == SEMICOLON)
                    {
                        state = STATE_SET_RES; 
                    }
                    else if (ch == COLON)
                    {
                        line->nComponents--;
                        numbers_began = FALSE;
                        numbers_ended = FALSE;
                        state = STATE_COLON;
                    }
                    else if (ch == BRACKET_CLOSE)
                    {
                        wo_close_bracket = FALSE;
                        state = STATE_END;
                    }
                    
                    interval_available = TRUE;
                    strcpy (comp, EMPTY_STRING);
                    break;
                } 
                
                if (ch == BSLASH)
                {
                    prev_bslash_state = STATE_SET_RES; 
                    state = STATE_BSLASH;
                    break;
                }
                
                if (ch == APOSTR)
                {
                    prev_apostr_state = STATE_SET_RES;
                    state = STATE_APOSTR;
                }
                
                if ((ch == DOT) && (interval_available) && (line->Type == LINE_TYPE_SET))
                {
                    state = STATE_INTERVAL;
                    break;
                }
                
                add_char_to_string (ch, comp);
                break;
            }
            
            case STATE_BSLASH:
            {
                //if ((prev_bslash_state == STATE_APOSTR) && (ch != APOSTR))
                {
                    add_char_to_string (BSLASH, comp);  
                }
                
                add_char_to_string (ch, comp);
                state = prev_bslash_state;
                break;
            }           
            
            case STATE_APOSTR:
            {
                if (ch == BSLASH)
                {
                    prev_bslash_state = STATE_APOSTR; 
                    state = STATE_BSLASH;
                    break;
                }
                
                if (ch == APOSTR)
                {
                    state = prev_apostr_state;
                }
                
                add_char_to_string (ch, comp);
                break;
            }
            
            case STATE_INTERVAL:
            {
                if (ch == DOT)
                {
                    trim (comp);
                    if (!(string_is_number (comp)))
                    {
                        is_warning = TRUE;
                        strcpy (*warning, WARNING_INTERVAL);
                        state = STATE_COMMON;
                        break;              
                    }
                    if (strlen (comp) > MAX_INT_LENGTH_IN_STR_FORM)
                    {
                        is_warning = TRUE;
                        strcpy (*warning, WARNING_MAX_INT);
                        state = STATE_COMMON;
                        break;              
                    }
                    
                    is_interval = TRUE;
                    interval_available = FALSE;

                    line->Components[line->nComponents] = NULL;
                    line->Components[line->nComponents] = alloc_mem (line->Components[line->nComponents], 2, sizeof (TComponent));
                    line->Components[line->nComponents]->Type = COMPONENT_TYPE_INTERVAL;
                    line->Components[line->nComponents]->a = atoi (comp);
                    line->Components[line->nComponents]->n = 1;
                    strcpy (comp, EMPTY_STRING);
                }
                else
                {
                    add_char_to_string (DOT, comp);
                    add_char_to_string (ch, comp);
                }
                
                state = STATE_SET_RES;
                break;
            }
            
            case STATE_COLON:
            {
                
                if (char_in_string (ch, SPTAB))
                {
                    if (numbers_began)
                    {
                        numbers_ended = TRUE;
                    }
                    break;
                }
                
                if (char_in_string (ch, NUMBERS))
                {
                    if (numbers_ended)
                    {
                        is_warning = TRUE;
                        strcpy (*warning, WARNING_COLON);
                        state = STATE_COMMON;
                        break;
                    }
                    
                    add_char_to_string (ch, comp);
                    numbers_began = TRUE;
                    break;
                }

                if ((ch == SEMICOLON) || (ch == BRACKET_CLOSE))
                {
                    if (strlen (comp) > MAX_INT_LENGTH_IN_STR_FORM)
                    {
                        is_warning = TRUE;
                        strcpy (*warning, WARNING_MAX_INT);
                        state = STATE_COMMON;
                        break;
                    }
                    
                    line->Components[line->nComponents]->n = atoi (comp);
                    line->nComponents++;

                    if (ch == SEMICOLON)
                    {
                        state = STATE_SET_RES; 
                    }
                    else 
                    {
                        wo_close_bracket = FALSE;
                        state = STATE_END;
                    }
                    
                    strcpy (comp, EMPTY_STRING);
                    break;
                }
                
                is_warning = TRUE;
                strcpy (*warning, WARNING_COLON);
                state = STATE_COMMON;
                break;              
            }
        }
        
        if (ch != NULL_TERMINATOR)
        {
            ++str;
            ch = *str;
        }
    }
    
    if (wo_close_bracket)
    {
        strcpy (*warning, WARNING_NO_END);
        for (i = 0; i < line->nComponents; i++)
        {
            if (line->Components[i]->Type == COMPONENT_TYPE_STRING)
            {
                free (line->Components[i]->str);
            }
            free (line->Components[i]);
        }
            
        line->Type = LINE_TYPE_COMMON; 
        line->nComponents = 1;
        
        line->Components[0] = NULL;
        line->Components[0] = alloc_mem (line->Components[0], 2, sizeof (TComponent));
        line->Components[0]->Type = COMPONENT_TYPE_STRING;
        line->Components[0]->str = NULL;
        line->Components[0]->str = alloc_mem_for_string (line->Components[0]->str, strlen (str_) + 1);
        strcpy (line->Components[0]->str, str_);
        line->Components[0]->n = 1;
    }
    
    free (comp);
    free (str_);
}

int
get_res_component_from_purp_number (TLine *line, int k)
{
    int         i, j;
    
    j = 0;
    for (i = 0; i < line->nComponents; i++)
    {
        j += line->Components[i]->n;
        if (j >= k)
        {
            return i;
        }
    } 
    
    return line->nComponents - 1;
}

void
parameter_and_text_generator (TPurpose *purp, char **text, char *text_tpl, const char *finally_code, long *pa, const int purp_num_real, int k, int *purp_num)
{
    int         i, j, temp;
    char        *temp_str1 = NULL;
    char        *temp_str2 = NULL;
    char        *temp_text = NULL;
    char        *comment = NULL;

    if (k == purp->nLines)
    {
        for (i = 0; i < purp->nLines; i++)
        {
            if (purp->Lines[i]->Type == LINE_TYPE_RES)
            {
                pa[i] = get_res_component_from_purp_number (purp->Lines[i], *purp_num);
            }
        }
        
        temp_str1 = alloc_mem_for_string (temp_str1, MAX_STRING_LEGTH);
        temp_str2 = alloc_mem_for_string (temp_str2, MAX_STRING_LEGTH);
        comment = strdup (EMPTY_STRING);
        temp_text = strdup (text_tpl);

        temp_text = replace_all_substr_in_string(temp_text, FINALLY_TAG, finally_code);
        for (i = 0; i < purp->nLines; i++)
        {
            sprintf (temp_str1, PARAMNUM_TAG, i);
            if (pa[i] < MAX_LONGINT)
            {
                char* tstr = trim(purp->Lines[i]->Components[pa[i]]->str);
                temp_text = replace_all_substr_in_string(temp_text, temp_str1, tstr);
                comment = str_append (comment, COMMENT_POS);
                comment = str_append (comment, tstr);
                comment = str_append (comment, STRING_NL);
            }
            else
            {
                sprintf (temp_str2, "%d", pa[i] - MAX_LONGINT);
                temp_text = replace_all_substr_in_string(temp_text, temp_str1, temp_str2);
                comment = str_append (comment, COMMENT_POS);
                comment = str_append (comment, temp_str2);
                comment = str_append (comment, STRING_NL);
            }
        }

        sprintf (temp_str1, "%d", purp_num_real + *purp_num);
        temp_text = replace_all_substr_in_string(temp_text, PURPNUM_TAG, temp_str1);
        temp_text = replace_all_substr_in_string(temp_text, PARAMS_TAG, comment);
        if (*purp_num == 1)
        {
            strcpy (*text, EMPTY_STRING);
        }
        *text = str_append (*text, temp_text);
        (*purp_num)++;
    
        free (temp_text);
        free (comment);
        free (temp_str2);
        free (temp_str1);
        return;
    }
    
    if (purp->Lines[k]->Type == LINE_TYPE_RES)
    {
        parameter_and_text_generator (purp, text, text_tpl, finally_code, pa, purp_num_real, k + 1, purp_num);
    }
    else
    {
        for (i = 0; i < purp->Lines[k]->nComponents; i++)
        {
            if (purp->Lines[k]->Components[i]->Type == COMPONENT_TYPE_STRING)
            {
                temp = pa[k];
                pa[k] = i;
                parameter_and_text_generator (purp, text, text_tpl, finally_code, pa, purp_num_real, k + 1, purp_num);
                pa[k] = temp;
            }
            else
            {
                for (j = purp->Lines[k]->Components[i]->a; j <= purp->Lines[k]->Components[i]->b; j++)
                {
                    temp = pa[k];
                    pa[k] = j + MAX_LONGINT;
                    parameter_and_text_generator (purp, text, text_tpl, finally_code, pa, purp_num_real, k + 1, purp_num);
                    pa[k] = temp;
                }
            }
        }
    }
}
