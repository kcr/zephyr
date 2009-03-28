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
static const char rcsid_X_gram_c[] = "$Id$";
#endif

#include <zephyr/mit-copyright.h>

#ifndef X_DISPLAY_MISSING

#include <zephyr/zephyr.h>
#include "X_gram.h"
#include "xmark.h"
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include "zwgc.h"
#include "X_driver.h"
#include "X_fonts.h"
#include "error.h"
#include "new_string.h"
#include "xrevstack.h"
#include "xerror.h"
#include "xselect.h"
#ifdef CMU_ZWGCPLUS
#include "plus.h"
#endif

extern XContext desc_context;
extern char *app_instance;

/*
 *
 */

int internal_border_width = 2;

unsigned long default_fgcolor;
unsigned long default_bgcolor;
unsigned long default_bordercolor;
long ttl = 0;
static int reset_saver;
static int border_width = 1;
static int cursor_code = XC_sailboat;
static int set_transient;
static int enable_delete;
static char *title_name,*icon_name;
static Cursor cursor;
static Window group_leader; /* In order to have transient windows,
			     * I need a top-level window to always exist
			     */
static XClassHint classhint;
static XSetWindowAttributes xattributes;
static unsigned long xattributes_mask;
static int set_all_desktops = True;
static Atom net_wm_desktop = None;
static Atom net_wm_window_type = None;
static Atom net_wm_window_type_utility = None;

/* ICCCM note:
 *
 * the following properties must be set on all top-level windows:
 *
 * WM_NAME                  XStoreName(dpy,w,name);
 * WM_ICON_NAME             XSetIconName(dpy,w,name);
 * WM_NORMAL_HINTS          XSetNormalHints(dpy,w,sizehints);
 * WM_HINTS                 XSetWMHints(dpy,w,wmhints);
 * WM_CLASS                 XSetClassHint(dpy,w,classhint);
 *
 * and for individual zgrams:
 *
 * WM_TRANSIENT_FOR         XSetTransientForHint(dpy,w,main_window);
 * WM_PROTOCOLS		    XSetWMProtocols(dpy,w,protocols,cnt);
 */

/* set all properties defined in ICCCM.  If main_window == 0,
 * per-zgram initialization is not done.
 */

/*ARGSUSED*/
void
x_set_icccm_hints(Display *dpy,
		  Window w,
		  char *name,
		  char *icon_name,
		  XSizeHints *psizehints,
		  XWMHints *pwmhints,
		  Window main_window)
{
   XStoreName(dpy,w,name);
   XSetIconName(dpy,w,icon_name);
   XSetWMNormalHints(dpy,w,psizehints);
   XSetWMHints(dpy,w,pwmhints);
   XSetClassHint(dpy,w,&classhint);
   /* in order for some wm's to iconify, the window shouldn't be transient.
      e.g. Motif wm */
   if (main_window != None) {
      if (set_transient)
	  XSetTransientForHint(dpy,w,main_window);
   }
   if (enable_delete)
      XSetWMProtocols(dpy,w,&XA_WM_DELETE_WINDOW,1);
}

