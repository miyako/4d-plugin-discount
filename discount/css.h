/* markdown: a C implementation of John Gruber's Markdown markup language.
 *
 * Copyright (C) 2009 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <iostream>
#include <sstream> 

#include "config.h"

#include "cstring.h"
#include "markdown.h"
#include "amalloc.h"

static void stylesheets(Paragraph *p, Cstring *f);
int mkd_css(Document *d, char **res);
int mkd_generatecss(Document *d, FILE *f);
int mkd_generatecss(Document *d, std::ostringstream &outstream);