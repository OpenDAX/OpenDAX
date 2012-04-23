/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2012 Phil Birkelbach
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

 * This is the source code file for the logging system.  Right now we either
 * log to the console (through stdout) or to syslog.
 */

#include <common.h>
#include <logger.h>
#include <syslog.h>
#include <signal.h>

static u_int32_t _logflags = LOG_ALL;

static void (*write_log)(int priority, const char *format, va_list val) = {NULL};
//static void
//write_log(int priority, const char *format, va_list val)
//{
//    vfprintf(stdout, format, val);
//    fprintf(stdout, "\n");
//}


void
log_syslog(int priority, const char *format, va_list val)
{
    vsyslog(priority, format, val);
}

void
log_stdout(int priority, const char *format, va_list val)
{
    /* TODO: Do something with the priority */
    vfprintf(stdout, format, val);
    fprintf(stdout, "\n");
}

/* This basically sets the function pointer that actually logs the output.
 * If this is not called before the first call to a logging function then bad
 * things will happen.
 */
void
logger_init(int type, char *progname)
{
    if(type == LOG_TYPE_SYSLOG) {
        printf("Opening Syslog\n");
        write_log = log_syslog;
        openlog(progname, LOG_NDELAY, LOG_DAEMON);
    } else {
        printf("Logging to STDOUT\n");
        write_log = log_stdout;
    }
}


/* logs the string if any of the bits in flags matches _logflags */
void
xlog(u_int32_t flags, const char *format, ...)
{
    va_list val;
    va_start(val, format);
    if(flags & _logflags) {
        write_log(LOG_NOTICE, format, val);
    }
    va_end(val);
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
    write_log(LOG_ERR, format, val);
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
    write_log(LOG_ERR, format, val);
    va_end(val);
}

void
set_log_topic(u_int32_t topic)
{
    _logflags = topic;
    xlog(LOG_MAJOR, "Log Topics Set to %d", _logflags);
}

