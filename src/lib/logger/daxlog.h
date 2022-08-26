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
 * This is main header file for the message logging library
 */

#ifndef __DAXLOG_H
#define __DAXLOG_H

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
/* We are doing this so that these syslog priorites won't
 * conflict with the ones that we define in opendax.h.  Most
 * are different but this makes it more clear which ones we
 * are dealing with */
const int SYSLOG_EMERG    = LOG_EMERG;
const int SYSLOG_ALERT    = LOG_ALERT;
const int SYSLOG_CRIT     = LOG_CRIT;
const int SYSLOG_ERR      = LOG_ERR;
const int SYSLOG_WARNING  = LOG_WARNING;
const int SYSLOG_NOTICE   = LOG_NOTICE;
const int SYSLOG_INFO     = LOG_INFO;
const int SYSLOG_DEBUG    = LOG_DEBUG;
/* The above must be done before including opendax.h */
#include <opendax.h>


typedef struct {
    uint32_t mask;
    void (*log_func)(uint32_t topic, void *data, char *str);
    void *data;
} service_item;

typedef struct {
    FILE *file;
} stdio_service;

typedef struct {
    int priority;
} syslog_service;



#endif  /* !__DAXLOG_H*/

