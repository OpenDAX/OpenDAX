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

 *  Source code file for opendax configuration
 */

#include <common.h>
#include "options.h"
#include "module.h"
#include "message.h"
#include "tagbase.h"
#include "func.h"

#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


static char *_configfile;
static char *_socketname;
static struct in_addr _serverip;
static unsigned int _serverport;
static int _min_buffers;


/* Initialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {

    _min_buffers = 0;
    _socketname = NULL;
    _serverip.s_addr = 0;
    _serverport = 0;
}

/* This function sets the defaults if nothing else has been done
   to the configuration parameters to this point */
static void
setdefaults(void)
{
    if(!_min_buffers) _min_buffers = DEFAULT_MIN_BUFFERS;
    if(!_socketname) _socketname = strdup("/tmp/opendax");
    if(!_serverport) _serverport = DEFAULT_PORT;
    if(!_serverip.s_addr) inet_aton("0.0.0.0", &_serverip);
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
        {"socketname", required_argument, 0, 'S'},
        {"serverip", required_argument, 0, 'I'},
        {"serverport", required_argument, 0, 'P'},
        {"version", no_argument, 0, 'V'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

/* Get the command line arguments */
    while ((c = getopt_long (argc, (char * const *)argv, "C:K:S:I:P:Vv", options, NULL)) != -1) {
        switch (c) {
        case 'C':
            _configfile = strdup(optarg);
            break;
        case 'T':
            dax_log_set_default_topics(strdup(optarg));
            break;
        case 'S':
            _socketname = strdup(optarg);
            break;
        case 'I':
            if(! inet_aton(optarg, &_serverip)) {
                dax_log(LOG_ERROR, "Unknown IP address %s", optarg);
            }
            break;
        case 'P':
            _serverport = strtol(optarg, NULL, 0);
            break;
        case 'V':
            printf("%s Version %s\n", PACKAGE, VERSION);
            exit(0);
            break;
        case 'v':
            dax_log_set_default_mask(LOG_ALL);
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

static int
readconfigfile(void)
{
    lua_State *L;
    int length;

    dax_log(2, "Reading Configuration file %s", _configfile);
    L = luaL_newstate();
    
    /* If no configuration file was given on the command line we build the default */
    if(_configfile == NULL) {
        length = strlen(ETC_DIR) + strlen("/tagserver.conf") +1;
        _configfile = (char *)malloc(sizeof(char) * length);
        if(_configfile) {
            sprintf(_configfile, "%s%s", ETC_DIR, "/tagserver.conf");
        } else {
            dax_log(LOG_FATAL, "Unable to allocate memory for configuration file");
        }
    }
    
    /* We don't open any librarires because we don't really want any
     function calls in the configuration file.  It's just for
     setting a few globals. */
    luaopen_base(L);

    dax_log_set_lua_function(L);

    lua_pushstring(L, "tagserver");
    lua_setglobal(L, CONFIG_GLOBALNAME);

    /* load and run the configuration file */
    if(luaL_loadfile(L, _configfile)  || lua_pcall(L, 0, 0, 0)) {
        dax_log(LOG_ERROR, "Problem executing configuration file - %s", lua_tostring(L, -1));
        return 1;
    }

    lua_getglobal(L, "min_buffers");
    if(_min_buffers == 0) { /* Make sure we didn't get anything on the commandline */
        _min_buffers = (int)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);

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
        dax_log(LOG_WARN, "Unable to read configuration running with defaults");
    }
    setdefaults();
    return 0;
}

struct in_addr
opt_serverip(void)
{
    return _serverip;
}

unsigned int
opt_serverport(void)
{
    return _serverport;
}

char *
opt_socketname(void)
{
    return _socketname;
}

int
opt_min_buffers(void)
{
    return _min_buffers;
}

