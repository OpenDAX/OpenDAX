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
            dax_log(LOG_FATAL, "Cannot allocate memory for the subscription list");
            kill(getpid(), SIGQUIT);
        }
        subscriber_size = SUB_START_SIZE;
    } else if(subscriber_count == subscriber_size) {
        ns = realloc(subscribers, sizeof(subscriber_t) * (subscriber_size * 2));
        if(ns != NULL) {
            subscribers = ns;
        } else {
            dax_log(LOG_ERROR, "Failure to allocate additional subscriptions");
            return -1;
        }
    }
    n = subscriber_count;
    subscriber_count++;
    /* Initialize the script structure */
    subscribers[n].enabled = ENABLE_UNINIT;
    subscribers[n].filter = 0;
    subscribers[n].tag_count = 0;
    subscribers[n].tagnames = NULL;
    subscribers[n].qos = 0;
    subscribers[n].h = NULL;
    subscribers[n].topic = NULL;
    subscribers[n].group = NULL;
    subscribers[n].buff = NULL;
    subscribers[n].buff_size = 0;
    return n;
}


static int
_add_sub(lua_State *L) {
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

    /* Retrieve tags array from table */
    if(lua_getfield(L, 1, "tags") != LUA_TNIL) {
        if(lua_istable(L, -1)) {
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
        } else if(lua_isstring(L, -1)) {
            s = lua_tostring(L, -1);
            subscribers[idx].tagnames = malloc(sizeof(char *));
            if(subscribers[idx].tagnames == NULL) {
                luaL_error(L, "Unable to allocate space for tagnames");
            }
            subscribers[idx].tagnames[0] = strdup(s);
            subscribers[idx].tag_count = 1;
        } else {
            luaL_error(L, "At least one tagname is required for subscription");
        }
    }
    lua_pop(L, 1); /* Pop off tags */

    if(lua_getfield(L, 1, "filter") != LUA_TNIL) {
        if(! lua_isfunction(L, -1)) {
            luaL_error(L, "filter should be a function ");
        }
        /* Pop the function off the stack and write it to the regsitry and assign
           the reference to .filter */
        subscribers[idx].filter = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1); /* Pop nil */
    }

    if(lua_getfield(L, 1, "qos") != LUA_TNIL) {
        subscribers[idx].qos = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); /* Pop off QOS */

    return 0;
}


/* This function returns an index into the publishers[] array for
   the next unassigned publisher */
static int
_get_new_pub(void)
{
    void *ns;
    int n;

    /* Allocate the script array if it has not already been done */
    if(publisher_count == 0) {
        publishers = malloc(sizeof(publisher_t) * PUB_START_SIZE);
        if(publishers == NULL) {
            dax_log(LOG_FATAL, "Cannot allocate memory for the subscription list");
        }
        publisher_size = PUB_START_SIZE;
    } else if(publisher_count == publisher_size) {
        ns = realloc(publishers, sizeof(publisher_t) * (publisher_size * 2));
        if(ns != NULL) {
            publishers = ns;
        } else {
            dax_log(LOG_ERROR, "Failure to allocate additional publishers");
            return -1;
        }
    }
    n = publisher_count;
    publisher_count++;
    /* Initialize the script structure */
    publishers[n].enabled = ENABLE_UNINIT;
    publishers[n].filter = 0;
    publishers[n].tag_count = 0;
    publishers[n].tagnames = NULL;
    publishers[n].qos = 0;
    publishers[n].h = NULL;
    publishers[n].topic = NULL;
    publishers[n].retained = 0;
    publishers[n].trigger_tag = NULL;
    publishers[n].trigger_type = EVENT_WRITE;
    publishers[n].trigger_value = "0";
    publishers[n].group = NULL;
    publishers[n].buff = NULL;
    publishers[n].buff_size = 0;
    return n;
}


