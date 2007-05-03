/*  opendcs - An open source distributed control system 
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
 */


#ifndef __MODULE_H
#define __MODULE_H

#include <sys/types.h>

/* Module Flags */
#define MFLAG_NORESTART 0x01
#define MFLAG_OPENPIPES 0x02

/* Modules are implemented as a circular doubly linked list */
typedef struct dcs_Module {
    unsigned int handle;
    pid_t pid;
    int status;         /* modules exit status */
    char *path;         /* modules execution */
    char **arglist;     /* exec() ready array of arguments */
    unsigned int flags;
    int pipe_in;        /* Redirected to the modules stdin */
    int pipe_out;       /* Redirected to the modules stdout */
    int pipe_err;       /* Redirected to the modules stderr */
    time_t starttime;
    struct dcs_Module *next,*prev;
} dcs_module;

/* Module List Handling Functions */
unsigned int add_module(char *, char *, unsigned int);
int del_module(unsigned int);
/* Module Query Functions */
//char *get_module_path(unsigned int);
//char *get_module_arglist(unsigned int);
//char *get_module_flags(unsigned int);

//dcs_module *get_module(int);

/* Module runtime functions */
pid_t start_module (unsigned int);
int stop_module(unsigned int);

#endif /* !__MODULE_H */
