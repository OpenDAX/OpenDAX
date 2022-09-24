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
 *  Source code file for the OpenDAX Historical Logging module
 *
 *  This file contains all of the plugin handling code
 */

#include <dlfcn.h>
#include <opendax.h>
#include "histlog.h"

static void *plugin;

extern dax_state *ds;

static int (*_init)(dax_state *ds);
tag_object *(*add_tag)(const char *tagname, uint32_t type, const char *attributes);
int (*free_tag)(tag_object *tag);
int (*write_data)(tag_object *tag, void *value, double timestamp);
int (*flush_data)(void);
void (*set_timefunc)(double (*f)(void));

/* Loads the dynamic library that represents the plugin, sets all of the
 * function pointers to the correct symbols in the library and runs init() */
int
plugin_load(char *file) {
    if(file == NULL) {
        dax_log(LOG_ERROR, "Plugin not given");
        return ERR_GENERIC;
    }
    plugin = dlopen(file, RTLD_LAZY);
    if(!plugin) {
        dax_log(LOG_ERROR, "plugin loading error: %s", dlerror());
        return ERR_NOTFOUND;
    }
    /* TODO: Check these for errors and deal appropriately */
    *(void **)(&_init) = dlsym(plugin, "init");
    *(void **)(&add_tag) = dlsym(plugin, "add_tag");
    *(void **)(&free_tag) = dlsym(plugin, "free_tag");
    *(void **)(&write_data) = dlsym(plugin, "write_data");
    *(void **)(&flush_data) = dlsym(plugin, "flush_data");
    *(void **)(&set_timefunc) = dlsym(plugin, "set_timefunc");
    set_timefunc(hist_gettime);
    return _init(ds);
}


