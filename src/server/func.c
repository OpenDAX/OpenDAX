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

static uint32_t _logflags = 0;

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

/* Some general error handling functions. */
/* TODO: These should get changed to deal with logging
   and properly exiting the program.  For now just print to
   stderr and then send a \n" */
void
xfatal(const char *format, ...)
{
    va_list val;
    va_start(val, format);
#ifdef DAX_LOGGER
    vsyslog(LOG_ERR, format, val);
#else
    vfprintf(stderr, format, val);
    fprintf(stderr, "\n");
#endif
    va_end(val);
    kill(getpid(), SIGQUIT);
}

/* Logs an error everytime it's called.  These should be for internal
 * program errors only.  If this function is called it really should
 * be pointing out some serious condition within the program.  For user
 * caused errors the xlog() function should be used with the ERR_LOG
 * flag bit */
void
xerror(const char *format, ...)
{
    va_list val;
    va_start(val, format);
#ifdef DAX_LOGGER
    vsyslog(LOG_ERR, format, val);
#else
    vfprintf(stdout, format, val);
    fprintf(stdout, "\n");
#endif
    va_end(val);
}

void
set_log_topic(uint32_t topic)
{
    _logflags = topic;
    xlog(LOG_MAJOR, "Log Topics Set to %d", _logflags);
}

/* logs the string if any of the bits in flags matches _logflags */
void xlog(uint32_t flags, const char *format, ...) {
    va_list val;
    if(flags & _logflags) {
        va_start(val, format);
#ifdef DAX_LOGGER
        vsyslog(LOG_NOTICE, format, val);
#else
        vfprintf(stdout, format, val);
        fprintf(stdout, "\n");
#endif
        va_end(val);
    }
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
    
