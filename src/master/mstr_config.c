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


#include "mstr_config.h"
#include "process.h"
#include <getopt.h>
#include <sys/stat.h>

static char *_pidfile;
static char *_configfile;
static int _daemonize;

/* Initialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {
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
        {"logtopics", required_argument, 0, 'T'},
        {"deamonize", no_argument, 0, 'D'},
        {"pidfile", required_argument, 0, 'p'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

/* Get the command line arguments */
    while ((c = getopt_long (argc, (char * const *)argv, "C:p:VvD",options, NULL)) != -1) {
        switch (c) {
        case 'C':
            _configfile = strdup(optarg);
            break;
        case 'T':
            dax_log_set_default_topics(strdup(optarg));
            break;
        case 'p':
            _pidfile = strdup(optarg);
            break;
        case 'V':
            printf("%s Version %s\n", PACKAGE, VERSION);
            break;
        case 'v':
            dax_log_set_default_mask(DAX_LOG_ALL);
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
            dax_log(DAX_LOG_ERROR, "Unable to find uid for user %s", proc->user);
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
            dax_log(DAX_LOG_ERROR, "Unable to find gid for group %s", proc->group);
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
        dax_log(DAX_LOG_ERROR, "No process name given");
        return 0;
    }
    lua_pop(L, 1);

    path = NULL;
    lua_getfield(L, -1, "path");
    path = (char *)lua_tostring(L, -1);
    if(path == NULL) {
        dax_log(DAX_LOG_ERROR, "No path given for process %s", name);
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

/* Make sure that if we are running as root, we do not execute
   a configuration file that is owned or writable by a user
   other than root.  Since we are setuid and we run arbitraty
   commands from the configuration file this would be a bit
   security problem.  If we are not root then we don't care.
   Returns zero if all is good */
static int
_check_config_permissions(char *filename)
{
    int result;
    uid_t euid, ruid;
    struct stat sb;

    euid = geteuid();
    ruid = getuid();

    /* If our effective user id is 0 then we have root permissions, if our real user id is 0
       then we were actually run by the root user.  If we are running as root but were not
       run by the root user (setuid bit set) then we have to make sure that everything is okay
       with the configuration file.  If we are not root then none of this matters and we can just
       let the normal file system permissions deal with it */
    if(euid == 0 && ruid != 0) {
        dax_log(DAX_LOG_DEBUG, "We are running as root so we much check the config file permissions");
        result = stat(filename, &sb);
        if(result) {
            dax_log(DAX_LOG_FATAL, "Unable to retrieve file information for file %s - %s", filename, strerror(errno));
            return -1;
        }
        if(sb.st_uid != 0 || sb.st_gid != 0) {
            dax_log(DAX_LOG_FATAL, "Configuration File %s, is not owned by root", filename);
            return -1;
        }
        if(sb.st_mode & 0x0002) { /* Is the config file writable by others */
            dax_log(DAX_LOG_FATAL, "Configuration File %s, cannot be writable by others if we are run as root", filename);
            return -1;
        }
    }
    return 0;
}

static int
readconfigfile(void)
{
    int result;
    lua_State *L;
    int length;
    char *string;

    dax_log(2, "Reading Configuration file %s", _configfile);
    L = luaL_newstate();

    /* If no configuration file was given on the command line we build the default */
    if(_configfile == NULL) {
        length = strlen(ETC_DIR) + strlen("/opendax.conf") +1;
        _configfile = (char *)malloc(sizeof(char) * length);
        if(_configfile) {
            sprintf(_configfile, "%s%s", ETC_DIR, "/opendax.conf");
        } else {
            dax_log(DAX_LOG_FATAL, "Unable to allocate memory for configuration file");
        }
    }
    result = _check_config_permissions(_configfile);
    if(result) return result;
    /* We don't open any librarires because we don't really want any
     function calls in the configuration file.  It's just for
     setting a few globals. */
    luaopen_base(L);

    lua_pushcfunction(L, _add_process);
    lua_setglobal(L, "add_process");

    dax_log_set_lua_function(L);

    lua_pushstring(L, "opendax");
    lua_setglobal(L, CONFIG_GLOBALNAME);

    /* load and run the configuration file */
    if(luaL_loadfile(L, _configfile)  || lua_pcall(L, 0, 0, 0)) {
        dax_log(DAX_LOG_ERROR, "Problem executing configuration file - %s", lua_tostring(L, -1));
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
        dax_log(DAX_LOG_FATAL, "Problem reading configuration file");
        return -1;
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
