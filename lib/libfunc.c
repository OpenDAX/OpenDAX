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
#include <stdarg.h>

static int _verbosity = 0;

/* These functions handle the logging and error callback function */

/* Callback functions. */
static void (*_dax_debug)(const char *output) = NULL;
static void (*_dax_error)(const char *output) = NULL;

/* Set the verbosity level for the debug callback */
void dax_set_level(int level) {
    /* bounds check */
    if(level < 0) _verbosity = 0;
    else if(_verbosity > 10) _verbosity = 10;
    else _verbosity = level;
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

#ifndef DEBUG_STRING_SIZE
  #define DEBUG_STRING_SIZE 80
#endif

/* Function for use inside the library for sending debug messages.
   If the level is lower than the global _verbosity level then it will
   print the message.  Otherwise do nothing */
void dax_debug(int level, const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    /* check if the callback has been set and _verbosity is set high enough */
    if(_dax_debug && level <= _verbosity) {
        va_list val;
        va_start(val,format);
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        va_end(val);
        _dax_debug(output);
    }
}

/* Prints an error message if the callback has been set */
void dax_error(const char *format, ...) {
    char output[DEBUG_STRING_SIZE];
    /* Check that the callback has been set */
    if(_dax_error) {
        va_list val;
        va_start(val,format);
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        va_end(val);
        _dax_error(format);
    }
}

/* Returns a pointer to a string that is the name of the datatype */
const char *dax_get_type(int type) {
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