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

 *  Header file for the Lua script interpreter
 */

#ifndef __DAXLUA_H
#define __DAXLUA_H

#include <opendax.h>
#include <common.h>

/* This defines the starting number of scripts in the array */
#define NUM_SCRIPTS 8
#define DEFAULT_RATE 1000

typedef struct Script_t {
    char enable;
    pthread_t thread;
    char *name;
    char *filename;
    long rate;
    long lastscan;
    int executions;
    int tagcount;
    dax_tag *tags;
} script_t;

/* options.c - Configuration functions */
int configure(int argc, char *argv[]);
char *get_init(void);
int get_scriptcount(void);
script_t *get_script(int index);

//--char *get_main(void);
//--int get_rate(void);
int get_verbosity(void);

/* luaif.c - Lua Interface functions */
int daxlua_init(void);
int setup_interpreter(lua_State *L);


#endif /* !__DAXLUA_H */
