/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2022 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * This is main source file for the message logging library
 */

#include "daxlog.h"
#include <common.h>
#include <config.h>
#include <stdarg.h>


#define LOG_STRING_SIZE 256
#define SERVICE_LIST_SIZE 32

/* This is the default log topic mask.  This is used if no other
 * logging services are configured. */
static uint32_t _default_topics = LOG_FATAL | LOG_ERROR;
static const char *_name;

static void (*_topic_callback)(char *topic);
static service_item _services[SERVICE_LIST_SIZE];
static int _service_size = SERVICE_LIST_SIZE;
static int _service_count = 0;

static char *
_topic_to_string(uint32_t topic) {
    switch(topic) {
        case LOG_MINOR:
            return "MINOR";
        case LOG_MAJOR:
            return "MAJOR";
        case LOG_WARN:
            return "WARN";
        case LOG_ERROR:
            return "ERROR";
        case LOG_FATAL:
            return "FATAL";
        case LOG_MODULE:
            return "MODULE";
        case LOG_COMM:
            return "COMM";
        case LOG_MSG:
            return "MSG";
        case LOG_MSGERR:
            return "MSGERR";
        case LOG_CONFIG:
            return "CONFIG";
        case LOG_PROTOCOL:
            return "PROTOCOL";
        case LOG_INFO:
            return "INFO";
        case LOG_DEBUG:
            return "DEBUG";
        case LOG_LOGIC:
            return "LOGIC";
        case LOG_LOGICERR:
            return "LOGICERR";
        default:
            return "?";
    }
}

/* Every logging service requires at least two functions.  One that is called
 * during configuration and the other that actually does the logging. Each
 * service also has a structure type that would be stored in the array to
 * maintain configuration for that particular instance of the service. */

/* These are for the stdio logging service.*/
static void
_log_stdio(uint32_t topic, void *data, char *str) {
    stdio_service *config;
    config = (stdio_service *)data;

    fprintf(config->file, "[%s] %s: %s\n",_name, _topic_to_string(topic), str);
}

static int
_add_stdio_service(lua_State *L, service_item *service) {
    stdio_service *new;
    const char *file;

    new = malloc(sizeof(stdio_service));
    lua_getfield(L, -1, "file");
    file = lua_tostring(L, -1);
    if(strcasecmp(file, "stdout")==0) {
        new->file = stdout;   
    } else if(strcasecmp(file, "stderr")==0) {
        new->file = stderr;
    }
    service->log_func = _log_stdio;
    service->data=(void *)new;

    return 0;
}

/* These are for the syslog logging service */
static void
_log_syslog(uint32_t topic, void *data, char *str) {
    syslog_service *config;
    config = (syslog_service *)data;

    syslog(LOG_USER | config->priority, "%s", str);
}

static int
_add_syslog_service(lua_State *L, service_item *service) {
    syslog_service *new;
    const char *priority;
    
    new = malloc(sizeof(syslog_service));
    lua_getfield(L, -1, "priority");
    priority = lua_tostring(L, -1);
    
    if(strcasecmp(priority, "EMERG") == 0)
        new->priority = SYSLOG_EMERG;
    else if(strcasecmp(priority, "ALERT") == 0)
        new->priority = SYSLOG_ALERT;
    else if(strcasecmp(priority, "CRIT") == 0)
        new->priority = SYSLOG_CRIT;
    else if(strcasecmp(priority, "ERR") == 0)
        new->priority = SYSLOG_ERR;
    else if(strcasecmp(priority, "WARNING") == 0)
        new->priority = SYSLOG_WARNING;
    else if(strcasecmp(priority, "NOTICE") == 0)
        new->priority = SYSLOG_NOTICE;
    else if(strcasecmp(priority, "INFO") == 0)
        new->priority = SYSLOG_INFO;
    else if(strcasecmp(priority, "DEBUG") == 0)
        new->priority = SYSLOG_DEBUG;
    else
        luaL_error(L, "Unknown priority given");

    service->log_func = _log_syslog;
    service->data=(void *)new;

    return 0;
}


/* Lua interface function for adding a port.  It takes a single
   table as an argument and returns the Port's index */
