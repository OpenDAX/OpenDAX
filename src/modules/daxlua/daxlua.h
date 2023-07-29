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
#include <libdaxlua.h>
#include <pthread.h>

/* This defines the starting number of scripts in the array */
#define NUM_SCRIPTS 8
#define DEFAULT_RATE 1000

/* These are the modes for the scripts globals. */
#define MODE_READ   0x01
#define MODE_WRITE  0x02
#define MODE_STATIC 0X04

/* This is the representation of a custom Lua global
   if the mode is static then the tagname will be written
   to the Lua registry otherwise it's either read, written
   or both to the OpenDAX server as a tag. */
typedef struct Global_t {
    char *name;
    unsigned char mode;
    tag_handle handle;
    int ref;
    struct Global_t *next;
} global_t;

/* Contains all the information to identify a script */
typedef struct Script_t {
    lua_State *L;
    char enable;
    char trigger;
    char *name;
    char *filename;
    char *threadname;
    int func_ref;
    char *event_tagname;
    int event_count;
    int event_type;
    lua_Number event_value;
    unsigned char firstrun;
    uint64_t lastscan;
    uint64_t thisscan;
    long executions;
    long rate;
    global_t *globals;
} script_t;

/* This represents the information for a single interval
   thread.  The scripts in this thread are run at each
   time the given 'interval' elapses.  Overruns are recorded*/
typedef struct Interval_thread_t {
    char *name;
    unsigned int interval;
    pthread_t thread;
    unsigned int scriptcount; /* Number of scripts */
    script_t **scripts; /* Array of script pointers */
    struct Interval_thread_t *next;
} interval_thread_t;

typedef struct Queue_thread_t {
    pthread_t thread;
} queue_thread_t;



/* luaopt.c - Configuration functions */
int configure(int argc, char *argv[]);
// char *get_init(void);

/* scripts.c - Script handling functions */
script_t *get_new_script(void);
void del_script(void);
void run_script(script_t *s);
int start_all_scripts(void);
int get_script_count(void);
script_t *get_script(int index);
script_t *get_script_name(char *name);


/* luaif.c - Lua Interface functions */
int daxlua_init(void);
int setup_interpreter(lua_State *L);
int fetch_tag(lua_State *L, tag_handle h);
int send_tag(lua_State *L, tag_handle h);
void tag_dax_to_lua(lua_State *L, tag_handle h, void* data);
int tag_lua_to_dax(lua_State *L, tag_handle h, void* data, void *mask);

/* thread.c - Thread handling functions */
void thread_set_qthreadcount(unsigned int count);
void thread_set_queue_size(unsigned int size);
int add_interval_thread(char *name, long interval);
void thread_start_all(void);

#endif /* !__DAXLUA_H */
