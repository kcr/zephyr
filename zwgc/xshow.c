#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "pointer_dictionary.h"
#include "new_memory.h"
#include "formatter.h"
#include "variables.h"
#include "zwgc.h"
#include "X_fonts.h"
#include "X_gram.h"
#include "xmode_stack.h"

#define max(a,b)   ((a)>(b)?(a):(b))

XContext desc_context;
static pointer_dictionary geometry_dict;

extern int internal_border_width;

xshowinit()
{
    desc_context = XUniqueContext();
}

static char *xres_get_geometry(style)
     char *style;
{
   char *desc;
   pointer_dictionary_binding *binding;
   int exists;
   char *family;

   desc=string_Concat("style.",style);
   desc=string_Concat2(desc,".geometry");

   if (!geometry_dict)
      geometry_dict = pointer_dictionary_Create(37);
   binding = pointer_dictionary_Define(geometry_dict,desc,&exists);

   if (exists) {
      free(desc);
      return((string) binding->value);
   } else {
#define STYLE_CLASS "Style.Style1.Style2.Style3.Geometry"
      family=get_string_resource(desc,STYLE_CLASS);
#undef STYLE_CLASS
      free(desc);
      if (family==NULL)
	 pointer_dictionary_Delete(geometry_dict,binding);
      else
	 binding->value=(pointer) family;
      return(family);  /* If resource returns NULL, return NULL also */
   }
}

