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
 */


#ifndef __MODULE_H
#define __MODULE_H

#include <common.h>
#include <dax/daxtypes.h>

#define MSTATE_RUNNING      0x00 /* Running normally */
#define MSTATE_WAITING      0x01 /* Waiting for restart */
#define MSTATE_CHILD        0x02 /* Module was started by this program */
#define MSTATE_REGISTERED   0x04 /* Is the module registered */

typedef struct {
    pid_t pid;
    int status;
} dead_module;

/* Module List Handling Functions */
//--mod_handle_t module_add(char *name, char *path, char *arglist, int startup, unsigned int flags);
dax_module *module_add(char *name, char *path, char *arglist, int startup, unsigned int flags);
//--int module_del(mod_handle_t);
int module_del(dax_module *);

/* Module runtime functions */
void module_start_all(void);
pid_t module_start (dax_module *);
int module_stop(dax_module *);
void module_register(char *, pid_t);
void module_unregister(pid_t);
//mod_handle_t module_get_pid(pid_t);
dax_module *module_get_pid(pid_t);

/* module maintanance */
void module_scan(void);

void module_dmq_add(pid_t, int);

#endif /* !__MODULE_H */
