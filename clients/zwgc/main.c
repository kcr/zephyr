#include <stdio.h>
#include <signal.h>
#include <zephyr/zephyr.h>
#include "new_memory.h"
#include "zwgc.h"
#include "parser.h"
#include "node.h"
#include "exec.h"
#include "zephyr.h"
#include "notice.h"
#include "subscriptions.h"
#include "file.h"
#include "mux.h"
#include "port.h"
#include "variables.h"

extern void notice_handler();
extern void setup_signals();

/*
 * Global zwgc-wide variables:
 */

#ifdef DEBUG
int zwgc_debug = 0;
#endif

/*
 * description_filename_override - <<<>>>
 */

static char *description_filename_override = NULL;

/*
 * subscriptions_filename_override - <<<>>> export!
 */

char *subscriptions_filename_override = NULL;

/****************************************************************************/
/*                                                                          */
/*             Code to deal with reading in the description file:           */
/*                                                                          */
/****************************************************************************/

/*
 * program - this holds a pointer to the node representation of the
 *           description file once it has been read in.
 */

static struct _Node *program = NULL;

/*
 * <<<>>>
 */

static void fake_startup_packet()
{
    ZNotice_t notice;

    var_set_variable("version", "0.3.5");

    bzero(&notice, sizeof(notice));

    notice.z_version = "";
    notice.z_class = "WG_CTL_CLASS";
    notice.z_class_inst = "WG_CTL_USER<<<>>>";
    notice.z_opcode = "WG_STARTUP";
    notice.z_default_format = "Zwgc mark II version $version now running...\n";
    notice.z_recipient = "";
    notice.z_sender = "ZWGC";
    notice.z_port = 0;
    notice.z_kind = ACKED;
    notice.z_auth = ZAUTH_YES;
    notice.z_message = "Zwgc mark II version 0.3.5 now running...";
    notice.z_message_len = strlen(notice.z_message)+1;
    
    notice_handler(&notice);
}

static void read_in_description_file()
{
    FILE *input_file;

/*    var_clear_all_variables(); <<<>>> */

    input_file = locate_file(description_filename_override, ".zwgc.desc",
 "/afs/athena.mit.edu/astaff/project/zephyr/src/zwgc/zwgc.dev/misc/zwgc.desc");
    if (input_file)
      program = parse_file(input_file);
    else
      program = NULL;
    
    fake_startup_packet();
}

/****************************************************************************/
/*                                                                          */
/*            Code to deal with argument parsing & overall control:         */
/*                                                                          */
/****************************************************************************/

/*
 *    void usage()
 *        Effects: Prints out an usage message on stderr then exits the
 *                 program with error code 1.
 */

void usage()
{
#ifdef DEBUG
    fprintf(stderr, "\
zwgc: usage: zwgc [-debug] [-f <filename>] [-subfile <filename>]\n\
                  [-default <driver>] {-disable <driver>}*\n\
                  [output driver options]\n");
#else
    fprintf(stderr, "\
zwgc: usage: zwgc [-f <filename>] [-subfile <filename>]\n\
                  [-default <driver>] {-disable <driver>}*\n\
                  [output driver options]\n");
#endif
    exit(1);
}

/*
 * <<<>>>
 */

static void run_initprogs()
{
    /*
     * This code stolen from old zwgc: yuck.  Clean up & fix.  <<<>>>
     * Should this fork instead of just systeming?
     */

    int status;
    char *progname = ZGetVariable("initprogs");
    
    if (!progname)
      return;
    
    status = system(progname);
    if (status == 127) {
	perror("zwgc initprog exec");
	fprintf(stderr,"zwgc initprog of <%s> failed: no shell.\n",
		progname);
    } else if (status!=-1 && status>>8) {
	perror("zwgc initprog exec");
	fprintf(stderr,"zwgc initprog of <%s> failed with status [%d].\n",
		progname, status>>8);
    }
}

/*
 * main -- the program entry point.  Does parsing & top level control.
 */

