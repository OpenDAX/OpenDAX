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
#include <syslog.h>
#include <signal.h>
#include <daemon.h>
#include <logger.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Writes the PID to the pidfile if one is configured */
static void
writepidfile(char *progname)
{
    int pidfd=0;
    char pid[10];
    char filename[41]; /* Arbitrary limit alert!!! */
    umask(0);
    snprintf(filename, 40, "%s/%s", PID_FILE_PATH, progname);
    pidfd=open(filename, O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(!pidfd) 
        xerror("Unable to open PID file - %s",filename);
    else {
        sprintf(pid, "%d", getpid());
        write(pidfd, pid, strlen(pid)); /*writes to the PID file*/
    }
    /* If we unlink the file while still open the kernel will delete
     it when we die */
    unlink(filename);
}

/* This function daemonizes the program. */
int
daemonize(char *progname)
{
    pid_t result;
    int n;
    char s[10];
   
    xlog(LOG_MAJOR, "Sending process to background");
    /* Call fork() and exit as the parent.  This returns control to the 
       command line and guarantees the program is not a process group
       leader. */
    result = fork();
    if(result < 0) {
        xerror("Failed initial process creation");
        return(-1);
    } else if(result > 0) {
        exit(0);
    }

    /*Call setsid() to dump the controlling terminal*/
    result=setsid();
   
    if(result<0) {
        xerror("Unable to dump controlling terminal");
        return(-1);
    }

    /* Call fork() and exit as the parent.  This assures that we
       will never again be able to connect to a controlling 
       terminal */
    result = fork();
    if(result < 0) {
        xerror("Unable to fork the final fork");
        return (-1);
    } else if(result > 0)
        exit(0);
   
    /* chdir() to / so that we don't hold any directories open */
    chdir("/");
    /* need control of the permissions of files that we write */
    umask(0);
    /* writes the PID to STDOUT */
    sprintf(s, "%d", getpid());
    write(2, s, strlen(s));
    writepidfile(progname);
    
    /* Closes the logger before we pull the rug out from under it
       by closing all the file descriptors */
    closelog();
    /* close all open file descriptors */
    for (n = getdtablesize(); n>=0; --n) close(n);
    /*reopen stdout stdin and stderr to /dev/null */
    n = open("/dev/null", O_RDWR);
    dup(n); dup(n);
    return 0;
}

