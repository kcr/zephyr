/*
 * make_commands.c
 *
 * $Header: /srv/kcr/locker/zephyr/lib/ss/mk_cmds.c,v 1.3 1995-06-30 22:02:42 ghudson Exp $
 * $Locker:  $
 *
 * Copyright 1987, 1988 by MIT Student Information Processing Board
 *
 * For copyright information, see copyright.h.
 */

#include "copyright.h"
#include "ss_internal.h"

static const char copyright[] =
    "Copyright 1987 by MIT Student Information Processing Board";

extern char *last_token;
extern FILE *output_file;

extern FILE *yyin, *yyout;
extern int num_lines;

main(argc, argv)
    int argc;
    char **argv;
{
    char c_file[MAXPATHLEN];
    int result;
    char *path, *p, *q;

    if (argc != 2) {
	fputs("Usage: ", stderr);
	fputs(argv[0], stderr);
	fputs("cmdtbl.ct\n", stderr);
	exit(1);
    }

    path = malloc(strlen(argv[1])+4); /* extra space to add ".ct" */
    strcpy(path, argv[1]);
    p = strrchr(path, '/');
    if (p == (char *)NULL)
	p = path;
    else
	p++;
    p = strrchr(p, '.');
    if (p == (char *)NULL || strcmp(p, ".ct"))
	strcat(path, ".ct");
    yyin = fopen(path, "r");
    if (!yyin) {
	perror(path);
	exit(1);
    }

    p = strrchr(path, '.');
    *p = '\0';
    q = strrchr(path, '/');
    strcpy(c_file, (q) ? q + 1 : path);
    strcat(c_file, ".c");
    *p = '.';

    output_file = fopen(c_file, "w+");
    if (!output_file) {
	perror(c_file);
	exit(1);
    }

    fputs("/* ", output_file);
    fputs(c_file, output_file);
    fputs(" - automatically generated from ", output_file);
    fputs(path, output_file);
    fputs(" */\n", output_file);
    fputs("#include <ss/ss.h>\n\n", output_file);
    fputs("#ifndef __STDC__\n#define const\n#endif\n\n", output_file);
    /* parse it */
    result = yyparse();
    /* put file descriptors back where they belong */
    fclose(yyin);		/* bye bye input file */
    fclose(output_file);	/* bye bye output file */

    return result;
}

yyerror(s)
    char *s;
{
    fputs(s, stderr);
    fprintf(stderr, "\nLine %d; last token was '%s'\n",
	    num_lines, last_token);
}
