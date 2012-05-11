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
 
 *  Source code file for opendax master daemon configuration
 */


#include <mstr_config.h>
#include <logger.h>
#include <process.h>
#include <getopt.h>

static char *_pidfile;
static char *_configfile;
static int _daemonize;
static int _verbosity;

/* Initialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {
    int length;
    
    if(!_configfile) {
        length = strlen(ETC_DIR) + strlen("/opendax.conf") +1;
        _configfile = (char *)malloc(sizeof(char) * length);
        if(_configfile) 
            sprintf(_configfile, "%s%s", ETC_DIR, "/opendax.conf");
    }
    _daemonize = -1; /* We set it to negative so we can determine when it's been set */    
    _pidfile = NULL;
}

/* This function sets the defaults if nothing else has been done 
   to the configuration parameters to this point */
static void
setdefaults(void)
{
    if(_daemonize < 0) _daemonize = DEFAULT_DAEMONIZE;
    if(!_pidfile) _pidfile = DEFAULT_PID;
}

/* This function parses the command line options and sets
   the proper members of the configuration structure */
static void
parsecommandline(int argc, const char *argv[])
{
    char c;

    static struct option options[] = {
        {"config", required_argument, 0, 'C'},
        {"deamonize", no_argument, 0, 'D'},
        {"pidfile", required_argument, 0, 'p'},
        {"logger", required_argument, 0, 'L'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
      
/* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "C:p:L:VvD",options, NULL)) != -1) {
        switch (c) {
        case 'C':
            _configfile = strdup(optarg);
            break;
        case 'p':
            _pidfile = strdup(optarg);
            break;
        case 'V':
            printf("%s Version %s\n", PACKAGE, VERSION);
            break;
        case 'v':
            _verbosity++;
            break;
        case 'D': 
            _daemonize = 1;
            break;
        case '?':
            printf("Got the big ?\n");
            break;
        case -1:
        case 0:
            break;
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
            break;
        } /* End Switch */
    } /* End While */           
}

/* If the uid was not set in the configuration but the user string was
 * then we find the uid for that user.  If we can't find the uid for the
 * user then we set the uid to negative.  If the uid is negative then we
 * will not start the process.  If it's zero then the process get's the
 * master's uid.  Same for gid. */
static void
_set_uid_gid(dax_process *proc)
{
    struct passwd *pw;
    struct group *gr;

    /* If the actual 'uid' is set then we'll let that override
     * the .user field. */
    if(proc->uid < 0  && proc->user != NULL) {
        pw = getpwnam(proc->user);
        if(pw != NULL) {
            proc->uid = pw->pw_uid;
        } else {
            xlog(LOG_ERROR, "Unable to find uid for user %s", proc->user);
        }
    }
    /* Neither the uid or the username were set */
    if(proc->uid < 0 && proc->user == NULL) {
        proc->uid = 0;
    }

    if(proc->gid < 0 && proc->group != NULL) {
        gr = getgrnam(proc->group);
        if(gr != NULL) {
            proc->gid = gr->gr_gid;
        } else {
            xlog(LOG_ERROR, "Unable to find gid for group %s", proc->group);
        }
    }
    /* Neither the gid or the groupname were set */
    if(proc->gid < 0 && proc->group == NULL) {
        proc->gid = 0;
    }

}

/* This is the wrapper for the add module function in the Lua configuration file */
/* The arguments are a table that consist of all the configuration
   parameters of a module.  See the opendax.conf for examples */
static int
_add_process(lua_State *L)
{
    char *name, *path, *arglist;
    unsigned int flags = 0;
    dax_process *proc;

    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_process() received an argument that is not a table");
    }

    lua_getfield(L, -1, "name");
    if( !(name = (char *)lua_tostring(L, -1)) ) {
        xerror("No process name given");
        return 0;
    }
    lua_pop(L, 1);
    
    path = NULL;
    lua_getfield(L, -1, "path");
    path = (char *)lua_tostring(L, -1);
    if(path == NULL) {
        xerror("No path given for process %s", name);
        return 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "args");
    arglist = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "restart");
    if(lua_toboolean(L, -1)) {
        flags |= PFLAG_RESTART;
    }
    lua_pop(L, 1);

    proc = process_add(name, path, arglist, flags);

    /* for some reason not doing this strdup() would cause the username
     * 'opendax' to be read as 'openda'???*/
    lua_getfield(L, -1, "user");
    proc->user = strdup((char *)lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "group");
    proc->group = strdup((char *)lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "uid");
    if(! lua_isnil(L, -1)) {
        proc->uid = (int)lua_tointeger(L, -1);
    } else {
        proc->uid = -1;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "gid");
    if(! lua_isnil(L, -1)) {
        proc->gid = (int)lua_tointeger(L, -1);
    } else {
        proc->gid = -1;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "env");
    proc->env = (char *)lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "delay");
    proc->delay = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "restartdelay");
    proc->restartdelay = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "cpu");
    proc->cpu = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "mem");
    proc->mem = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    _set_uid_gid(proc);
    proc->flags = flags;
    return 0;
}