void
x_gram_init(Display *dpy)
{
    char *temp;
    XSizeHints sizehints;
    XWMHints wmhints;
    unsigned long rv,tc;

    default_fgcolor = BlackPixelOfScreen(DefaultScreenOfDisplay(dpy));
    default_bgcolor = WhitePixelOfScreen(DefaultScreenOfDisplay(dpy));
    rv = get_bool_resource("reverseVideo", "ReverseVideo", 0);
    if (rv) {
       tc = default_fgcolor;
       default_fgcolor = default_bgcolor;
       default_bgcolor = tc;
    }
    temp = get_string_resource("foreground", "Foreground");
    if (temp)
      default_fgcolor = x_string_to_color(temp, default_fgcolor);
    temp = get_string_resource("background", "Background");
    if (temp)
      default_bgcolor = x_string_to_color(temp, default_bgcolor);
    default_bordercolor = default_fgcolor;
    temp = get_string_resource("borderColor", "BorderColor");
    if (temp)
      default_bordercolor = x_string_to_color(temp, default_bordercolor);

    temp = get_string_resource("minTimeToLive", "MinTimeToLive");
    if (temp && atoi(temp)>=0)
       ttl = atoi(temp);

#ifdef CMU_ZWGCPLUS
    if (ttl == 0) {
      temp = get_string_resource("lifespan", "LifeSpan");
      if (temp && atoi(temp)>=0)
        ttl = atoi(temp);
    }

    get_full_names = get_bool_resource("getFullNames", "GetFullNames", 0);
#endif

    reverse_stack = get_bool_resource("reverseStack", "ReverseStack", 0);
    reset_saver =  get_bool_resource("resetSaver", "ResetSaver", 1);
    /* The default here should be 1, but mwm sucks */
    set_transient = get_bool_resource("transient", "Transient", 0);
    enable_delete = get_bool_resource("enableDelete", "EnableDelete", 1);

    temp = get_string_resource("borderWidth", "BorderWidth");
    /* <<<>>> */
    if (temp && atoi(temp)>=0)
      border_width = atoi(temp);

    temp = get_string_resource("internalBorder", "InternalBorder");
    /* <<<>>> */
    if (temp && atoi(temp)>=0)
      internal_border_width = atoi(temp);

    temp = get_string_resource("cursorCode", "CursorCode");
    /* <<<>>> */
    if (temp && atoi(temp))
      cursor_code = atoi(temp);

    cursor = XCreateFontCursor(dpy, cursor_code);
    if (!cursor)
      cursor = XCreateFontCursor(dpy, XC_sailboat);

    temp = get_string_resource("pointerColor", "Foreground");
    if (temp) {
	char *temp2;
	XColor cursor_fore, cursor_back;
	/* XXX need to do our own parsing here, since the RecolorCursor
	   routine requires an XColor, not an unsigned long (pixel) */
	if (!(temp2 = get_string_resource("background","Background"))) {
	    if (default_bgcolor == WhitePixelOfScreen(DefaultScreenOfDisplay(dpy)))
		temp2 = "white";
	    else
		temp2 = "black";
	}
	if (XParseColor(dpy,
			DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy)),
			temp, &cursor_fore) &&
	    XParseColor(dpy,
			DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy)),
			temp2, &cursor_back)) {
	      XRecolorCursor(dpy, cursor, &cursor_fore, &cursor_back);
	  }
    }
    if (!(title_name=get_string_resource("title","Title")))
      if (!(title_name=get_string_resource("name","Name")))
	title_name=app_instance;

    if (!(icon_name=get_string_resource("iconName","IconName")))
      if (!(icon_name=get_string_resource("name","Name")))
	icon_name=app_instance;

    if (!(temp=get_string_resource("name","Name")))
      if (!(temp=(char *) getenv("RESOURCE_NAME")))
	temp=app_instance;
    classhint.res_name=string_Copy(temp);
    classhint.res_class="Zwgc";

    if (set_transient) {
       group_leader=XCreateSimpleWindow(dpy,DefaultRootWindow(dpy),0,0,100,100,
					0,default_bordercolor,default_bgcolor);
       sizehints.x = 0;
       sizehints.y = 0;
       sizehints.width = 100;
       sizehints.height = 100;
       sizehints.flags = PPosition | PSize;

       wmhints.input = False;
       wmhints.initial_state = DontCareState;
       wmhints.flags = InputHint | StateHint;

       x_set_icccm_hints(dpy,group_leader,"ZwgcGroup","ZwgcGroup",&sizehints,
			 &wmhints,0);
    }
    xattributes.border_pixel = default_bordercolor;
    xattributes.cursor = cursor;
    xattributes.event_mask = (ExposureMask|ButtonReleaseMask|ButtonPressMask
			      |LeaveWindowMask|Button1MotionMask
#ifdef CMU_ZWGCPLUS
			      |KeyPressMask
#endif
			      |Button3MotionMask|StructureNotifyMask);
    xattributes_mask = (CWBackPixel|CWBorderPixel|CWEventMask|CWCursor);

    set_all_desktops = get_bool_resource("allDesktops", "AllDesktops", True);
    net_wm_desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
    net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    net_wm_window_type_utility = XInternAtom(dpy,
					     "_NET_WM_WINDOW_TYPE_UTILITY",
					     False);

    temp = get_string_resource ("backingStore", "BackingStore");
    if (!temp)
	return;
    xattributes_mask |= CWBackingStore;
    if (!strcasecmp (temp, "notuseful"))
	xattributes.backing_store = NotUseful;
    else if (!strcasecmp (temp, "whenmapped"))
	xattributes.backing_store = WhenMapped;
    else if (!strcasecmp (temp, "always"))
	xattributes.backing_store = Always;
    else if (!strcasecmp (temp, "default"))
	xattributes_mask &= ~CWBackingStore;
    else {
	switch (get_bool_resource ("backingStore", "BackingStore", -1)) {
	case 0:
	    xattributes.backing_store = NotUseful;
	    break;
	case 1:
	    xattributes.backing_store = WhenMapped;
	    break;
	case -1:
	    fprintf (stderr,
		 "zwgc: Cannot interpret backing-store resource value `%s'.\n",
		     temp);
	    xattributes_mask &= ~CWBackingStore;
	    break;
	}
    }
}

