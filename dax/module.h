/*  OpenDAX - An open source distributed control system 
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
 */


#ifndef __MODULE_H
#define __MODULE_H

#include <sys/types.h>

/* Module Flags */
#define MFLAG_NORESTART 0x01
#define MFLAG_OPENPIPES 0x02

#define MSTATE_RUNNING      0x00 /* Running normally */
#define MSTATE_WAITING      0x01 /* Waiting for restart */
#define MSTATE_CHILD        0x02 /* Module was started by this program */
#define MSTATE_REGISTERED   0x04 /* Is the module registered */

typedef unsigned int mod_handle_t;

/* Modules are implemented as a circular doubly linked list */
typedef struct dax_Module {
    mod_handle_t handle;
    char *name;
    pid_t pid;
    int exit_status;    /* modules exit status */
    char *path;         /* modules execution */
    char **arglist;     /* exec() ready array of arguments */
    unsigned int flags; /* Configuration Flags for the module */
    unsigned int state; /* Modules Current Running State */
    int pipe_in;        /* Redirected to the modules stdin */
    int pipe_out;       /* Redirected to the modules stdout */
    int pipe_err;       /* Redirected to the modules stderr */
    time_t starttime;
    struct dax_Module *next,*prev;
} dax_module;

typedef struct DeadModule {
    pid_t pid;
    int status;
} dead_module;

/* Module List Handling Functions */
mod_handle_t module_add(char *, char *, char *, unsigned int);
int module_del(mod_handle_t);

/* Module runtime functions */
pid_t module_start (mod_handle_t);
int module_stop(mod_handle_t);
void module_register(char *,pid_t);
void module_unregister(pid_t);
mod_handle_t module_get_pid(pid_t);

/* module maintanance */
void module_scan(void);

void module_dmq_add(pid_t,int);

#endif /* !__MODULE_H */
