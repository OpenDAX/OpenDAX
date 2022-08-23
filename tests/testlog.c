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

static uint32_t _log_mask = LOG_ALL;
static void (*_topic_callback)(char *topic);

int
dax_init_logger(char *name) {
    DF("yep");
    return 0;
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
        if(strcmp(s,"ALL") == 0) _log_mask |= LOG_ALL;
        if(strcmp(s,"ERROR") == 0) _log_mask |= LOG_ERROR;
        if(strcmp(s,"FATAL") == 0) _log_mask |= LOG_FATAL;
        if(strcmp(s,"MAJOR") == 0) _log_mask |= LOG_MAJOR;
        if(strcmp(s,"MINOR") == 0) _log_mask |= LOG_MINOR;
        if(strcmp(s,"COMM") == 0) _log_mask |= LOG_COMM;
        if(strcmp(s,"MSG") == 0) _log_mask |= LOG_MSG;
        if(strcmp(s,"MSGERR") == 0) _log_mask |= LOG_MSGERR;
        if(strcmp(s,"CONFIG") == 0) _log_mask |= LOG_CONFIG;
        if(strcmp(s,"MODULE") == 0) _log_mask |= LOG_MODULE;
        
        if(_topic_callback != NULL) { /* If there is a callback function assigned */
            _topic_callback(s);       /* Call it */
        }
        s = strtok(NULL, ",");
    }
    free(temp);
    return 0;
}

void
dax_set_log_mask(uint32_t mask) {
    _log_mask = mask;
}

uint32_t
dax_get_log_mask(void) {
    return _log_mask;
}

/* This assigns a callback function that if set, will be called for each logging topic
 * that is assigned during the configuration */
int
dax_log_topic_callback(void (*topic_callback)(char *topic)) {
    _topic_callback = topic_callback;
    return 0;
}

void
dax_log(int mask, const char *format, ...)
{
    char output[LOG_STRING_SIZE];
    va_list val;

    /* check if the callback has been set and _verbosity is set high enough */
    if(mask & _log_mask) {
        va_start(val, format);
            vprintf(format, val);
            printf("\n");
        va_end(val);
    }
}
