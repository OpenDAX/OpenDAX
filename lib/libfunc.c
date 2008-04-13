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

 * This file contains some of the misc functions for the library
 */

#include <libdax.h>
#include <common.h>
#include <stdarg.h>
#include <strings.h>
#include <signal.h>

static int _verbosity = 0;
static int _logflags = 0;

/* These functions handle the logging and error callback function */

/* Callback functions. */
static void (*_dax_debug)(const char *output) = NULL;
static void (*_dax_error)(const char *output) = NULL;
static void (*_dax_log)(const char *output) = NULL;

/* Set the verbosity level for the debug callback */
void dax_set_verbosity(int level) {
    /* bounds check */
    if(level < 0) _verbosity = 0;
    else if(level > 10) _verbosity = 10;
    else _verbosity = level;
}

void dax_set_debug_topic(u_int32_t topic) {
    _logflags = topic;
    dax_debug(LOG_MAJOR, "Log Topics Set to %d", _logflags);
}

/* Function for modules to set the debug message callback */
int dax_set_debug(void (*debug)(const char *format)) {
    _dax_debug = debug;
    return (int)_dax_debug;
}

/* Function for modules to set the error message callback */
int dax_set_error(void (*error)(const char *format)) {
    _dax_error = error;
    return (int)_dax_error;
}

/* Function for modules to override the dax_log function */
int dax_set_log(void (*log)(const char *format)) {
    _dax_log = log;
    return (int)_dax_log;
}

/* TODO: Make these function allocate the memory at run time so that
   we don't have this limitation */
#ifndef DEBUG_STRING_SIZE
  #define DEBUG_STRING_SIZE 80
#endif

/* Function for use inside the library for sending debug messages.
   If the level is lower than the global _verbosity level then it will
   print the message.  Otherwise do nothing */
void dax_debug(int level, const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    va_list val;
    
    /* check if the callback has been set and _verbosity is set high enough */
    if(level & _logflags) {
        va_start(val, format);
        if(_dax_debug) {
            vsnprintf(output, DEBUG_STRING_SIZE, format, val);
            _dax_debug(output);
        } else {
            vprintf(format, val);
            printf("\n");
        }
        va_end(val);
    }
}

/* Prints an error message if the callback has been set otherwise
   sends the string to stderr. */
void dax_error(const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    va_list val;
    va_start(val, format);
    
    /* Check whether the callback has been set */
    if(_dax_error) {
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        _dax_error(format);
    } else {
        vfprintf(stderr, format, val);
        printf("\n");
    }
    va_end(val);
}

/* This function would be for logging events within the module */
/* TODO: At some point this may send a message to opendax right
 now it either calls the callback or if none is given prints to stdout */
void dax_log(const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    va_list val;
    va_start(val, format);
    
    if(_dax_log) {
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        _dax_log(format);
    } else {
        vprintf(format, val);
        printf("\n");
    }
    va_end(val);
}

void dax_fatal(const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    va_list val;
    va_start(val, format);
    
    /* Check whether the callback has been set */
    if(_dax_error) {
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        _dax_error(format);
    } else {
        vfprintf(stderr, format, val);
    }
    va_end(val);
    kill(getpid(), SIGQUIT);
}


/* Returns a pointer to a string that is the name of the datatype */
int dax_string_to_type(const char *type) {
    if(!strcasecmp(type, "BOOL"))  return DAX_BOOL;
    if(!strcasecmp(type, "BYTE"))  return DAX_BYTE;
    if(!strcasecmp(type, "SINT"))  return DAX_SINT;
    if(!strcasecmp(type, "WORD"))  return DAX_WORD;
    if(!strcasecmp(type, "INT"))   return DAX_INT;
    if(!strcasecmp(type, "UINT"))  return DAX_UINT;
    if(!strcasecmp(type, "DWORD")) return DAX_DWORD;
    if(!strcasecmp(type, "DINT"))  return DAX_DINT;
    if(!strcasecmp(type, "UDINT")) return DAX_UDINT;
    if(!strcasecmp(type, "TIME"))  return DAX_TIME;
    if(!strcasecmp(type, "REAL"))  return DAX_REAL;
    if(!strcasecmp(type, "LWORD")) return DAX_LWORD;
    if(!strcasecmp(type, "LINT"))  return DAX_LINT;
    if(!strcasecmp(type, "ULINT")) return DAX_ULINT;
    if(!strcasecmp(type, "LREAL")) return DAX_LREAL;
    return -1;
}

/* Returns a pointer to a string that is the name of the datatype */
const char *dax_type_to_string(int type) {
    switch (type) {
        case DAX_BOOL:
            return "BOOL";
        case DAX_BYTE:
            return "BYTE";
        case DAX_SINT:
            return "SINT";
        case DAX_WORD:
            return "WORD";
        case DAX_INT:
            return "INT";
        case DAX_UINT:
            return "UINT";
        case DAX_DWORD:
            return "DWORD";
        case DAX_DINT:
            return "DINT";
        case DAX_UDINT:
            return "UDINT";
        case DAX_TIME:
            return "TIME";
        case DAX_REAL:
            return "REAL";
        case DAX_LWORD:
            return "LWORD";
        case DAX_LINT:
            return "LINT";
        case DAX_ULINT:
            return "ULINT";
        case DAX_LREAL:
            return "LREAL";
        default:
            return NULL;
    }
}