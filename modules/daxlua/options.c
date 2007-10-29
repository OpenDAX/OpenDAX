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
#include <options.h>
#include <string.h>
#include <getopt.h>
#include <math.h>

/* TODO: Allow user to set verbosity to a number */
/* TODO: Allow user to set whether an error in a script is fatal */

static char *configfile;
static char *initscript;
static char *mainscript;
static long rate;

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
static void parsecommandline(int argc, char *argv[])  {
    char c;
    
    static struct option options[] = {
    {"config",required_argument,0,'C'},
    {"version",no_argument, 0, 'V'},
    {"verbose",no_argument,0,'v'},
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
                setverbosity(10);
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
}


static void setdefaults(void) {
    if(! rate) rate = 1000;
}

/* Public function to initialize the module */
int configure(int argc, char *argv[]) {
    lua_State *L;
    
    initconfig();
    parsecommandline(argc,argv);
    setdefaults();
    
    L = lua_open();
 /* We don't open any librarires because we don't really want any
    function calls in the configuration file.  It's just for
    setting a few globals. */
    
 /* load and run the configuration file */
    if(luaL_loadfile(L, configfile)  || lua_pcall(L, 0, 0, 0)) {
        xerror("Problem executing configuration file %s", configfile);
        return 1;
    }
    
 /* tell lua to push these variables onto the stack */
    lua_getglobal(L, "init");
    lua_getglobal(L, "main");
    lua_getglobal(L, "rate");
    
    initscript = strdup(lua_tostring(L,-3));
    mainscript = strdup(lua_tostring(L, -2));
    rate = lround(lua_tonumber(L, -1));
    
 /* bounds check rate */
    if(rate < 10 || rate > 100000) {
        xerror("Rate is out of bounds.  Setting to 1000.");
        rate = 1000;
    }
    xlog(2,"Initialization Script set to %s",initscript);
    xlog(2,"Main Script set to %s", mainscript);
    xlog(2,"Rate set to %d", rate);
 /* Clean up and get out */
    lua_close(L);
    
    return 0;
}

/* Configuration retrieval functions */
char *get_init(void) {
    return initscript;
}

char *get_main(void) {
    return mainscript;
}

int get_rate(void) {
    return rate;
}