int
x_calc_gravity(int xalign,
	       int yalign)
{
    if (yalign > 0) {					/* North */
	return (xalign > 0)  ? NorthWestGravity
	     : (xalign == 0) ? NorthGravity
	     :                 NorthEastGravity;
    } else if (yalign == 0) {				/* Center */
	return (xalign > 0)  ? WestGravity
	     : (xalign == 0) ? CenterGravity
	     :                 EastGravity;
    } else {						/* South */
	return (xalign > 0)  ? SouthWestGravity
	     : (xalign == 0) ? SouthGravity
	     :                 SouthEastGravity;
    }
}

void
x_gram_create(Display *dpy,
	      x_gram *gram,
	      int xalign,
	      int yalign,
	      int xpos,
	      int ypos,
	      int xsize,
	      int ysize,
	      int beepcount)
{
    Window w;
    XSizeHints sizehints;
    XWMHints wmhints;
    XSetWindowAttributes attributes;
    unsigned long all_desktops = 0xFFFFFFFF;

    /*
     * Adjust xpos, ypos based on the alignments xalign, yalign and the sizes:
     */
    if (xalign<0)
      xpos = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - xpos - xsize
	- 2*border_width;
    else if (xalign == 0)
      xpos = ((WidthOfScreen(DefaultScreenOfDisplay(dpy)) - xsize
	       - 2*border_width)>>1) + xpos;

    if (yalign<0)
      ypos = HeightOfScreen(DefaultScreenOfDisplay(dpy)) - ypos - ysize
	- 2*border_width;
    else if (yalign == 0)
      ypos = ((HeightOfScreen(DefaultScreenOfDisplay(dpy)) - ysize
	       - 2*border_width)>>1) + ypos;

    /*
     * Create the window:
     */
    attributes = xattributes;
    attributes.background_pixel = gram->bgcolor;
    
    gram->w = w = XCreateWindow (dpy, DefaultRootWindow (dpy), xpos, ypos,
				 xsize, ysize, border_width, 0,
				 CopyFromParent, CopyFromParent,
				 xattributes_mask, &attributes);
    
    sizehints.x = xpos;
    sizehints.y = ypos;
    sizehints.width = xsize;
    sizehints.height = ysize;
    sizehints.win_gravity = x_calc_gravity(xalign, yalign);
    sizehints.flags = USPosition|USSize|PWinGravity;

    wmhints.input = False;
    wmhints.initial_state = NormalState;
    if (set_transient) {
       wmhints.window_group = group_leader;
       wmhints.flags = InputHint | StateHint | WindowGroupHint;

       x_set_icccm_hints(dpy,w,title_name,icon_name,&sizehints,&wmhints,
			 group_leader);
    } else {
       wmhints.flags = InputHint | StateHint;

       x_set_icccm_hints(dpy,w,title_name,icon_name,&sizehints,&wmhints,0);
    }
       
    if (net_wm_window_type != None && net_wm_window_type_utility != None)
	XChangeProperty(dpy, w, net_wm_window_type, XA_ATOM, 32,
			PropModeReplace,
			(unsigned char *) &net_wm_window_type_utility, 1);
    if (set_all_desktops && net_wm_desktop != None)
	XChangeProperty(dpy, w, net_wm_desktop, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *) &all_desktops, 1);

    XSaveContext(dpy, w, desc_context, (caddr_t)gram);

    gram->can_die.tv_sec = 0;

    XMapWindow(dpy, w);

    if (beepcount)
	XBell(dpy, 0);

    xerror_happened = 0;
    if (reverse_stack && bottom_gram) {
       XWindowChanges winchanges;
       
       winchanges.sibling=bottom_gram->w;
       winchanges.stack_mode=Below;
       /* Metacity may use border_width even if it's not specified in
	* the value mask, so we must initialize it.  See:
	* http://bugzilla.gnome.org/show_bug.cgi?id=305257 */
       winchanges.border_width=border_width;

       begin_xerror_trap (dpy);
       XReconfigureWMWindow (dpy, w, DefaultScreen (dpy),
			     CWSibling|CWStackMode, &winchanges);
       end_xerror_trap (dpy);
       if (xerror_happened) {
	   /* The event didn't go.  Print an error message, and continue.  */
	   ERROR ("Error configuring window to the bottom of the stack.\n");
       }
    }
    /* we always need to keep a linked list of windows */
    add_to_bottom(gram);
    if (xerror_happened)
       pull_to_top(gram);

    if (reset_saver)
	XResetScreenSaver(dpy);

    XFlush(dpy);
    /* Because the flushing/syncing/etc with the error trapping can cause
       events to be read into the Xlib queue, we need to go through the queue
       here before exiting so that any pending events get processed.
       */
    x_get_input(dpy);
}

