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
 *  Main source code file for the OpenDAX Bad Module
 */

/* This module is designed to misbehave.  It's purpose is for load testing
 * or testing features of the system that are supposed to detect misbehaviour
 * and act accordingly.  It can be made to flood the system with messages,
 * use up all the memory or use up as much CPU as possible.  It reads tags
 * from the tagserver to determine how to act so that other modules can 
 * force the behavior.
 */

#include <badmodule.h>

static dax_state *ds;

int
main(int argc,char *argv[])
{
    int result=0, flags;
    
    ds = dax_init("badmodule");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }
    
    dax_log(ds, "Starting module test");
    dax_set_debug_topic(ds, 0xFFFF); /* This should get them all out there */
        
    dax_init_config(ds, "daxtest");
    flags = CFG_CMDLINE | CFG_DAXCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "exitonfail","exitonfail", 'x', flags, "0");
    result += dax_add_attribute(ds, "testscript","testscript", 't', flags, "daxtest.lua");
    
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);
    
    if(dax_connect(ds))
        dax_fatal(ds, "Unable to register with the server");
    
    //script = dax_get_attr(ds, "testscript");

    /* Free the configuration memory once we are done with it */
    dax_free_config(ds);
    while(1) {

	}
    dax_disconnect(ds);
    
    return 0;
}
