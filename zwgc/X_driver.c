/****************************************************************************/
/*                                                                          */
/*                             The X driver:                                */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include "X_driver.h"
#include <X11/Xresource.h>
#include "new_memory.h"
#include "formatter.h"
#include "mux.h"
#include "variables.h"
#include "error.h"
#include "X_gram.h"
#include "xselect.h"

char *app_instance;

/*
 * dpy - the display we are outputting to
 */

Display *dpy = NULL;

/****************************************************************************/
/*                                                                          */
/*                  Code to deal with getting X resources:                  */
/*                                                                          */
/****************************************************************************/

/*
 *
 */

#ifndef  APPDEFDATABASE
#define  APPDEFDATABASE "/afs/athena.mit.edu/astaff/project/zephyr/src/zwgc/zwgc.dev/zwgc_resources"
#endif

/*
 *
 */

#ifndef  APPNAME
#define  APPNAME        "zwgc2"
#endif

/*
 *
 */

#ifndef  APPCLASS
#define  APPCLASS        "Zwgc"
#endif

/*
 * x_resources - our X resources from application resources, command line,
 *               and user's X resources.
 */

static XrmDatabase x_resources = NULL;

/*
 *  Internal Routine:
 *
 *    int convert_string_to_bool(string text)
 *         Effects: If text represents yes/true/on, return 1.  If text
 *                  representes no/false/off, return 0.  Otherwise,
 *                  returns -1.
 */

static int convert_string_to_bool(text)
     string text;
{
    if (!strcasecmp("yes", text) || !strcasecmp("y", text) ||
	!strcasecmp("true", text) || !strcasecmp("t", text) ||
	!strcasecmp("on", text))
      return(1);
    else if (!strcasecmp("no", text) || !strcasecmp("n", text) ||
	!strcasecmp("false", text) || !strcasecmp("f", text) ||
	!strcasecmp("off", text))
      return(0);
    else
      return(-1);
}

/*
 *
 */

char *get_string_resource(name, class)
     string name;
     string class;
{
    string full_name, full_class;
    int status;
    char *type;
    XrmValue value;

    full_name = string_Concat(APPNAME, ".");
    full_name = string_Concat2(full_name, name);
    full_class = string_Concat(APPCLASS, ".");
    full_class = string_Concat2(full_class, class);

    status = XrmGetResource(x_resources, full_name, full_class, &type, &value);
    free(full_name);
    free(full_class);

    if (status != True)
      return(NULL);

    if (string_Neq(type, "String"))
      return(NULL);

    return(value.addr);
}

/*
 *
 */

int get_bool_resource(name, class, default_value)
     string name;
     string class;
     int default_value;
{
    int result;
    char *temp;

    if (!(temp = get_string_resource(name, class)))
      return(default_value);

    result = convert_string_to_bool(temp);
    if (result == -1)
      result = default_value;

    return(result);
}

/*
 * Standard X Toolkit command line options:
 */

static XrmOptionDescRec cmd_options[] = {
    {"+rv",          "*reverseVideo", XrmoptionNoArg,  (caddr_t) "off"},
    {"+synchronous", "*synchronous",  XrmoptionNoArg,  (caddr_t) "off"},
    {"-background",  "*background",   XrmoptionSepArg, (caddr_t) NULL},
    {"-bd",          "*borderColor",  XrmoptionSepArg, (caddr_t) NULL},
    {"-bg",          "*background",   XrmoptionSepArg, (caddr_t) NULL},
    {"-bordercolor", "*borderColor",  XrmoptionSepArg, (caddr_t) NULL},
    {"-borderwidth", ".borderWidth",  XrmoptionSepArg, (caddr_t) NULL},
    {"-bw",          ".borderWidth",  XrmoptionSepArg, (caddr_t) NULL},
    {"-display",     ".display",      XrmoptionSepArg, (caddr_t) NULL},
    {"-fg",          "*foreground",   XrmoptionSepArg, (caddr_t) NULL},
    {"-fn",          "*font",         XrmoptionSepArg, (caddr_t) NULL},
    {"-font",        "*font",         XrmoptionSepArg, (caddr_t) NULL},
    {"-foreground",  "*foreground",   XrmoptionSepArg, (caddr_t) NULL},
    {"-geometry",    ".geometry",     XrmoptionSepArg, (caddr_t) NULL},
    {"-iconname",    ".iconName",     XrmoptionSepArg, (caddr_t) NULL},
    {"-name",        ".name",         XrmoptionSepArg, (caddr_t) NULL},
    {"-reverse",     "*reverseVideo", XrmoptionNoArg,  (caddr_t) "on"},
    {"-rv",          "*reverseVideo", XrmoptionNoArg,  (caddr_t) "on"},
    {"-selectionTimeout", 
       ".selectionTimeout", XrmoptionSepArg,   (caddr_t) NULL},
    {"-synchronous", "*synchronous",  XrmoptionNoArg,  (caddr_t) "on"},
    {"-title",       ".title",        XrmoptionSepArg, (caddr_t) NULL},
    {"-xrm",         NULL,            XrmoptionResArg, (caddr_t) NULL} };

