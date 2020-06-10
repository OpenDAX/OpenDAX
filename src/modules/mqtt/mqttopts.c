/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2020 Phil Birkelbach
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
 *  Main source code file for the OpenDAX MQTT module configuration options
 */

#include "mqtt.h"

static subscriber_t *subscribers = NULL; /* Array of subscription definitions */
static int subscriber_count = 0;      /* Number of items in the array */
static int subscriber_size = 0;       /* Current size of the array */
static publisher_t *publishers = NULL;
static int publisher_count = 0;
static int publisher_size = 0;

extern dax_state *ds;


/* This function returns an index into the subscribers[] array for
   the next unassigned subscription */
static int
_get_new_sub(void)
{
    void *ns;
    int n;

    /* Allocate the script array if it has not already been done */
    if(subscriber_count == 0) {
        subscribers = malloc(sizeof(subscriber_t) * SUB_START_SIZE);
        if(subscribers == NULL) {
            dax_fatal(ds, "Cannot allocate memory for the subscription list");
        }
        subscriber_size = SUB_START_SIZE;
    } else if(subscriber_count == subscriber_size) {
        ns = realloc(subscribers, sizeof(subscriber_t) * (subscriber_size * 2));
        if(ns != NULL) {
            subscribers = ns;
        } else {
            dax_error(ds, "Failure to allocate additional subscriptions");
            return -1;
        }
    }
    n = subscriber_count;
    subscriber_count++;
    /* Initialize the script structure */
    subscribers[n].enabled = ENABLE_UNINIT;
    subscribers[n].format_type = CFG_STR;
    subscribers[n].tag_count = 0;
    subscribers[n].tagnames = NULL;
    subscribers[n].qos = 0;
    subscribers[n].h = NULL;
    subscribers[n].topic = NULL;
    subscribers[n].update_mode = CFG_CHANGE;
    return n;
}


static int
_add_sub(lua_State *L)
{
    int  idx;
    int i, len;
    const char *s;
    
    if(! lua_istable(L, 1) ) {
        luaL_error(L, "add_sub() argument is not a table");
    }
    idx = _get_new_sub();
    if(idx < 0) {
        luaL_error(L, "Unable to allocate new subscription");
    }

    lua_getfield(L, 1, "topic");
    if( lua_isnil(L, -1)) {
        luaL_error(L, "Topic is required for subscription");
    } else {
        s = lua_tostring(L, -1);
        subscribers[idx].topic = strdup(s);
    }
    lua_pop(L, 1);

    /* Retrieve tagname string or tagnames array from table */
    lua_getfield(L, 1, "tagname");
    if( lua_isnil(L, -1)) {
        lua_getfield(L, 1, "tagnames");
        if( lua_isnil(L, -1)) {
            luaL_error(L, "At least one tagname is required for subscription");
        }
        if(! lua_istable(L, -1)) {
            luaL_error(L, "Tagnames should be a table");
        } else {
            lua_len(L, -1);
            len = lua_tointeger(L, -1);
            lua_pop(L, 1);
            subscribers[idx].tagnames = malloc(sizeof(char *) * len);
            if(subscribers[idx].tagnames == NULL) {
                luaL_error(L, "Unable to allocate space for tagnames");
            }
        
            for(i = 0; i < len; i++) {
                lua_geti(L, -1, i+1);
                s = lua_tostring(L, -1);
                lua_pop(L, 1);
                subscribers[idx].tagnames[i] = strdup(s);
            }
            subscribers[idx].tag_count = len;
        }
    } else {
        s = lua_tostring(L, -1);
        subscribers[idx].tagnames = malloc(sizeof(char *));
        if(subscribers[idx].tagnames == NULL) {
            luaL_error(L, "Unable to allocate space for tagnames");
        }
        subscribers[idx].tagnames[0] = strdup(s);
        subscribers[idx].tag_count = 1;
    }

    lua_pop(L, 1); /* Pop off tagnmaes */
    
    lua_getfield(L, 1, "type");
    if(! lua_isnil(L, -1)) {
        subscribers[idx].format_type = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); /* Pop off tagnmaes */
    
    lua_getfield(L, 1, "format");
    
    if(! lua_isnil(L, -1)) {
        s = lua_tostring(L, -1);
        subscribers[idx].format_str = strdup(s);
        if(subscribers[idx].format_str == NULL) {
            luaL_error(L, "Unable to allocate space for format string");
        }
    }
    lua_pop(L, 1); /* Pop off tagnmaes */

    lua_getfield(L, 1, "qos");
    if(! lua_isnil(L, -1)) {
        subscribers[idx].qos = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); /* Pop off tagnmaes */
    
    return 0;
}

static int
_add_pub(lua_State *L)
{
    printf("Adding Publisher Not Implemented Yet!\n");
    return 0;
}


/* Public function to initialize the module */
int
configure(int argc, char *argv[])
{
    int flags, result = 0;
    lua_State *L;

    dax_init_config(ds, "daxmqtt");
    /* Set some globals for the  configuration script to use. */
    /* Minor problem here is that the script could change these */
    L = dax_get_luastate(ds);
    lua_pushinteger(L, CFG_RAW);
    lua_setglobal(L, "RAW");
    lua_pushinteger(L, CFG_STR);
    lua_setglobal(L, "STR");
    lua_pushinteger(L, CFG_RE);
    lua_setglobal(L, "RE");
    lua_pushinteger(L, CFG_WRITE);
    lua_setglobal(L, "WRITE");
    lua_pushinteger(L, CFG_CHANGE);
    lua_setglobal(L, "CHAGNE");
    lua_pushinteger(L, CFG_ANY_WRITE);
    lua_setglobal(L, "ANY_WRITE");
    lua_pushinteger(L, CFG_ANY_CHANGE);
    lua_setglobal(L, "ANY_CHANGE");
    lua_pushinteger(L, CFG_ALL_WRITE);
    lua_setglobal(L, "ALL_WRITE");
    
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "broker_ip", "broker-ip", 'b', flags, "127.0.0.1");
    result += dax_add_attribute(ds, "broker_port", "broker-port", 'p', flags, "1883");
    result += dax_add_attribute(ds, "broker_timeout", "broker-timeout", 't', flags, "10");
    result += dax_add_attribute(ds, "clientid", "clientid", 'i', flags, "OpenDAX MQTT Client");
    result += dax_add_attribute(ds, "username", "username", 'u', flags, NULL);
    result += dax_add_attribute(ds, "password", "password", 'w', flags, NULL);
    if(result) {
        dax_fatal(ds, "Problem setting attributes");
    }
        
    dax_set_luafunction(ds, (void *)_add_pub, "add_pub");
    dax_set_luafunction(ds, (void *)_add_sub, "add_sub");

    dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    //dax_free_config(ds);

    return 0;
}

/* iterator that returns each subscriber in turn.  Returns
 * NULL when there are no more. */
subscriber_t *
get_sub_iter(void) {
    static int idx;
    if(idx >= subscriber_count) {
        idx = 0;
        return NULL;
    } else {
        return &subscribers[idx++];
    }
}

/* return a pointer to the subscription given by topic */
// TODO This function just does a linear search through the array.
//      woudl be better as a sorted array and bisection search or something
subscriber_t *
get_sub(char *topic) {
    int n;
    for(n=0;n<subscriber_count;n++) {
        if(! strcmp(topic, subscribers[n].topic)) {
            return &subscribers[n];
        }
    }
    return NULL;
}