static int
_add_pub(lua_State *L)
{
    int  idx;
    int i, len;
    const char *s;

    if(! lua_istable(L, 1) ) {
        luaL_error(L, "add_pub() argument is not a table");
    }
    idx = _get_new_pub();
    if(idx < 0) {
        luaL_error(L, "Unable to allocate new publisher");
    }

    lua_getfield(L, 1, "topic");
    if( lua_isnil(L, -1)) {
        luaL_error(L, "Topic is required for publisher");
    } else {
        s = lua_tostring(L, -1);
        publishers[idx].topic = strdup(s);
    }
    lua_pop(L, 1);

    if(lua_getfield(L, 1, "retained") != LUA_TNIL) {
        if(lua_toboolean(L, -1)) {
            publishers[idx].retained = 1;
        }
    }
    lua_pop(L, 1);

    /* Retrieve tags array from table */
    if(lua_getfield(L, 1, "tags") != LUA_TNIL) {
        if(lua_istable(L, -1)) {
            lua_len(L, -1);
            len = lua_tointeger(L, -1);
            lua_pop(L, 1);
            publishers[idx].tagnames = malloc(sizeof(char *) * len);
            if(publishers[idx].tagnames == NULL) {
                luaL_error(L, "Unable to allocate space for tagnames");
            }

            for(i = 0; i < len; i++) {
                lua_geti(L, -1, i+1);
                s = lua_tostring(L, -1);
                lua_pop(L, 1);
                publishers[idx].tagnames[i] = strdup(s);
            }
            publishers[idx].tag_count = len;
        } else if(lua_isstring(L, -1)) {
            s = lua_tostring(L, -1);
            publishers[idx].tagnames = malloc(sizeof(char *));
            if(publishers[idx].tagnames == NULL) {
                luaL_error(L, "Unable to allocate space for tagnames");
            }
            publishers[idx].tagnames[0] = strdup(s);
            publishers[idx].tag_count = 1;
        } else {
            luaL_error(L, "At least one tagname is required for subscription");
        }
    }
    lua_pop(L, 1); /* Pop off tags */

    if(lua_getfield(L, 1, "filter") != LUA_TNIL) {
        if(! lua_isfunction(L, -1)) {
            luaL_error(L, "Filter should be a function");
        }
        /* Pop the function off the stack and write it to the regsitry and assign
           the reference to .filter */
        publishers[idx].filter = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1); /* Pop nil */
    }

    if(lua_getfield(L, 1, "qos") != LUA_TNIL) {
        publishers[idx].qos = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); /* Pop off QOS */

    lua_getfield(L, 1, "trigger_tag");
    if(! lua_isnil(L, -1)) {
        s = lua_tostring(L, -1);
        publishers[idx].trigger_tag = strdup(s);
        if(publishers[idx].trigger_tag == NULL) {
            luaL_error(L, "Unable to allocate space for format string");
        }
    } else {
        if(publishers[idx].tag_count > 0) {
            /* If we don't have a trigger tag then we'll use the first
             * tagname given */
            publishers[idx].trigger_tag = publishers[idx].tagnames[0];
        } else {
            luaL_error(L, "A trigger tag must be set");
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "trigger_type");
    if(! lua_isnil(L, -1)) {
         publishers[idx].trigger_type = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);



    lua_getfield(L, 1, "trigger_value");
    if(! lua_isnil(L, -1)) {
        publishers[idx].trigger_value = strdup(lua_tostring(L, -1));

    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "qos");
    if(! lua_isnil(L, -1)) {
        publishers[idx].qos = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); /* Pop off tagnmaes */

    return 0;
}


