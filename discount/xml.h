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

#include <iostream>
#include <sstream> 

#include "config.h"

#include "cstring.h"
#include "markdown.h"
#include "amalloc.h"

static char *mkd_xmlchar(unsigned char c);
int mkd_generatexml(char *p, int size, std::ostringstream &output);
int mkd_xml(char *p, int size, char **res);