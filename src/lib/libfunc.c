/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
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

 * This file contains some of the misc functions for the library
 */

#include <libdax.h>
#include <common.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

/* These functions handle the logging and error callback function */

/* Callback functions. */
static void (*_dax_debug)(const char *output) = NULL;
static void (*_dax_error)(const char *output) = NULL;
static void (*_dax_log)(const char *output) = NULL;

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
}

/* Function for modules to set the error message callback */
void
dax_set_error(dax_state *ds, void (*error)(const char *format))
{
    _dax_error = error;
}

/* Function for modules to override the dax_log function */
void
dax_set_log(dax_state *ds, void (*log)(const char *format))
{
    _dax_log = log;
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

/* The following two functions are utility functions for converting dax
   values to strings and strings to dax values.

 * Used to printf a dax tag value. Size is the size of buff to keep from being
 * overflowed.  The index indicates where in val we'll look for the data. */
int
dax_val_to_string(char *buff, int size, tag_type type, void *val, int index)
{
    int ms;
    long int sec;
    struct tm *tm;
    char tstr[16];
    
    switch (type) {
     /* Each number has to be cast to the right datatype then dereferenced */
        case DAX_BOOL:
            if((0x01 << (index % 8)) & ((u_int8_t *)val)[index / 8]) {
                snprintf(buff, size, "1");
            } else {
                snprintf(buff, size, "0");
            }
            break;
        case DAX_BYTE:
            snprintf(buff, size, "%u", ((dax_byte *)val)[index]);
            break;
        case DAX_SINT:
            snprintf(buff, size, "%hhd", ((dax_sint *)val)[index]);
            break;
        case DAX_WORD:
        case DAX_UINT:
            snprintf(buff, size, "%d", ((dax_uint *)val)[index]);
            break;
        case DAX_INT:
            snprintf(buff, size, "%hd", ((dax_int *)val)[index]);
            break;
        case DAX_DWORD:
        case DAX_UDINT:
            snprintf(buff, size, "%u", ((dax_udint *)val)[index]);
            break;
        case DAX_DINT:
            snprintf(buff, size, "%d", ((dax_dint *)val)[index]);
            break;
        case DAX_REAL:
            snprintf(buff, size, "%.8g", ((dax_real *)val)[index]);
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            snprintf(buff, size, "%lu", ((dax_ulint *)val)[index]);
            break;
        case DAX_TIME:
            ms = ((dax_lint *)val)[index] % 1000;
            sprintf(tstr, ".%03d", ms);
            sec = ((dax_lint *)val)[index] / 1000;
            tm = gmtime(&sec);
            strftime(buff, size, "%FT%T", tm);
            strncat(buff, tstr, size);
            break;
        case DAX_LINT:
            snprintf(buff, size, "%ld", ((dax_lint *)val)[index]);
            break;
        case DAX_LREAL:
            snprintf(buff, size, "%.16g", ((dax_lreal *)val)[index]);
            break;
    }
    // TODO: return errors if needed
    return 0;
}

/* This function figures out how to format the data from the string given
 * by *val and places the result in *buff.  If *mask is NULL it is ignored.
 * index is where the data will be wrtitten in buff */
int
dax_string_to_val(char *instr, tag_type type, void *buff, void *mask, int index)
{
    long temp;
    long long ltemp;
    int retval = 0;
    int result, ms, year;
    struct tm tm = {0,0,0,0,0,0,0,0,0};
   
    switch (type) {
        case DAX_BOOL:
            temp = strtol(instr, NULL, 0);
            if(temp == 0) {
                ((u_int8_t *)buff)[index / 8] &= ~(0x01 << (index % 8));
            } else {
                ((u_int8_t *)buff)[index / 8] |= (0x01 << (index % 8));
            }
            if(mask) {
                ((u_int8_t *)mask)[index / 8] |= (0x01 << (index % 8));
            }
            break;
        case DAX_BYTE:
            temp =  strtol(instr, NULL, 0);
            if(temp < DAX_BYTE_MIN) {
                ((dax_byte *)buff)[index] = DAX_BYTE_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(temp > DAX_BYTE_MAX) {
                ((dax_byte *)buff)[index] = DAX_BYTE_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_byte *)buff)[index] = temp;
            if(mask) ((dax_byte *)mask)[index] = 0xFF;
            break;
        case DAX_SINT:
            temp =  strtol(instr, NULL, 0);
            if(temp < DAX_SINT_MIN) {
                ((dax_sint *)buff)[index] = DAX_SINT_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(temp > DAX_SINT_MAX) {
                ((dax_sint *)buff)[index] = DAX_SINT_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_sint *)buff)[index] = temp;
            if(mask) ((dax_sint *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            temp =  strtoul(instr, NULL, 0);
            if(temp < DAX_UINT_MIN) {
                ((dax_uint *)buff)[index] = DAX_UINT_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(temp > DAX_UINT_MAX) {
                ((dax_uint *)buff)[index] = DAX_UINT_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_uint *)buff)[index] = temp;
            if(mask) ((dax_uint *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            temp =  strtol(instr, NULL, 0);
            if(temp < DAX_INT_MIN) {
                ((dax_int *)buff)[index] = DAX_INT_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(temp > DAX_INT_MAX) {
                ((dax_int *)buff)[index] = DAX_INT_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_int *)buff)[index] = temp;
            if(mask) ((dax_int *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        ltemp =  strtol(instr, NULL, 0);
            if(ltemp < DAX_UDINT_MIN) {
                ((dax_udint *)buff)[index] = DAX_UDINT_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(ltemp > DAX_UDINT_MAX) {
                ((dax_udint *)buff)[index] = DAX_UDINT_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_udint *)buff)[index] = ltemp;
            if(mask) ((dax_udint *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            temp =  strtol(instr, NULL, 0);
            if(temp < DAX_DINT_MIN) {
                ((dax_dint *)buff)[index] = DAX_DINT_MIN;
                retval = ERR_UNDERFLOW;
            }
            else if(temp > DAX_DINT_MAX) {
                ((dax_dint *)buff)[index] = DAX_DINT_MAX;
                retval = ERR_OVERFLOW;
            }
            else
                ((dax_dint *)buff)[index] = temp;
            if(mask) ((dax_dint *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((dax_real *)buff)[index] = strtof(instr, NULL);
            if(mask) ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            if(instr[0]=='-') {
                ((dax_ulint *)buff)[index] = 0x0000000000000000;
                retval = ERR_UNDERFLOW;
                break;
            }
            errno = 0;
            ((dax_ulint *)buff)[index] = (dax_ulint)strtoull(instr, NULL, 0);
            if(mask) ((dax_ulint *)mask)[index] = DAX_64_ONES;
            if(errno == ERANGE) {
                retval = ERR_OVERFLOW;
            }
            break;
        case DAX_LINT:
            errno = 0;
            ((dax_lint *)buff)[index] = (dax_lint)strtoll(instr, NULL, 0);
            if(mask) ((dax_lint *)mask)[index] = DAX_64_ONES;
            if(errno == ERANGE) {
                if(instr[0]=='-') {
                    retval = ERR_UNDERFLOW;
                } else {
                    retval = ERR_OVERFLOW;
                }
            }
            break;
        case DAX_TIME:
            /* We use sscanf because strptime doesn't do the milliseconds */
            result = sscanf(instr, "%d-%d-%dT%d:%d:%d.%d", &year, &tm.tm_mon, &tm.tm_mday, 
                                                           &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &ms);
            if(result < 6) { /* It didnt' work */
                errno = 0;
                ((dax_lint *)buff)[index] = (dax_lint)strtoll(instr, NULL, 0);
                if(mask) ((dax_lint *)mask)[index] = DAX_64_ONES;
                if(errno == ERANGE) {
                    if(instr[0]=='-') {
                        retval = ERR_UNDERFLOW;
                    } else {
                        retval = ERR_OVERFLOW;
                    }
                }
            } else {
                if(result == 6) ms = 0;
                tm.tm_year = year - 1900; /* Year is since 1900 in struct tm */
                tm.tm_mon--; /* Month is 0-30 in struct tm */
                ((dax_lint *)buff)[index] = ((dax_lint)(mktime(&tm)-timezone) * 1000) + ms;
                if(mask) ((dax_lint *)mask)[index] = DAX_64_ONES;
            }
            break;
        case DAX_LREAL:
            ((dax_lreal *)buff)[index] = strtod(instr, NULL);
            if(mask) ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
    }
    return retval;
}


#ifdef USE_PTHREAD_LOCK

int
libdax_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_lock(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

int
libdax_unlock(dax_lock *lock) {
    int result;
    result = pthread_mutex_unlock(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

int
libdax_init_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_init(lock, NULL);
    if(result) return ERR_GENERIC;
    return 0;
}

int
libdax_destroy_lock(dax_lock *lock) {
    int result;
    result = pthread_mutex_destroy(lock);
    if(result) return ERR_GENERIC;
    return 0;
}

/* If no locking mechanisms are #defined then we just set the functions
 * to return nothing and hope that the optimizer will eliminate them */
#else

int
libdax_lock(dax_lock *lock) {
    return 0;
}

int
libdax_unlock(dax_lock *lock) {
    return 0;
}

int
libdax_init_lock(dax_lock *lock) {
    return 0;
}

int
libdax_destroy_lock(dax_lock *lock) {
    return 0;
}

#endif