#define NUMBER_OF_OPTIONS ((sizeof (cmd_options))/ sizeof(cmd_options[0]))

/*
 *
 */

int open_display_and_load_resources(pargc, argv)
     int *pargc;
     char **argv;
{
    XrmDatabase temp_db1, temp_db2;

    /* Initialize X resource manager: */
    XrmInitialize();

    /*
     * Parse X toolkit command line arguments (including -display)
     * into resources:
     */
    XrmParseCommand(&x_resources, cmd_options, NUMBER_OF_OPTIONS, APPNAME,
		    pargc, argv);

    /*
     * Try and open the display using the display specified if given.
     * If can't open the display, return an error code.
     */
    dpy = XOpenDisplay(get_string_resource("display", "display"));
    if (!dpy)
      return(1);

    /* Read in our application-specific resources: */
    temp_db1 = XrmGetFileDatabase(APPDEFDATABASE);

    /*
     * Get resources from the just opened display:
     */
    temp_db2 = XrmGetStringDatabase(dpy->xdefaults);

    /*
     * Merge the 3 sets of resources together such that when searching
     * for resources, they are checking in the following order:
     * command arguments, server resources, application resources
     */
    XrmMergeDatabases(temp_db2, &temp_db1);
    XrmMergeDatabases(x_resources, &temp_db1);
    x_resources = temp_db1;

    return(0);
}

/****************************************************************************/
/*                                                                          */
/*                Code to deal with initializing the driver:                */
/*                                                                          */
/****************************************************************************/

extern void x_get_input();

int X_driver_init(pargc, argv)
     int *pargc;
     char **argv;
{
    string temp;
    int sync;

    /*
     * Attempt to open display and read resources, including from the
     * command line.  If fail, exit with error code, disabling this
     * driver:
     */
    if (open_display_and_load_resources(pargc, argv)) {
	ERROR("Unable to open X display -- disabling X driver.\n");
	return(1);
    }

    /*
     * For now, set some useful variables using resources:
     */
    if (sync=get_bool_resource("synchronous", "Synchronous", 0))
      XSynchronize(dpy,sync);
    if (temp = get_string_resource("geometry", "Geometry"))
      var_set_variable("default_X_geometry", temp);

    temp=rindex(argv[0],'/');

    app_instance=string_Copy(temp?temp+1:argv[0]);

    xshowinit();
    x_gram_init(dpy);
    xselInitAtoms(dpy);
    
    mux_add_input_source(ConnectionNumber(dpy), x_get_input, dpy);

    return(0);
}

void X_driver_reset()
{
}

/****************************************************************************/
/*                                                                          */
/*                         The display routine itself:                      */
/*                                                                          */
/****************************************************************************/

char *X_driver(text)
     string text;
{
    string text_copy;
    desctype *desc;
    int numstr, numnl;
    
    text_copy = string_Copy(text);
    desc = disp_get_cmds(text_copy, &numstr, &numnl);
    
    xshow(dpy, desc, numstr, numnl);
    
    free(text_copy);
    free_desc(desc);
    return(NULL);
}