void
x_gram_draw(Display *dpy,
	    Window w,
	    x_gram *gram,
	    Region region)
{
   int i;
   GC gc;
   XGCValues gcvals;
   xblock *xb;
   XTextItem text;
   int startblock, endblock, startpixel = 0, endpixel = 0;
   
#define SetFG(fg) \
   gcvals.foreground=fg; \
   XChangeGC(dpy,gc,GCForeground,&gcvals)

   gc = XCreateGC(dpy, w, 0, &gcvals);
   XSetRegion(dpy,gc,region);
 
   if ((markgram == gram) && (STARTBLOCK != -1) && (ENDBLOCK != -1)) {
      if (xmarkSecond() == XMARK_END_BOUND) {
	 startblock=STARTBLOCK;
	 endblock=ENDBLOCK;
	 startpixel=STARTPIXEL;
	 endpixel=ENDPIXEL;
      } else {
	 startblock=ENDBLOCK;
	 endblock=STARTBLOCK;
	 startpixel=ENDPIXEL;
	 endpixel=STARTPIXEL;
      }
   } else {
      startblock = -1;
      endblock = -1;
   }

   for (i=0,xb=gram->blocks ; i<gram->numblocks ; i++,xb++) {
      if (XRectInRegion(region,xb->x1,xb->y1,xb->x2-xb->x1,
			xb->y2-xb->y1) != RectangleOut) {
	 if (i==startblock) {
	    if (i==endblock) {
	       SetFG(gram->bgcolor);
	       XFillRectangle(dpy,w,gc,xb->x1,xb->y1,startpixel,
			      (xb->y2-xb->y1));
	       SetFG(xb->fgcolor);
	       XFillRectangle(dpy,w,gc,xb->x1+startpixel,xb->y1,
			      (endpixel-startpixel),(xb->y2-xb->y1));
	       SetFG(gram->bgcolor);
	       XFillRectangle(dpy,w,gc,xb->x1+endpixel,xb->y1,
			      (xb->x2-xb->x1-endpixel),(xb->y2-xb->y1));
	    } else {
	       SetFG(gram->bgcolor);
	       XFillRectangle(dpy,w,gc,xb->x1,xb->y1,startpixel,
			      (xb->y2-xb->y1));
	       SetFG(xb->fgcolor);
	       XFillRectangle(dpy,w,gc,xb->x1+startpixel,xb->y1,
			      (xb->x2-xb->x1-startpixel),(xb->y2-xb->y1));
	    }
	 } else if (i==endblock) {
	    SetFG(xb->fgcolor);
	    XFillRectangle(dpy,w,gc,xb->x1,xb->y1,endpixel,
			   (xb->y2-xb->y1));
	    SetFG(gram->bgcolor);
	    XFillRectangle(dpy,w,gc,xb->x1+endpixel,xb->y1,
			   (xb->x2-xb->x1-endpixel),(xb->y2-xb->y1));
	 } else {
	    if ((startblock < i) && (i < endblock)) {
	       SetFG(xb->fgcolor);
	    } else {
	       SetFG(gram->bgcolor);
	    }
	    XFillRectangle(dpy,w,gc,xb->x1,xb->y1,(xb->x2-xb->x1),
			   (xb->y2-xb->y1));
	 }
      }
   }

   gcvals.function=GXxor;
   XChangeGC(dpy,gc,GCFunction,&gcvals);

   for (i=0,xb=gram->blocks ; i<gram->numblocks ; i++,xb++) {
      if (XRectInRegion(region,xb->x1,xb->y1,xb->x2-xb->x1,
			xb->y2-xb->y1) != RectangleOut) {
	 SetFG(gram->bgcolor^xb->fgcolor);
	 text.chars=gram->text+xb->strindex;
	 text.nchars=xb->strlen;
	 text.delta=0;
	 text.font=xb->fid;
	 XDrawText(dpy,w,gc,xb->x,xb->y,&text,1);
     }
   }

   XFreeGC(dpy,gc);
}

void
x_gram_expose(Display *dpy,
	      Window w,
	      x_gram *gram,
	      XExposeEvent *event)
{
   static Region region;
   static int partregion;
   XRectangle rect;

   rect.x = (short) event->x;
   rect.y = (short) event->y;
   rect.width = (unsigned short) event->width;
   rect.height = (unsigned short) event->height;

#ifdef MARK_DEBUG
   printf("----- xeventExpose:\nx=%d y=%d w=%d h=%d\n-----",
	  event->x,event->y,event->width,event->height);
#endif

   if (! partregion) {
      region=XCreateRegion();
      partregion = 1;
   }

   if (rect.width && rect.height) XUnionRectWithRegion(&rect,region,region);

   if (event->count == 0) {
      x_gram_draw(dpy,w,gram,region);
      partregion = 0;
      XDestroyRegion(region);
   }
}

#endif /* X_DISPLAY_MISSING */

