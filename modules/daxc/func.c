/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *

 * This file contains general functions that are used throughout the
 * OpenDAX program for things like logging, error reporting and 
 * memory allocation.
 */

#include <common.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include "func.h"


/* Memory management functions.  These are just to override the
 * standard memory management functions in case I decide to do
 * something createive with them later. */

void *xmalloc(size_t num) {
    void *new = malloc(num);
    if(new) memset(new,0,num);
    return new;
}

void *xrealloc(void *ptr, size_t num) {
    void *new;
    if(!ptr) {
        new = xmalloc(num);
    } else {
        new = realloc(ptr,num);
    }
    return new;
}

void *xcalloc(size_t count, size_t size) {
    return xmalloc(count * size);
}

/* Some general error handling functions. */
/* TODO: These should get changed to deal with logging
   and properly exiting the program.  For now just print to
   stderr and then send a \n" */
void xfatal(const char *format,...) {
    va_list val;
    va_start(val,format);
    //vsyslog(LOG_ERR,format,val);
    vfprintf(stderr,format,val);
    fprintf(stderr,"\n");
    va_end(val);
    kill(getpid(),SIGQUIT);
}

void xerror(const char *format,...) {
    va_list val;
    va_start(val,format);
    //vsyslog(LOG_ERR,format,val);
    vfprintf(stderr,format,val);
    fprintf(stderr,"\n");
    va_end(val);
}

void xnotice(const char *format,...) {
    va_list val;
    va_start(val,format);
    //vsyslog(LOG_NOTICE,format,val);
    vfprintf(stderr,format,val);
    fprintf(stderr,"\n");
    va_end(val);
}

void xlog(int verbosity, const char *format,...) {
    va_list val;
    va_start(val,format);
    //vsyslog(LOG_NOTICE,format,val);
    vfprintf(stderr,format,val);
    fprintf(stderr,"\n");
    va_end(val);
}

/* allocates and copies a string.  This string would have to be
   deallocated with free() */
/* TODO: Should use strdup if available right??? */
char *xstrdup(char *src) {
    char *dest;
    dest=(char *)xmalloc((strlen(src)*sizeof(char))+1);
    if(dest) {
        strcpy(dest,src);
    }
    return dest;
}
