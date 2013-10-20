/*
 * toc -- spit out a table of contents based on header blocks
 *
 * Copyright (C) 2008 Jjgod Jiang, David L Parsons
 * portions Copyright (C) 2011 Stefano D'Angelo
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <iostream>
#include <sstream> 

#include "cstring.h"
#include "markdown.h"
#include "amalloc.h"

/* write an header index
 */
int
mkd_toc(Document *p, char **doc);
int mkd_generatetoc(Document *p, FILE *out);
int mkd_generatetoc(Document *p, std::ostringstream &outstream);