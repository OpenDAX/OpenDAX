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
 *  Main source code file for the OpenDAX example module
 */

#include <skel.h>

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int flags, result = 0;
    char *str,*tagname, *event_tag, *event_type;
    
    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("skel"); /* Replace 'skel' with your module name */
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }

    /* Create and initialize the configuration subsystem in the library */
    dax_init_config(ds, "skel");
    /* These flags will be passed to the dax_add_attribute() function.  They
     * basically mean that the following attributes can be set from either the
     * command line or the skel.conf module configuration file and that an
     * argument is required */
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "tagname","tagname", 't', flags, "skel");
    result += dax_add_attribute(ds, "event_tag","event_tag", 'e', flags, "skel_event");
    /* For these attributes we add the opendax.conf configuration file as a
     * place where the attrubute can be set. */
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_DAXCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "event_type","event_type", 'y', flags, "poll");
    /* Execute the configuration */
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_DAXCONF);

    /* Get the results of the configuration */
    str = dax_get_attr(ds, "tagname");
    if(str != NULL) tagname = strdup(str);
    str = dax_get_attr(ds, "event_tag");
    if(str != NULL) event_tag = strdup(str);
    str = dax_get_attr(ds, "event_type");
    if(str != NULL) event_type = strdup(str);
    /* Copying all of the strings into local variables might not be
     * the most efficient way of doing this.  The pointers returned
     * from dax_get_attr() function point to the strings inside the
     * configuration system.  Once dax_free_config() is called they
     * would no longer exist but the configuration can be freed at
     * any time or never freed if that makes sense for the module. We
     * did it this way in the skel module for simplicity */
    
    /* Free the configuration data */
    dax_free_config (ds);

    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    /* Check for OpenDAX and register the module */
    if( dax_mod_register(ds, "skel") ) {
        dax_fatal(ds, "Unable to find OpenDAX");
        getout(_quitsignal);
    }


    while(1) {
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        
    }
    
 /* This is just to make the compiler happy */
    return(0);
}

/* Signal handler */
void
quit_signal(int sig)
{
    _quitsignal = sig;
}

static void
getout(int exitstatus)
{
    dax_mod_unregister(ds);
    exit(exitstatus);
}