static int
_add_service(lua_State *L)
{   
    const char *type;
    const char *topics;
    uint32_t mask;
    int result;

    /* Check that we have services available in the list */
    /* TODO: Reallocate the array instead of static */
    if(_service_count >= _service_size) {
        dax_log(LOG_ERROR, "No more log services available");
        return ERR_OVERFLOW;
    }
    if(!lua_istable(L, -1)) {
        luaL_error(L, "add_port() received an argument that is not a table");
    }
    
    lua_getfield(L, -1, "topics");
    topics = lua_tostring(L, -1);
    if(topics == NULL) {
        dax_log(LOG_ERROR, "No Topic list given");
        return ERR_ARG;
    }
    mask = dax_parse_log_topics((char *)topics);
    lua_pop(L, 1);

    lua_getfield(L, -1, "type");
    type = lua_tostring(L, -1);
    if(strcasecmp(type, "stdio")==0) {
        lua_pop(L, 1); /* Put the table back at the top of the stack */
        result = _add_stdio_service(L, &_services[_service_count]);
    } else if(strcasecmp(type, "syslog")==0) {
        lua_pop(L, 1); /* Put the table back at the top of the stack */
        result = _add_syslog_service(L, &_services[_service_count]);
    } else {
        dax_log(LOG_ERROR, "Unknown log service %s", type);
        return ERR_ARG;
    }
    if(result) {
        return result; /* Return the result of the _add_xx_service function */
    }
    _services[_service_count].mask = mask;
    _service_count++;
    return 0;
}

/* Set the name and the default logger topics for the logger */
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

/* Parse a comma delimited string and return a logging topics bit mask */
uint32_t
dax_parse_log_topics(char *topic_string) {
    char *temp, *s;
    uint32_t log_mask = 0;

    if(topic_string == NULL) {
        return 0;
    }
    temp = strdup(topic_string);

    s = strtok(temp, ",");
    while(s != NULL) {
        if(strcmp(s,"ALL") == 0) log_mask |= LOG_ALL;
        if(strcmp(s,"MINOR") == 0) log_mask |= LOG_MINOR;
        if(strcmp(s,"MAJOR") == 0) log_mask |= LOG_MAJOR;
        if(strcmp(s,"WARN") == 0) log_mask |= LOG_WARN;
        if(strcmp(s,"ERROR") == 0) log_mask |= LOG_ERROR;
        if(strcmp(s,"FATAL") == 0) log_mask |= LOG_FATAL;
        if(strcmp(s,"MODULE") == 0) log_mask |= LOG_MODULE;
        if(strcmp(s,"COMM") == 0) log_mask |= LOG_COMM;
        if(strcmp(s,"MSG") == 0) log_mask |= LOG_MSG;
        if(strcmp(s,"MSGERR") == 0) log_mask |= LOG_MSGERR;
        if(strcmp(s,"CONFIG") == 0) log_mask |= LOG_CONFIG;
        if(strcmp(s,"PROTOCOL") == 0) log_mask |= LOG_PROTOCOL;
        if(strcmp(s,"INFO") == 0) log_mask |= LOG_INFO;
        if(strcmp(s,"DEBUG") == 0) log_mask |= LOG_DEBUG;
        
        if(_topic_callback != NULL) { /* If there is a callback function assigned */
            _topic_callback(s);       /* Call it */
        }
        s = strtok(NULL, ",");
    }
    free(temp);
    return log_mask;
}

/* This assigns a callback function that if set, will be called for each logging topic
 * that is assigned during the configuration */
// int
// dax_log_topic_callback(void (*topic_callback)(char *topic)) {
//     _topic_callback = topic_callback;
//     return 0;
// }

/* Sets the default bit mask directly */
void
dax_log_set_default_mask(uint32_t mask) {
    _default_topics = mask;
}

/* Takes a comma delimited string and usees that
 * to set the log topic bit mask */
void
dax_log_set_default_topics(char *topics) {
    _default_topics = dax_parse_log_topics(topics);
}


/* Function for sending log messages.  _log_mask is a bit mask
   that is used to decide whether this message should actually get
   logged */
void
dax_log(uint32_t topic, const char *format, ...)
{
    char output[LOG_STRING_SIZE];
    va_list val;
    
    /* Format the string */
    va_start(val, format);
    vsnprintf(output, LOG_STRING_SIZE, format, val);
    va_end(val);
    if(_service_count == 0 && _default_topics & topic) {
        printf("[%s] %s: %s\n",_name, _topic_to_string(topic), output);
    } else {
        /* Loop through the services and call the function if
           we have a match */
        for(int n = 0;n < _service_count;n++) {
            if(_services[n].mask & topic) {
                _services[n].log_func(topic, _services[n].data, output);
            }
        }   
    }
}
