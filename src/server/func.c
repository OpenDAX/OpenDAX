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
#include "func.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdarg.h>
#include <signal.h>

/* Wrapper functions - Mostly system calls that need special handling */

/* Wrapper for write.  This will block and retry until all the bytes
 * have been written or an error other than EINTR is returned */
ssize_t
xwrite(int fd, const void *buff, size_t nbyte)
{
    const void *sbuff;
    size_t left;
    ssize_t result;

    sbuff = buff;
    left = nbyte;

    while(left > 0) {
        result = write(fd, sbuff, left);
        if(result <= 0) {
            /* If we get interrupted by a signal... */
            if(result < 0 && errno == EINTR) {
                /*... then go again */
                result = 0;
            } else {
                /* return error */
                return -1;
            }
        }
        left -= result;
        sbuff += result;
    }
    return nbyte;
}

/* Memory management functions.  These are just to override the
 * standard memory management functions in case I decide to do
 * something creative with them later. */

void *
xmalloc(size_t num)
{
    void *new = malloc(num);
    if(new) memset(new, 0, num);
    return new;
}

void *
xrealloc(void *ptr, size_t num)
{
    void *new;
    if(!ptr) {
        new = xmalloc(num);
    } else {
        new = realloc(ptr, num);
    }
    return new;
}

void *
xcalloc(size_t count, size_t size)
{
    return xmalloc(count * size);
}

void
xfree(void *ptr) {
    free(ptr);
}

/* allocates and copies a string.  This string would have to be
   deallocated with free() */
char *
xstrdup(char *src)
{
    char *dest;
    dest=(char *)xmalloc((strlen(src) * sizeof(char)) +1);
    if(dest) {
        strcpy(dest, src);
    }
    return dest;
}

time_t
xtime(void) {
    struct timeval gettime;
    
    gettimeofday(&gettime, NULL);
    return gettime.tv_sec * 1000 + gettime.tv_usec / 1000;
}
    
