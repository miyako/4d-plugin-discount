/* markdown: a C implementation of John Gruber's Markdown markup language.
 *
 * Copyright (C) 2007 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "config.h"

#include "cstring.h"
#include "markdown.h"
#include "amalloc.h"

/* free a (single) line
 */
void
___mkd_freeLine(Discount::Line *ptr);

/* free a list of lines
 */
void
___mkd_freeLines(Discount::Line *p);

/* bye bye paragraph.
 */
void
___mkd_freeParagraph(Paragraph *p);


/* bye bye footnote.
 */
void
___mkd_freefootnote(Footnote *f);


/* bye bye footnotes.
 */
void
___mkd_freefootnotes(MMIOT *f);



/* initialize a new MMIOT
 */
void
___mkd_initmmiot(MMIOT *f, void *footnotes);


/* free the contents of a MMIOT, but leave the object alone.
 */
void
___mkd_freemmiot(MMIOT *f, void *footnotes);


/* free lines up to an barrier.
 */
void
___mkd_freeLineRange(Discount::Line *anchor, Discount::Line *stop);


/* clean up everything allocated in __mkd_compile()
 */
void
mkd_cleanup(Document *doc);
