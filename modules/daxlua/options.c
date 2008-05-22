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

 * Configuration source code file for the Lua script interpreter
 */

#include <daxlua.h>
#include <string.h>
#include <getopt.h>
#include <math.h>



/* TODO: Allow user to set whether an error in a script is fatal */

static char *configfile;
static char *initscript;
static long verbosity;
static script_t *scripts = NULL;
static int scripts_size = 0;
static int scriptcount = 0;

static void initconfig(void) {
    int length;
    
    if(!configfile) {
        length = strlen(ETC_DIR) + strlen("/daxlua.conf") +1;
        configfile = (char *)malloc(sizeof(char) * length);
        if(configfile) 
            sprintf(configfile,"%s%s",ETC_DIR,"/daxlua.conf");
    }
}


/* This function parses the command line options and sets
the proper members of the configuration structure */
static void
parsecommandline(int argc, char *argv[])
{
    char c;
    
    static struct option options[] = {
    {"config", required_argument,0,'C'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument,0,'v'},
    {0, 0, 0, 0}
    };
    
    /* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "C:Vv", options, NULL)) != -1) {
        switch (c) {
            case 'C':
                configfile = strdup(optarg);
                break;
            case 'V':
                printf("daxlua Version %s\n",VERSION);
                break;
            case 'v':
                verbosity++;
                break;
            case '?':
                printf("Got the big ?\n");
                break;
            case -1:
            case 0:
                break;
            default:
                printf ("?? getopt returned character code 0%o ??\n", c);
        } /* End Switch */
    } /* End While */
    if(verbosity) dax_set_verbosity(verbosity);
}

/* This function is called last and makes sure that we have some sane defaults
   for all the configuration options */
static void setdefaults(void) {
    //--if(! rate) rate = 1000;
}

/* This function returns an index into the scripts[] array for
   the next unassigned script */
static int
_get_new_script(void)
{
    void *ns;
    
    /* Allocate the script array if it has not already been done */
    if(scriptcount == 0) {
        scripts = malloc(sizeof(script_t) * NUM_SCRIPTS);
        if(scripts == NULL) {
            dax_fatal("Cannot allocate memory for the scripts");
        }
        scripts_size = NUM_SCRIPTS;
    } else if(scriptcount == scripts_size) {
        ns = realloc(scripts, sizeof(script_t) * (scripts_size + NUM_SCRIPTS));
        if(ns != NULL) {
            scripts = ns;
        } else {
            dax_error("Failure to allocate additional scripts");
            return -1;
        }
    }
    scriptcount++;
    return scriptcount - 1;
}

/* Lua interface function for adding a modbus command to a port.  
 Accepts two arguments.  The first is the port to assign the command
 too, and the second is a table describing the command. */
static int
_add_script(lua_State *L)
{
    int tagcount, n, si;
    const char *string;
    
    if(! lua_istable(L, 1) ) {
        luaL_error(L, "Table needed to add script");
    }
    
    si = _get_new_script();
    if(si < 0) {
        /* Just bail for now */
        return 0;
    }
    
    lua_getfield(L, 1, "enable");
    scripts[si].enable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "name");
    string = lua_tostring(L, -1);
    if(string) {
        scripts[si].name = strdup(string);
    } else {
        scripts[si].name = NULL;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "filename");
    string = lua_tostring(L, -1);
    if(string) {
        scripts[si].filename = strdup(string);
    } else {
        scripts[si].filename = NULL;
        scripts[si].enable = 0;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "rate");
    scripts[si].rate = lua_tointeger(L, -1);
    if(scripts[si].rate <= 0) {
        scripts[si].rate = DEFAULT_RATE;
    }
    lua_pop(L, 1);
    
    lua_getfield(L, 1, "tags");
    if( ! lua_istable(L, -1) ) {
        /* TODO: I guess we could just get the value as a string */
        dax_log("tags should be specified as an array");
    } else {
        /* Find how many objects are in the table.*/
        tagcount = lua_objlen(L, -1);
        /* Allocate the tag array */
        scripts[si].tags = malloc(sizeof(char *) * tagcount);
        /* TODO: What to do if this fails? */
        for(n = 1; n <= tagcount; n++) {
            lua_rawgeti(L, -1, n);
            string = lua_tostring(L, -1);
            if(string) {
                /* TODO: What do do if this fails?? */
                scripts[si].tags[n-1] = strdup(string);
                printf("adding %s to the tag list\n", scripts[si].tags[n-1]);
            }
            
            lua_pop(L, 1);
        }
        scripts[si].tagcount = tagcount;
    }
    return 0;
}

/* Public function to initialize the module */
int
configure(int argc, char *argv[])
{
    lua_State *L;
    const char *string;
    
    initconfig();
    parsecommandline(argc,argv);
    setdefaults();
    
    L = lua_open();
 /* We don't open any librarires because we don't really want any
    function calls in the configuration file.  It's just for
    setting a few globals. */    
    lua_pushcfunction(L, _add_script);
    lua_setglobal(L,"add_script");
    
 /* load and run the configuration file */
    if(luaL_loadfile(L, configfile)  || lua_pcall(L, 0, 0, 0)) {
        dax_error("Problem executing configuration file %s", lua_tostring(L, -1));
        return 1;
    }
    
 /* tell lua to push these variables onto the stack */
    lua_getglobal(L, "init");
    string = lua_tostring(L, -1);
    if(string != NULL) {
        initscript = strdup(string);
    } else {
        initscript = NULL;
    }
    lua_pop(L, 1);
    
    lua_getglobal(L, "verbosity");
    
    if(verbosity == 0) { /* Make sure we didn't get anything on the commandline */
        verbosity = lround(lua_tonumber(L, -1));
        dax_set_verbosity(verbosity);
    }
    
    dax_debug(2, "Initialization Script set to %s",initscript);
    dax_debug(2, "Verbosity set to %d", verbosity);

    /* Clean up and get out */
    lua_close(L);
    
    return 0;
}

/* Configuration retrieval functions */
char *
get_init(void)
{
    return initscript;
}

int
get_scriptcount(void)
{
    return scriptcount;
}

script_t *
get_script(int index)
{
    if(index < 0 || index >= scriptcount) {
        return NULL;
    } else {
        return &scripts[index];
    }
}

//char *get_main(void) {
    //return mainscript;
//}

//int get_rate(void) {
    //return rate;
//}
