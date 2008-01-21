/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Id$
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#include <sysdep.h>

#if (!defined(lint) && !defined(SABER))
static const char rcsid_formatter_c[] = "$Id$";
#endif

#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>

#include "new_memory.h"
#include "char_stack.h"
#include "string_dictionary.h"
#include "formatter.h"
#include "text_operations.h"

#if !defined(__STDC__) && !defined(const)
#define const
#endif

static int pure_text_length(char *, char);
static int env_length(char *);

#ifdef notdef
static character_class atsign_set = { /* '@' = 0x40 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };
#endif

static const character_class paren_set = { /* '(' = 0x28, ')' = 0x29 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static const character_class sbracket_set = { /* '[' = 0x5b, ']' = 0x5d */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static const character_class abracket_set = { /* '<' = 0x3c, '>' = 0x3e */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static const character_class cbracket_set = { /* '{' = 0x7b, '}' = 0x7d */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static const character_class allbracket_set = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static const character_class allmaskable_set = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,
   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };

static char brackets[]="()<>[]{}@";
static char *openbracket[]={"@<","@<","@[","@[","@{","@{","@(","@(","@("};
static char *closebracket[]={">",">","]","]","}","}",")",")",")"};

static int
not_contains(string str,
	     const character_class set)
{
   while (*str && ! set[*str]) str++;
   return (! *str);
}

static int
pure_text_length(char *text,
		 char terminator)
{
   int len=0;

   while (1) {
      while (*text!='@' && *text!=terminator && *text) {
         text++;
         len++;
      }

      if (*text!='@')
         return(len);

      if (*(text+1)=='@') {
	 text++;
	 len++;
      } else if (env_length(text+1) != -1)
        return(len);

      text++;
      len++;
   }
}

static char
otherside(char opener)
{
   switch (opener) {
    case '(':
      return(')');
    case '{':
      return('}');
    case '[':
      return(']');
    case '<':
      return('>');
   }

#ifdef DEBUG
   abort();
#endif
}

/* the char * that str points to is free'd by this function.
 * if you want to keep it, save it yourself
 */
string
verbatim(string str, int bracketsonly)
{
   char *temp,*temp2;
   int bracketnum,len;

   if (strlen(str) == pure_text_length(str,0)) {
      /* No environments, so consider the fast-and-easy methods */

      if (not_contains(str,allbracket_set)) {
	 temp = string_Copy(str);
	 free(str);
	 return(temp);
      }

      if (not_contains(str,abracket_set)) {
	 temp=(char *) malloc((len=strlen(str))+4);
	 temp[0]='@';
	 temp[1]='<';
	 (void) memcpy(temp+2,str,len);
	 temp[len+2]='>';
	 temp[len+3]='\0';
	 free(str);
	 return(temp);
      }
      if (not_contains(str,sbracket_set)) {
	 temp=(char *) malloc((len=strlen(str))+4);
	 temp[0]='@';
	 temp[1]='[';
	 (void) memcpy(temp+2,str,len);
	 temp[len+2]=']';
	 temp[len+3]='\0';
	 free(str);
	 return(temp);
      }
      if (not_contains(str,cbracket_set)) {
	 temp=(char *) malloc((len=strlen(str))+4);
	 temp[0]='@';
	 temp[1]='{';
	 (void) memcpy(temp+2,str,len);
	 temp[len+2]='}';
	 temp[len+3]='\0';
	 free(str);
	 return(temp);
      }
      if (not_contains(str,paren_set)) {
	 temp=(char *) malloc((len=strlen(str))+4);
	 temp[0]='@';
	 temp[1]='(';
	 (void) memcpy(temp+2,str,len);
	 temp[len+2]=')';
	 temp[len+3]='\0';
	 free(str);
	 return(temp);
      }
   }

   temp=lbreak(&str,bracketsonly?allbracket_set:allmaskable_set);
   while(*str) {
      bracketnum=(int) (strchr(brackets,str[0])-brackets);
      temp=string_Concat2(temp,openbracket[bracketnum]);
      temp=string_Concat2(temp,temp2=lany(&str," "));
      free(temp2);
      temp=string_Concat2(temp,closebracket[bracketnum]);
      temp=string_Concat2(temp,temp2=lbreak(&str,bracketsonly?
					  allbracket_set:allmaskable_set));
      free(temp2);
   }
   free(str);  /* str is "" at this point, anyway */

   return(temp);
}

/* text points to beginning of text string.  return value is
   length of string, up to but not including the passed terminator
   or the default terminator \0.  The text will not be modified,
   and @@ will be counted twice */

string
protect(string str)
{
   string temp,temp2,temp3;
   int len,templen;
   char_stack chs;
   char tos;

   temp = string_Copy("");
   templen = 1;
   chs = char_stack_create();

   while(*str) {
      tos = (char_stack_empty(chs)?0:char_stack_top(chs));

      if (*str == tos) {
	 /* if the character is the next terminator */

	 temp = (char *) realloc(temp,++templen);
	 temp[templen-2] = *str++;
	 char_stack_pop(chs);
	 temp[templen-1] = '\0';
      } else if (len = pure_text_length(str,tos)) {
	 if (tos) {
	    /* if the block is text in an environment, just copy it */

	    temp2 = string_CreateFromData(str,len);
	    str += len;
	    temp = string_Concat2(temp,temp2);
	    templen += len;
	    free(temp2);
	 } else {
	    /* if the block is top level text, verbatim brackets only
	       (not @'s) and add text to temp */

	    temp2 = string_CreateFromData(str,len);
	    str += len;
	    temp3 = verbatim(temp2,1);
	    temp = string_Concat2(temp,temp3);
	    templen += strlen(temp3);
	    free(temp3);
	 }
      } else {
	 /* if the block is an environment, copy it, push delimiter */

	 len = env_length(str+1);
	 char_stack_push(chs,otherside(str[len+1]));
	 len += 2;
     	 temp2 = string_CreateFromData(str,len);
	 str += len;
	 temp = string_Concat2(temp,temp2);
	 templen += len;
	 free(temp2);
      }
   }
   /* all blocks have been copied. */

   while (!char_stack_empty(chs)) {
      temp = (char *) realloc(temp,++templen);
      temp[templen-2] = char_stack_top(chs);
      char_stack_pop(chs);
   }
   temp[templen-1] = '\0';

   return(temp);
}

/* str points to a string.  return value is another string
   which is the original with all styles removed. */
string
stylestrip(string str)
{
    int templen = 0, otherchar;
    char *temp = (char *) malloc(string_Length(str) + 1);
    char_stack chs;
    string ostr = str;

    chs = char_stack_create();

    while (*str) {
	if (*str == '@') {
	    int len = env_length(str + 1);
	    if (len != -1) {
		otherchar = 0;
		if ((len == 4 && !strncasecmp(str + 1, "font", 4))
		  || (len == 5 && !strncasecmp(str + 1, "color", 5)))
		    otherchar = 0x80;
		otherchar |= otherside(str[len + 1]);
		char_stack_push(chs, otherchar);
		str += len + 2;
		continue;
	    }
	}
	if (!char_stack_empty(chs) && *str == (char_stack_top(chs) & 0x7f)) {
	    char_stack_pop(chs);
	    str++;
	    continue;
	}
	if (!char_stack_empty(chs) && (char_stack_top(chs) & 0x80))
	    str++;
	else
	    temp[templen++] = *str++;
    }
    temp[templen] = 0;

    while (!char_stack_empty(chs))
	char_stack_pop(chs);
    free(ostr);

    return(temp);
}

void
free_desc(desctype *desc)
{
    desctype *next_desc;

    while (desc->code != DT_EOF) {
	next_desc = desc->next;
	free(desc);
	desc = next_desc;
    }
    free(desc);
}

/* text points to beginning of possible env name.  return value is
   length of env name, not including @ or opener, or -1 if not a
   possible env name. */
static int
env_length(char *text)
{
   int len=0;

   while (*text && (isalnum(*text) || *text=='_')) {
      text++;
      len++;
   }

   if ((*text=='(') || (*text=='{') || (*text=='[') || (*text=='<'))
     return(len);
   else
     return(-1);
}

/* text points to beginning of text string.  return value is
   length of string, up to but not including the passed terminator
   or the default terminators \0 \n @.  This can modify text, and 0
   is a valid return value. */
static int
text_length(char *text,
	    char terminator)
{
   int len=0;

   while (1) {
      while (*text!='@' && *text!='\n' && *text!=terminator && *text) {
	 text++;
	 len++;
      }

      if (*text!='@')
	 return(len);

      if (*(text+1)=='@')
	 (void) memmove(text+1,text+2,strlen(text+1));
      else if (env_length(text+1) != -1)
	return(len);

      text++;
      len++;
   }
}

/* parses str into a desc linked list.  Returns number of strings and
   newlines in *pstr and *pnl */

desctype *
disp_get_cmds(char *str,
	      int *pstr,
	      int *pnl)
{
   desctype *desc,*here;
   int len;
   char_stack terminators = char_stack_create();
   char terminator;
   int nstr=0, nnl=0;
   char *curstr;

   desc=(desctype *) malloc(sizeof(desctype));
   here=desc;
   curstr=str;
   terminator = '\0';

   while (*curstr) {
      if (*curstr=='\n') {
	 here->code=DT_NL;
	 curstr++;
	 nnl++;
      } else if (*curstr==terminator) { /* if this is the end of an env */
	 here->code=DT_END;
	 terminator = char_stack_top(terminators);
	 char_stack_pop(terminators);
	 curstr++;
      } else if (len=text_length(curstr,terminator)) { /* if there is a text
							  block here */
	 here->code=DT_STR;
	 here->str=curstr;
	 here->len=len;
	 curstr+=len;
	 nstr++;
      } else if (*curstr=='@') { /* if this is the beginning of an env */
	 len=env_length(curstr+1);
	 here->code=DT_ENV;
	 here->str=curstr+1;
	 here->len=len;
	 char_stack_push(terminators, terminator);
	 terminator=otherside(*(curstr+1+len));
	 curstr+=(len+2); /* jump over @, env name, and opener */
      }

      here->next=(desctype *) malloc(sizeof(desctype));
      here=here->next;
   }

   while (!char_stack_empty(terminators)) {
      here->code=DT_END;
      terminator = char_stack_top(terminators);
      char_stack_pop(terminators);
      here->next=(desctype *) malloc(sizeof(desctype));
      here=here->next;
   }
   here->code=DT_EOF;
   *pstr=nstr;
   *pnl=nnl;

#ifdef DEBUG_PRINTOUT
   { string temp;
       here = desc;
       while (here->code != DT_EOF) {
	   if (here->code == DT_STR || here->code == DT_ENV) {
	       temp = string_CreateFromData(here->str, here->len);
	       printf("[%d <%s>]\n", here->code, temp);
	       free(temp);
	   } else
	     printf("[%d]\n", here->code);
	   here=here->next;
       }
 }
#endif

   return(desc);
}
