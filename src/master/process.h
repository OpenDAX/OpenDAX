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
 */

/* Header file for the process handling functions for the master process
 * handling program. */

#ifndef __PROCESS_H
#define __PROCESS_H

#include <common.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <netinet/in.h>

/* The difference between STARTED and RUNNING is whether or not the master
 * has either seen the startup string on the modules stdout or the wait timer
 * has elapsed. */
#define PSTATE_STARTED      0x01 /* Process has been started by master*/
#define PSTATE_RUNNING      0x02 /* Process is running */
#define PSTATE_CONDEMNED    0X04 /* Killed by master */
#define PSTATE_DEAD         0x08 /* Waiting for restart or deletion */

/* Process Flags */
#define PFLAG_RESTART       0x01
#define PFLAG_OPENPIPES     0x02
#define PFLAG_REGISTER      0x04

/* Modules are implemented as a circular doubly linked list */
typedef struct dax_process {
    /* configuration data */
    char *name;
    pid_t pid;          /* process id */
    int exit_status;    /* exit status */
    char *path;         /* process executable path */
    char **arglist;     /* exec() ready array of arguments */
    unsigned int flags; /* Configuration Flags for the process */
    char *env;          /* Environment to use for process */
    char *user;         /* Set UID to this user before exec() */
    char *group;        /* Set GID to this group before exec() */
    int delay;          /* Time to wait after the process starts mS*/
    double cpu;         /* CPU percentage threshold (0.0 - 1.0) */
    int mem;            /* Memory Threshold (kB) */
    /* internal usage data */
    int fd;             /* The socket file descriptor for this module */
    int efd;            /* The event notification file descriptor */
    int uid;
    int gid;
    long restartdelay;   /* Time to wait before restarting the process mS*/
    unsigned long last_ptime; /* jiffy time for process last time by */
    unsigned long last_etime; /* total cpu jiffy time last time by */
    /* status */
    unsigned int state; /* Process' Current Running State */
    long rss;           /* Current memory usage */
    float pcpu;         /* Current cpu percentage */
    int restartcount;   /* Used to control restart frequency */
    struct timeval starttime; /* Time that the process was started */
    struct timeval deadtime;  /* Time the process was cleaned up */
    /* next */
    struct dax_process *next;
} dax_process;

typedef struct {
    pid_t pid;
    int status;
} dead_process;

void process_init(void);
void process_start_all(void);
pid_t process_start(dax_process *);
int process_scan(void);
void process_dpq_add(pid_t, int);
dax_process *process_add(char *name, char *path, char *arglist, unsigned int flags);
void _print_process_list(void);


#endif
