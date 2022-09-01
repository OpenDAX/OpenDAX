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
 *  Test logging file
 */

/* Since the logging facility is fairly complex and we don't really need all of
   that functionality for testing we have duplicated those functions here that
   are needed by most of the system.  These simplify the logging system and 
   simply log everythign to stdout. */

#include <stdio.h>
#include <stdarg.h>
#include <common.h>
#include <opendax.h>

#define LOG_STRING_SIZE 256

static uint32_t _default_topics = LOG_ALL;
static const char *_name;

static void (*_topic_callback)(char *topic);


int
dax_init_logger(const char *name, uint32_t topics) {
    /* The name may be used for some loggin services. */
    if(name != NULL) {
        _name = name;
    }
    if(topics) {
        _default_topics = topics;
    }
    return 0;
}

static int
_add_service(lua_State *L) {
    return 0;
}

/* Dummys to make the below function happy so we don't have to link in Lua */
void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n) {
    return;
}

void lua_setglobal(lua_State *L, const char *name) {
    return;
}

/* Set the add_log_service() function for the configuration Lua state */
int
dax_log_set_lua_function(lua_State *L) {
    if(L != NULL) {
        lua_pushcfunction(L, _add_service);
        lua_setglobal(L, "add_log_service");
        return 0;
    } else {
        return ERR_ARG;
    }
}

/* This parses the string in the 'topic_string' sets
 * the appropriate log flags and calls the callback function
 * if it exists */
uint32_t
dax_parse_log_topics(char *topic_string) {
    char *temp, *s;

    if(topic_string == NULL) {
        return 0;
    }
    temp = strdup(topic_string);

    s = strtok(temp, ",");
    while(s != NULL) {
        if(strcmp(s,"ALL") == 0) _default_topics |= LOG_ALL;
        if(strcmp(s,"ERROR") == 0) _default_topics |= LOG_ERROR;
        if(strcmp(s,"FATAL") == 0) _default_topics |= LOG_FATAL;
        if(strcmp(s,"MAJOR") == 0) _default_topics |= LOG_MAJOR;
        if(strcmp(s,"MINOR") == 0) _default_topics |= LOG_MINOR;
        if(strcmp(s,"COMM") == 0) _default_topics |= LOG_COMM;
        if(strcmp(s,"MSG") == 0) _default_topics |= LOG_MSG;
        if(strcmp(s,"MSGERR") == 0) _default_topics |= LOG_MSGERR;
        if(strcmp(s,"CONFIG") == 0) _default_topics |= LOG_CONFIG;
        if(strcmp(s,"MODULE") == 0) _default_topics |= LOG_MODULE;
        
        if(_topic_callback != NULL) { /* If there is a callback function assigned */
            _topic_callback(s);       /* Call it */
        }
        s = strtok(NULL, ",");
    }
    free(temp);
    return 0;
}

void
dax_log_set_default_mask(uint32_t topics) {
    _default_topics = topics;
}

/* Takes a comma delimited string and usees that
 * to set the log topic bit mask */
void
dax_log_set_default_topics(char *topics) {
    _default_topics = dax_parse_log_topics(topics);
}

uint32_t
dax_get_log_mask(void) {
    return _default_topics;
}

/* This assigns a callback function that if set, will be called for each logging topic
 * that is assigned during the configuration */
int
dax_log_topic_callback(void (*topic_callback)(char *topic)) {
    _topic_callback = topic_callback;
    return 0;
}

void
dax_log(uint32_t mask, const char *format, ...)
{
    char output[LOG_STRING_SIZE];
    va_list val;

    /* check if the callback has been set and _verbosity is set high enough */
    if(mask & _default_topics) {
        va_start(val, format);
            vprintf(format, val);
            printf("\n");
        va_end(val);
    }
}