int main(argc, argv)
     int argc;
     char **argv;
{
    char **new, **current;

    /*
     * Process "-f <filename>", "-subfile <filename>", and (if DEBUG) "-debug"
     * arguments, removing then from argc, agrv:
     */
    for (new=current=argv+1; *current; current++) {
	if (string_Eq(*current, "-debug")) {
	    argc--;
#ifdef DEBUG
	    zwgc_debug = 1;
#endif
	} else if (string_Eq(*current, "-f")) {
	    argc -= 2; current++;
	    if (!*current)
	      usage();
	    description_filename_override = *current;
	} else if (string_Eq(*current, "-subfile")) {
	    argc -= 2; current++;
	    if (!*current)
	      usage();
	    subscriptions_filename_override = *current;
	} else
	  *(new)++ = *current;
    }
    *new = *current;

    /*
     * Initialize various subsystems in proper order:
     */
    dprintf("Initializing subsystems...\n"); /*<<<>>>*/
    mux_init();
    var_clear_all_variables(); /* <<<>>> */
    init_ports();       /* <<<>>> */
    dprintf("Initializing standard ports...\n");
    init_standard_ports(&argc, argv);
    if (argc>1)
      usage();
    dprintf("Initializing zephyr...\n");
    setup_signals();
    zephyr_init(notice_handler);

    /*
     * Run the initprogs program(s) now that we are all set to deal:
     */
    dprintf("Running initprogs program...\n");
    run_initprogs();

    dprintf("Test Zwgc parser.\n\n");
    read_in_description_file();

    dprintf("Entering main loop\n");
    mux_loop();

    dprintf("Returning from main loop\n");
    finalize_zephyr();

    return(0);
}

/****************************************************************************/
/*                                                                          */
/*               :               */
/*                                                                          */
/****************************************************************************/

#define  USER_SUPPRESS     "SUPPRESS"
#define  USER_UNSUPPRESS   "UNSUPPRESS"

void notice_handler(notice)
     ZNotice_t *notice;
{
    char *control_opcode;

    dprintf("Got a message\n");

    if (control_opcode = decode_notice(notice)) {
#ifdef DEBUG
	printf("got control opcode <%s>.\n", control_opcode);
#endif
	if (!strcasecmp(control_opcode, USER_REREAD)) {
	    printf("zwgc: rereading descfile...\n");
	    read_in_description_file();
	} else if (!strcasecmp(control_opcode, USER_SHUTDOWN))
	  zwgc_shutdown();
	else if (!strcasecmp(control_opcode, USER_STARTUP)) {
#ifdef DEBUG_MEMORY
	    report_memory_usage(); /* <<<>>> */
#endif
	    zwgc_startup();
	} else if (!strcasecmp(control_opcode, USER_SUPPRESS)) {
	    string class = get_field(notice->z_message,
				     notice->z_message_len, 1);
	    string instance = get_field(notice->z_message,
					notice->z_message_len, 2);
	    string recipient = get_field(notice->z_message,
					 notice->z_message_len, 3);
	    punt(class, instance, recipient);
	    free(class);
	    free(instance);
	    free(recipient);
	} else if (!strcasecmp(control_opcode, USER_UNSUPPRESS)) {
	    string class = get_field(notice->z_message,
				     notice->z_message_len, 1);
	    string instance = get_field(notice->z_message,
					notice->z_message_len, 2);
	    string recipient = get_field(notice->z_message,
					 notice->z_message_len, 3);
	    unpunt(class, instance, recipient);
	    free(class);
	    free(instance);
	    free(recipient);
	} else
	  printf("zwgc: unknown control opcode %s.\n", control_opcode);

	return;
    }

    if (!zwgc_active) {
#ifdef DEBUG
	if (zwgc_debug)
	  printf("NON-ACTIVE: PUNTED <%s>!!!!\n", notice->z_class_inst);
#endif
	return;
    }
    
    if (puntable_address_p(notice->z_class,
			   notice->z_class_inst,
			   notice->z_recipient)) {
#ifdef DEBUG
	if (zwgc_debug)
	  printf("PUNTED <%s>!!!!\n", notice->z_class_inst);
#endif
	return;
    }

    exec_process_packet(program, notice);
}

/***************************************************************************/

/*
 *
 */

static void signal_exit()
{
    mux_end_loop_p = 1;
}

#include <sys/wait.h>
static signal_child()
{
  union wait status;

  (void)wait(&status);
}

static void setup_signals()
{
    signal(SIGTERM, signal_exit);
    signal(SIGHUP, signal_exit);
    signal(SIGINT, signal_exit);
    signal(SIGCHLD, signal_child);
}
