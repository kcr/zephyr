#ifndef lexer_MODULE
#define lexer_MODULE

#include <ctype.h>

/*
 * is_identifier_char(c) - is c a character that could be part of
 *                         an identifier?
 *
 * NOTE: this information is hardwired into yylex() in lexer.c!
 */

#define  is_identifier_char(c)                    (isalnum(c) || (c)=='_')

/*
 * The maximum # of significant letters in an identifier:
 *
 * Note: in order for all keywords to be recognized, this must be at least 20.
 */

#define MAX_IDENTIFIER_LENGTH 128

/*
 * yylineno - this holds the current line # we are on.  Updated automatically
 *            by yylex.
 */

extern int yylineno;

/*
 * lex_open - this routine [re]initializes the lexer & prepares it to lex
 *            a file.  Resets current line # to 1.
 */

extern void lex_open(/* FILE *file */);

/*
 * yylex - performs as per. the yacc manual's requirements
 */

extern int yylex();

#endif