void fixup_and_draw(dpy, style, auxblocks, blocks, num, lines, numlines)
     Display *dpy;
     char *style;
     xblock *blocks;
     xauxblock *auxblocks;
     int num;
     xlinedesc *lines;
     int numlines;
{
    int gram_xalign = 1;
    int gram_yalign = 1;
    int gram_xpos, gram_ypos, gram_xsize, gram_ysize;

    x_gram *gram;
    int strindex = 0;

    int line, block=0;
    int maxwidth=0, chars=0, maxascent, maxdescent;
    int ssize,  lsize,csize, rsize, width;
    int i, ascent, descent;

    int yofs = internal_border_width;
    int lofs, cofs, rofs;
    int ystart,yend;

    char *geometry,xpos[10], ypos[10], xfrom, yfrom;

    gram = (x_gram *)malloc(sizeof(x_gram));

    /* Find total lengths of left, center, and right parts.  Also find the
       length of the longest line and the total number of characters. */

    for (line=0; line<numlines; line++) {
	lsize = csize = rsize = 0;
	maxascent = maxdescent = 0;
	
	/* add up sizes for each block, get max ascent and descent */
	
	for (i=0; i<lines[line].numblock; i++,block++) {
	    chars += auxblocks[block].len;
	    ssize = XTextWidth(auxblocks[block].font, auxblocks[block].str,
			       auxblocks[block].len);
	    auxblocks[block].width = ssize;
	    ascent = auxblocks[block].font->ascent;
	    descent = auxblocks[block].font->descent;
	    if (ascent>maxascent)
	      maxascent = ascent;
	    if (descent>maxdescent)
	      maxdescent = descent;
	    switch (auxblocks[block].align) {
	      case LEFTALIGN:
		lsize += ssize;
		break;
		
	      case CENTERALIGN:
		csize += ssize;
		break;
		
	      case RIGHTALIGN:
		rsize += ssize;
		break;
	    }
	}
	
	/* save what we need to do size fixups */
	
	if (maxascent>lines[line].ascent)
	  lines[line].ascent = maxascent;
	if (maxdescent>lines[line].descent)
	  lines[line].descent = maxdescent;
	lines[line].lsize = lsize;
	lines[line].csize = csize;
	lines[line].rsize = rsize;
	
	/* get width of line and see if it is bigger than the max width */
	
	switch ((lsize?1:0)+(csize?2:0)+(rsize?4:0)) {
#ifdef DEBUG
	  default:
	    abort();
#endif
	    
	  case 0:
	    width = 0;
	    break;
	    
	  case 1:
	    width = lsize;
	    break;
	    
	  case 2:
	    width = csize;
	    break;
	    
	  case 3:
	    width = lsize*2+csize+XTextWidth(auxblocks[i].font," ",1);
	    break;
	    
	  case 4:
	    width = rsize;
	    break;
	    
	  case 5:
	    width = lsize+rsize+XTextWidth(auxblocks[i].font, " ", 1);
	    break;
	    
	  case 6:
	    width = csize+rsize*2+XTextWidth(auxblocks[i].font, " ", 1);
	    break;
	    
	  case 7:
	    width = max(lsize,rsize)*2+csize+
	      XTextWidth(auxblocks[i].font," ",1)*2;
	    break;
	}
	if (width>maxwidth)
	  maxwidth = width;
    }

    /* fixup x,y for each block, create big string and indices into it */
    /* set x1,y1,x2,y2 of each block also. */

    gram->text = (char *)malloc(chars);
    block = 0;

    for (line=0; line<numlines; line++) {
	lofs = internal_border_width;
	cofs = ((maxwidth-lines[line].csize)>>1) + internal_border_width;
	rofs = maxwidth-lines[line].rsize + internal_border_width;
	ystart = yofs;
	yofs += lines[line].ascent;
	yend = yofs+lines[line].descent+1;   /* +1 because lines look scrunched
						without it. */

	for (i=0; i<lines[line].numblock; i++,block++) {
	    blocks[block].fid = auxblocks[block].font->fid;
	    switch (auxblocks[block].align) {
	      case LEFTALIGN:
		blocks[block].x = lofs;
		blocks[block].x1 = lofs;
		lofs += auxblocks[block].width;
		blocks[block].x2 = lofs;
		break;
		
	      case CENTERALIGN:
		blocks[block].x = cofs;
		blocks[block].x1 = cofs;
		cofs += auxblocks[block].width;
		blocks[block].x2 = cofs;
		break;

	      case RIGHTALIGN:
		blocks[block].x = rofs;
		blocks[block].x1 = rofs;
		rofs += auxblocks[block].width;
		blocks[block].x2 = rofs;
		break;
	    }
	    blocks[block].y = yofs;
	    blocks[block].y1 = ystart;
	    blocks[block].y2 = yend;
	    blocks[block].strindex = strindex;
	    blocks[block].strlen = auxblocks[block].len;
	    strncpy(gram->text+strindex, auxblocks[block].str,
		    auxblocks[block].len);
	    strindex += blocks[block].strlen;
	}

	yofs = yend;

    }

    if ((geometry = var_get_variable("X_geometry")),(geometry[0]=='\0')) 
      if ((geometry = xres_get_geometry(style))==NULL)
	if ((geometry = var_get_variable("default_X_geometry")),
	    (geometry[0]=='\0'))
	  geometry = "+0+0";
    sscanf(geometry, "%c%[0123456789c]%c%[0123456789c]", &xfrom, xpos,
	   &yfrom, ypos);

    if (xpos[0]=='c')
      gram_xalign = 0;
    else
      gram_xpos = atoi(xpos);
    if (xfrom=='-')
      gram_xalign *= -1;

    if (ypos[0]=='c')
      gram_yalign = 0;
    else
      gram_ypos = atoi(ypos);
    if (yfrom=='-')
      gram_yalign *= -1;
    
    gram_xsize = maxwidth+(internal_border_width<<1);
    gram_ysize = yofs+internal_border_width;
    gram->numblocks = num;
    gram->blocks = blocks;
    
    x_gram_create(dpy, gram, gram_xalign, gram_yalign, gram_xpos,
		  gram_ypos, gram_xsize, gram_ysize);
}

#define MODE_TO_FONT(dpy,style,mode) \
  get_font((dpy),(style),(mode)->substyle,(mode)->size, \
	   (mode)->bold+(mode)->italic*2)
