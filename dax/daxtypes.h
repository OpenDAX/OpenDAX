/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 
 * This header contains the private type definitions 
 * for the opendax server and library
 */

#ifndef __DAXTYPES_H
#define __DAXTYPES_H

/* Module Flags */
#define MFLAG_RESTART       0x01
#define MFLAG_OPENPIPES     0x02
#define MFLAG_REGISTER      0x04


/* Modules are implemented as a circular doubly linked list */
typedef struct dax_Module {
    char *name;
    pid_t pid;          /* the modules process id */
    long int mid;       /* the modules id (currently the same as the pid) */
    int exit_status;    /* modules exit status */
    char *path;         /* modules execution */
    char **arglist;     /* exec() ready array of arguments */
    int startup;        /* Execution order 0 = no auto start */
    unsigned int flags; /* Configuration Flags for the module */
    unsigned int state; /* Modules Current Running State */
    int pipe_in;        /* Redirected to the modules stdin */
    int pipe_out;       /* Redirected to the modules stdout */
    int pipe_err;       /* Redirected to the modules stderr */
    int fd;             /* The socket file descriptor for this module */
    char umarsh;        /* Whether the socket data will be marshaled */
    time_t starttime;
    struct dax_Module *next,*prev;
} dax_module;

    

#endif /* !__DAXTYPES_H */
