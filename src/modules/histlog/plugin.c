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

static int (*_init)(char *file);
int (*set_config)(const char *attr, char *value);
char * (*get_config)(const char *attr);
tag_object *(*add_tag)(const char *tagname, uint32_t type, const char *attributes);
int (*free_tag)(tag_object *tag);
int (*write_data)(uint32_t index, void *value, double timestamp);

/* Loads the dynamic library that represents the plugin, sets all of the
 * function pointers to the correct symbols in the library and runs init() */
int
plugin_load(char *file) {
    plugin = dlopen(file, RTLD_LAZY);
    if(!plugin) {
        dax_error(ds, "plugin loading error: %s\n", dlerror());
        return ERR_GENERIC;
    }
    /* TODO: Check these for errors and deal appropriately */
    *(void **)(&_init) = dlsym(plugin, "init");
    *(void **)(&set_config) = dlsym(plugin, "set_config");
    *(void **)(&get_config) = dlsym(plugin, "get_config");
    *(void **)(&add_tag) = dlsym(plugin, "add_tag");
    *(void **)(&free_tag) = dlsym(plugin, "free_tag");
    *(void **)(&write_data) = dlsym(plugin, "write_data");
    return _init(file);
}

