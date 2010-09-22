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
#include <signal.h>

//static int _verbosity = 0;
//static int _logflags = 0;

/* These functions handle the logging and error callback function */

/* Callback functions. */
static void (*_dax_debug)(const char *output) = NULL;
static void (*_dax_error)(const char *output) = NULL;
static void (*_dax_log)(const char *output) = NULL;

/* Set the verbosity level for the debug callback */
//void
//dax_set_verbosity(int level)
//{
//    /* bounds check */
//    if(level < 0) _verbosity = 0;
//    else if(level > 10) _verbosity = 10;
//    else _verbosity = level;
//}

void
dax_set_debug_topic(dax_state *ds, u_int32_t topic)
{
    ds->logflags = topic;
    dax_debug(ds, LOG_MAJOR, "Log Topics Set to %d", ds->logflags);
}

/* Function for modules to set the debug message callback */
void
dax_set_debug(dax_state *ds, void (*debug)(const char *format))
{
    _dax_debug = debug;
//--    return (long)_dax_debug;
}

/* Function for modules to set the error message callback */
void
dax_set_error(dax_state *ds, void (*error)(const char *format))
{
    _dax_error = error;
//--    return (int)_dax_error;
}

/* Function for modules to override the dax_log function */
void
dax_set_log(dax_state *ds, void (*log)(const char *format))
{
    _dax_log = log;
//--    return (int)_dax_log;
}

/* TODO: Make these function allocate the memory at run time so that
 * we don't have this limitation */
#ifndef DEBUG_STRING_SIZE
  #define DEBUG_STRING_SIZE 80
#endif

/* Function for use inside the library for sending debug messages.
 * If the level is lower than the global _verbosity level then it will
 * print the message.  Otherwise do nothing */
void
dax_debug(dax_state *ds, int level, const char *format, ...)
{
    char output[DEBUG_STRING_SIZE];
    va_list val;
    
    /* check if the callback has been set and _verbosity is set high enough */
    if(level & ds->logflags) {
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
void
dax_error(dax_state *ds, const char *format, ...)
{
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
void
dax_log(dax_state *ds, const char *format, ...)
{
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

void
dax_fatal(dax_state *ds, const char *format, ...)
{
    char output[DEBUG_STRING_SIZE];
    va_list val;
    va_start(val, format);
    
    /* Check whether the callback has been set */
    if(_dax_error) {
        vsnprintf(output, DEBUG_STRING_SIZE, format, val);
        _dax_error(format);
    } else {
        vfprintf(stderr, format, val);
        fprintf(stderr, "\n");
    }
    va_end(val);
    kill(getpid(), SIGQUIT);
}

#ifdef USE_PTHREAD_LOCK

inline int
libdax_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_lock(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

inline int
libdax_unlock(dax_lock *lock) {
    int result;
    result = pthread_mutex_unlock(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

inline int
libdax_init_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_init(lock, NULL);
    if(result) return ERR_GENERIC;
    return 0;
}

inline int
libdax_destroy_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_destroy(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

/* If not locking mechanisms are #defined then we just set the functions
 * to return nothing and hope that the optimizer will eliminate them */
#else

inline int
libdax_lock(dax_lock *lock) {
    return 0;
}

inline int
libdax_unlock(dax_lock *lock) {
    return 0;
}

inline int
libdax_init_lock(dax_lock *lock) {
    return 0;
}

inline int
libdax_destroy_lock(dax_lock *lock) {
    return 0;
}

#endif


/* This function takes the name argument and figures out the text part and puts
 that in 'tagname' then it sees if there is an index in [] and puts that in *index.
 The calling function should make sure that *tagname is big enough */
/* TODO: This will probably be removed */
//int
//dax_tag_parse(char *name, char *tagname, int *index)
//{
//    int n = 0;
//    int i = 0;
//    int tagend = 0;
//    char test[10];
//    *index = -1;
//    
//    while(name[n] != '\0') {
//        if(name[n] == '[') {
//            tagend = n++;
//            /* figure the tagindex here */
//            while(name[n] != ']') {
//                if(name[n] == '\0') return -1; /* Gotta get to a ']' before the end */
//                test[i++] = name[n++];
//                if(i == 9) return -1; /* Number is too long */
//            }
//            test[i] = '\0';
//            *index = (int)strtol(test, NULL, 10);
//            n++;
//        }
//        n++;        
//    }
//    if(tagend) {
//        strncpy(tagname, name, tagend);
//        tagname[tagend] = '\0';
//    } else {
//        strcpy(tagname, name);
//    }
//    return 0;
//}