static int
readconfigfile(void)
{
    lua_State *L;
    char *string;
    
    xlog(2, "Reading Configuration file %s", _configfile);
    L = lua_open();
    /* We don't open any librarires because we don't really want any
     function calls in the configuration file.  It's just for
     setting a few globals. */
    
    lua_pushcfunction(L, _add_process);
    lua_setglobal(L, "add_process");
    
    luaopen_base(L);
    
    lua_pushstring(L, "opendax");
    lua_setglobal(L, CONFIG_GLOBALNAME);
    
    /* load and run the configuration file */
    if(luaL_loadfile(L, _configfile)  || lua_pcall(L, 0, 0, 0)) {
        xerror("Problem executing configuration file - %s", lua_tostring(L, -1));
        return 1;
    }
    
    /* tell lua to push these variables onto the stack */
    lua_getglobal(L, "daemonize");
    if(_daemonize < 0) { /* negative if not already set */
        _daemonize = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    
//    lua_getglobal(L, "statustag");
//    /* Do I really need to do this???? */
//    if(_statustag == NULL) {
//        if( (string = (char *)lua_tostring(L, -1)) ) {
//            _statustag = strdup(string);
//        }
//    }
//    lua_pop(L, 1);
    
    lua_getglobal(L, "pidfile");
    if(_pidfile == NULL) {
        if( (string = (char *)lua_tostring(L, -1)) ) {
            _pidfile = strdup(string);
        }
    }
    lua_pop(L, 1);
    
//    lua_getglobal(L, "min_buffers");
//    if(_min_buffers == 0) { /* Make sure we didn't get anything on the commandline */
//        _min_buffers = (int)lua_tonumber(L, -1);
//    }
//    lua_pop(L, 1);

//    lua_getglobal(L, "start_timeout");
//    if(_start_timeout == 0) {
//        _start_timeout = (int)lua_tonumber(L, -1);
//    }
//    lua_pop(L, 1);

    /* TODO: This needs to be changed to handle the new topic handlers */
    if(_verbosity == 0) { /* Make sure we didn't get anything on the commandline */
        //_verbosity = (int)lua_tonumber(L, 4);
        //set_log_topic(_verbosity);
        set_log_topic(0xFFFFFFFF);    
    }
    
    /* Clean up and get out */
    lua_close(L);
    
    return 0;
}


/* This function should be called from main() to configure the program.
 * After the configurations have been initialized the command line is
 * parsed.  Then the configuration file is read and after that if any
 * parameter has not been set the defaults are used. */
int
opt_configure(int argc, const char *argv[])
{    
    initconfig();
    parsecommandline(argc, argv);
    if(readconfigfile()) {
        xerror("Unable to read configuration running with defaults");
    }
    setdefaults();

    return 0;
}
 
int
opt_daemonize(void)
{
    return _daemonize;
}

char *
opt_pidfile(void)
{
    return _pidfile;
}

