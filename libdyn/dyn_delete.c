/*
 * This file is part of libdyn.a, the C Dynamic Object library.  It
 * contains the source code for the function DynDelete().
 *
 * There are no restrictions on this code; however, if you make any
 * changes, I request that you document them so that I do not get
 * credit or blame for your modifications.
 *
 * Written by Barr3y Jaspan, Student Information Processing Board (SIPB)
 * and MIT-Project Athena, 1989.
 */

#include <stdio.h>

#include "dynP.h"

/*
 * Checkers!  Get away from that "hard disk erase" button!
 *    (Stupid dog.  He almost did it to me again ...)
 */                                 
int DynDelete(obj, idx)
   DynObject obj;
   int idx;
{
     if (idx < 0) {
	  if (obj->debug)
	       fprintf(stderr, "dyn: delete: bad index %d\n", idx);
	  return DYN_BADINDEX;
     }
     
     if (idx >= obj->num_el) {
	  if (obj->debug)
	       fprintf(stderr, "dyn: delete: Highest index is %d.\n",
		       obj->num_el);
	  return DYN_BADINDEX;
     }

     if (idx == obj->num_el-1) {
	  if (obj->paranoid) {
	       if (obj->debug)
		    fprintf(stderr, "dyn: delete: last element, zeroing.\n");
	       (void) memset(obj->array + idx*obj->el_size, 0, obj->el_size);
	  }
	  else {
	       if (obj->debug)
		    fprintf(stderr, "dyn: delete: last element, punting.\n");
	  }
     }	  
     else {
	  if (obj->debug)
	       fprintf(stderr,
		       "dyn: delete: copying %d bytes from %p + %d to + %d.\n",
		       obj->el_size*(obj->num_el - idx), obj->array,
		       (idx+1)*obj->el_size, idx*obj->el_size);
	  
	  (void) memmove(obj->array + idx*obj->el_size,
			 obj->array + (idx+1)*obj->el_size,
			 obj->el_size*(obj->num_el - idx));

	  if (obj->paranoid) {
	       if (obj->debug)
		    fprintf(stderr,
			    "dyn: delete: zeroing %d bytes from %p + %d\n",
			    obj->el_size, obj->array,
			    obj->el_size*(obj->num_el - 1));
	       (void) memset(obj->array + obj->el_size*(obj->num_el - 1),
			     0, obj->el_size);
	  }
     }
     
     --obj->num_el;
     
     if (obj->debug)
	  fprintf(stderr, "dyn: delete: done.\n");

     return DYN_OK;
}