/* Public function to initialize the module */
int
configure(int argc, char *argv[])
{
    int flags, result = 0;
    lua_State *L;

    /* Set some globals for the  configuration script to use. */
    /* Minor problem here is that the script could change these */
    L = dax_get_luastate(ds);
    lua_pushinteger(L, EVENT_WRITE);
    lua_setglobal(L, "WRITE");
    lua_pushinteger(L, EVENT_CHANGE);
    lua_setglobal(L, "CHANGE");
    lua_pushinteger(L, EVENT_EQUAL);
    lua_setglobal(L, "EQUAL");
    lua_pushinteger(L, EVENT_SET);
    lua_setglobal(L, "SET");
    lua_pushinteger(L, EVENT_RESET);
    lua_setglobal(L, "RESET");
    lua_pushinteger(L, EVENT_GREATER);
    lua_setglobal(L, "GREATER");
    lua_pushinteger(L, EVENT_LESS);
    lua_setglobal(L, "LESS");
    lua_pushinteger(L, EVENT_DEADBAND);
    lua_setglobal(L, "DEADBAND");


    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "broker_ip", "broker-ip", 'b', flags, "127.0.0.1");
    result += dax_add_attribute(ds, "broker_port", "broker-port", 'p', flags, "1883");
    result += dax_add_attribute(ds, "broker_timeout", "broker-timeout", 't', flags, "10");
    result += dax_add_attribute(ds, "clientid", "clientid", 'i', flags, "OpenDAX MQTT Client");
    result += dax_add_attribute(ds, "username", "username", 'u', flags, NULL);
    result += dax_add_attribute(ds, "password", "password", 'w', flags, NULL);
    if(result) {
        dax_log(LOG_FATAL, "Problem setting attributes");
    }

    dax_set_luafunction(ds, (void *)_add_pub, "add_pub");
    dax_set_luafunction(ds, (void *)_add_sub, "add_sub");

    result = dax_configure(ds, argc, (char **)argv, CFG_CMDLINE | CFG_MODCONF);

    return result;
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

/* This function is used to compare the topics that we are subscribed to
   with the topic that is received in a message.  It takes into account
   the wildcard characters '+' and '#' */
static int
_topic_compare(char *subscription, char *received) {
    char cs, cr;
    int i=0, j=0;

    while(1) {
        cs = subscription[i];
        cr = received[j];
        if(cs == '+') {
            i++; /* skip the '+' */
            /* Now we need to roll past the next sub topic to either the next '/'
               or the end of the string */
            while(cr != '/' && cr != '\0') {
                cr = received[++j];
            }
        } else if(cs != cr) {
            return -1; /* No match */
        }
        if(cs == '\0' && cr == '\0') {
            return 0; /* We have a winner */
        }
        if(cs == '/' && subscription[i+1] == '#') {
            return 0; /* Wildcard character so the rest doesn't matter */
        }
        /* else if they are the same then continue on */
        i++;
        j++;
    }

}

/* return a pointer to the subscription that matches the topic */
// TODO This function just does a linear search through the array.
//      would be better as a sorted array and bisection search or something
// TODO Right now we only match the first subscription so we can only
//      have one subscription per topic match.  Does this matter?
subscriber_t *
get_sub(char *topic) {
    int n;
    for(n=0;n<subscriber_count;n++) {
        if(subscribers[n].enabled == ENABLE_GOOD) { /* We only care if this sub is good */
            if(_topic_compare(subscribers[n].topic, topic) == 0) {
                return &subscribers[n];
            }
        }
    }
    return NULL;
}


/* iterator that returns each subscriber in turn.  Returns
 * NULL when there are no more. */
publisher_t *
get_pub_iter(void) {
    static int idx;
    if(idx >= publisher_count) {
        idx = 0;
        return NULL;
    } else {
        return &publishers[idx++];
    }
}

/* return a pointer to the publisher given by topic */
// TODO This function just does a linear search through the array.
//      would be better as a sorted array and bisection search or something
publisher_t *
get_pub(char *topic) {
    int n;
    for(n=0;n<publisher_count;n++) {
        if(! strcmp(topic, publishers[n].topic)) {
            return &publishers[n];
        }
    }
    return NULL;
}
