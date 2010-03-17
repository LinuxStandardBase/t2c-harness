#ifndef PARAM_H_
#define PARAM_H_

#ifndef TRUE
    #define TRUE                    1
#endif
#ifndef FALSE
    #define FALSE                   0
#endif

#define MAX_COMPONENTS              1000
#define MAX_LINES                   100
#define MAX_INT_LENGTH_IN_STR_FORM  5
#define MAX_LONGINT                 1000000
#define MAX_STRING_LEGTH            100000

#define EMPTY_STRING                ""
#define SET                         "SET"
#define RES                         "RES"
#define SPTAB                       " \t"
#define NUMBERS                     "0123456789"
#define PURPNUM_TAG                 "<%purpose_number%>"
#define PARAMS_TAG                  "<%params%>"
#define FINALLY_TAG                 "<%finally%>"
#define PARAMNUM_TAG                "<%%%d%%>"
#define COMMENT_NONE                "//    none\n"
#define COMMENT_POS                 "//    "
#define STRING_NL                   "\n"
#define	STRING_NULL					"NULL"
#define	STRING_APOSTR				"\""
#define	STRING_SEMICOLON			";"

#define NULL_TERMINATOR             '\0'
#define BRACKET_OPEN                '('
#define BRACKET_CLOSE               ')'
#define COLON                       ':'
#define SEMICOLON                   ';'
#define DOT                         '.'
#define QUOT	                    '"'
#define APOSTR                      '\''
#define BSLASH                      '\\'
#define	SHARP						'#'

#define WARNING_PREFIX              "WARNING: [purpose parameter]: "
#define WARNING_OK                  "No warnings"
#define WARNING_COLON               "Syntax error near ':' token"
#define WARNING_INTERVAL            "Invalid token in interval (number is expected)"
#define WARNING_MAX_INT             "Too big number (exceeds MAX_INT)"
#define WARNING_NO_END              "Close bracket is missing"

enum
{
    COMPONENT_TYPE_STRING = 0,
    COMPONENT_TYPE_INTERVAL
} ComponentTypes;

enum
{
    LINE_TYPE_COMMON = 0,
    LINE_TYPE_SET,
    LINE_TYPE_RES
} LineTypes;

enum 
{
    STATE_INIT = 0,
    STATE_COMMON,
    STATE_MB_SET,
    STATE_MB_RES,
    STATE_SET_RES,
    STATE_END,
    STATE_BSLASH,
    STATE_APOSTR,
    STATE_INTERVAL,
    STATE_COLON
} States;

typedef struct 
{
    int             Type;
    char            *str;
    int             a, b;
    int             n;
} TComponent;
    
typedef struct
{
    int             Type;
    int             nComponents;
    TComponent      *Components[MAX_COMPONENTS];
} TLine;

typedef struct
{
    int             nLines;
    TLine           *Lines[MAX_LINES];
} TPurpose;

#ifdef __cplusplus
extern "C"
{
#endif

extern void parse_purpose_line (const char *string, TLine *line, char **warning);
extern void parameter_and_text_generator (TPurpose *purp, char **text, char *text_tpl, const char *finally_code, long *pa, const int purp_num_real, int k, int *purp_num);

#ifdef  __cplusplus
}
#endif

#endif /*PARAM_H_*/