void xshow(dpy, desc, numstr, numnl)
     Display *dpy;
     desctype *desc;
     int numstr;
     int numnl;
{
    XFontStruct *font;
    xmode_stack modes = xmode_stack_create();
    xmode curmode;
    xlinedesc *lines;
    xblock *blocks;
    xauxblock *auxblocks;
    int nextblock=0;
    int line=0,linestart=0;
    char *style;

    lines = (xlinedesc *)malloc(sizeof(xlinedesc)*(numnl+1));

    blocks = (xblock *)malloc(sizeof(xblock)*numstr);
    auxblocks = (xauxblock *)malloc(sizeof(xauxblock)*numstr);

    curmode.bold = 0;
    curmode.italic = 0;
    curmode.size = MEDIUM_SIZE;
    curmode.align = LEFTALIGN;
    curmode.substyle = string_Copy("default");

    style = var_get_variable("style");
    if (style[0] == '\0') {
       style = string_Concat(var_get_variable("class"),".");
       style = string_Concat2(style,var_get_variable("instance"));
       style = string_Concat2(style,".");
       style = string_Concat2(style,var_get_variable("sender"));
    }

    for (; desc->code!=DT_EOF; desc=desc->next) {
	switch (desc->code) {
	  case DT_ENV:
	    xmode_stack_push(modes, curmode);
	    curmode.substyle = string_Copy(curmode.substyle);
	    
#define envmatch(string,length) ((desc->len==(length)) && (strncasecmp(desc->str,(string),(length))==0))

	    if (envmatch("roman",5)) {
		curmode.bold = 0;
		curmode.italic = 0;
	    } else if (envmatch("bold",4) || envmatch("b",1))
	      curmode.bold = 1;
	    else if (envmatch("italic",6)||envmatch("i",1))
	      curmode.italic = 1;
	    else if (envmatch("large",5))
	      curmode.size = LARGE_SIZE;
	    else if (envmatch("medium",6))
	      curmode.size = MEDIUM_SIZE;
	    else if (envmatch("small",5))
	      curmode.size = SMALL_SIZE;
	    else if (envmatch("left",4)||envmatch("l",1))
	      curmode.align = LEFTALIGN;
	    else if (envmatch("center",6)||envmatch("c",1))
	      curmode.align = CENTERALIGN;
	    else if (envmatch("right",5)||envmatch("r",1))
	      curmode.align = RIGHTALIGN;
	    else if (envmatch("default",7)) {
	       free(curmode.substyle);
	       curmode.substyle = string_Copy("default");
	    } else if (envmatch("font",4)) {
	       /* lookahead needed.  desc->next->str should be the
		  font name, and desc->next->next->code should be
		  a DT_END*/
	       if ((desc->next) &&
		   (desc->next->next) &&
		   (desc->next->code == DT_STR) &&
		   (desc->next->next->code==DT_END)) {
		  curmode.size=SPECIAL_SIZE; /* This is an @font() */
		  free(curmode.substyle);
		  curmode.substyle=string_CreateFromData(desc->next->str,
							 desc->next->len);
		  /* skip over the rest of the @font */
		  desc=desc->next->next;
	       }
	    } else if (desc->len > 0) { /* avoid @{...} */
	       free(curmode.substyle);
	       curmode.substyle = string_CreateFromData(desc->str, desc->len);
	    }
	    break;

	  case DT_STR:
	    auxblocks[nextblock].align = curmode.align;
	    auxblocks[nextblock].font = MODE_TO_FONT(dpy,style,&curmode);
	    auxblocks[nextblock].str = desc->str;
	    auxblocks[nextblock].len = desc->len;
	    nextblock++;
	    break;

	  case DT_END:
	    free(curmode.substyle);
	    curmode = xmode_stack_top(modes);
	    xmode_stack_pop(modes);
	    break;

	  case DT_NL:
	    lines[line].startblock = linestart;
	    lines[line].numblock = nextblock-linestart;
	    font = MODE_TO_FONT(dpy,style,&curmode);
	    lines[line].ascent = font->ascent;
	    lines[line].descent = font->descent;
	    line++;
	    linestart = nextblock;
	    break;
	}
    }

    /* case DT_EOF:    will drop through to here. */

    if (linestart != nextblock) {
       lines[line].startblock = linestart;
       lines[line].numblock = nextblock-linestart;
       font = MODE_TO_FONT(dpy,style,&curmode);
       lines[line].ascent = 0;
       lines[line].descent = 0;
       line++;
    }
    
    free(curmode.substyle);
    fixup_and_draw(dpy, style, auxblocks, blocks, nextblock, lines, line);
    free(lines);
    free(auxblocks);
}

static void xhandleevent(dpy, w, event)
     Display *dpy;
     Window w;
     XEvent *event;
{
    x_gram *gram;
    
    if (XFindContext(dpy, w, desc_context, (caddr_t *)&gram))
      return;

    if (event->type == Expose)
      x_gram_expose(dpy, w, gram,&(event->xexpose));
    else
      xcut(dpy, event, desc_context);

    XFlush(dpy);
}

void x_get_input(dpy)
     Display *dpy;
{
    XEvent event;
    
    dprintf1("Entering x_get_input(%x).\n",dpy);

    /*
     * Kludge to get around lossage in XPending:
     *
     * (the problem: XPending on a partial packet returns 0 without
     *  reading in the packet.  This causes a problem when the X server
     *  dies in the middle of sending a packet.)
     */
    if (XPending(dpy)==0)
      XNoOp(dpy);  /* Ensure server is still with us... */
    
    while (XPending(dpy)) {
	XNextEvent(dpy,&event);
	xhandleevent(dpy, event.xany.window, &event);
    }
}
