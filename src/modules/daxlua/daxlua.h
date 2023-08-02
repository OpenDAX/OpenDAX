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
#define MODE_READ     0x01  /* Tag read from server before script runs */
#define MODE_WRITE    0x02  /* Tag written to the server after the scrpt runs */
#define MODE_STATIC   0x04  /* Static global value (mutulally exclusive from read / wrtite */
#define MODE_INIT     0x80  /* Flag set after then handle has been initialized */

#define COMMAND_ENABLE 0x01
#define COMMAND_DISABLE 0x02

/* This is the representation of a custom Lua global
   if the mode is static then the tagname will be written
   to the Lua registry otherwise it's either read, written
   or both to the OpenDAX server as a tag. */
typedef struct Global_t {
    char *name;
    char *tagname;
    unsigned char mode;
    tag_handle handle;
    int ref;
    struct Global_t *next;
} global_t;

typedef struct Trigger_t {
    char *tagname;
    int count;
    int type;
    lua_Number value;
    tag_handle handle;
    uint8_t *buff;
    dax_id id;
} trigger_t;

/* Contains all the information to identify a script */
typedef struct Script_t {
    lua_State *L;
    unsigned char enabled;
    unsigned char failed;
    //unsigned char trigger;
    unsigned char command;
    char *name;
    char *filename;
    char *threadname;
    int func_ref;
    trigger_t *script_trigger;
    trigger_t *enable_trigger;
    trigger_t *disable_trigger;
    unsigned char firstrun;
    unsigned char running;
    uint64_t lastscan;
    uint64_t thisscan;
    long executions;
    long interval;
    global_t *globals;
} script_t;

/* This represents the information for a single interval
   thread.  The scripts in this thread are run at each
   time the given 'interval' elapses.  Overruns are recorded*/
typedef struct Interval_thread_t {
    char *name;
    unsigned int interval;
    uint32_t overruns;
    pthread_t thread;
    unsigned int scriptcount; /* Number of scripts */
    script_t **scripts; /* Array of script pointers */
    struct Interval_thread_t *next;
} interval_thread_t;


/* luaopt.c - Configuration functions */
int configure(int argc, char *argv[]);

/* scripts.c - Script handling functions */
script_t *get_new_script(void);
void del_script(void);
void run_script(script_t *s);
int start_all_scripts(void);
int get_script_count(void);
script_t *get_script(int index);
script_t *get_script_name(char *name);
int add_global(script_t *script, char *varname, char* tagname, unsigned char mode);
int add_static(script_t *script, lua_State *L, char* varname);


/* luaif.c - Lua Interface functions */
int setup_interpreter(lua_State *L);
int fetch_tag(lua_State *L, tag_handle h);
int send_tag(lua_State *L, tag_handle h);

/* thread.c - Thread handling functions */
void thread_set_qthreadcount(unsigned int count);
void thread_set_queue_size(unsigned int size);
int add_interval_thread(char *name, long interval);
int thread_start_all(void);

#endif /* !__DAXLUA_H */
