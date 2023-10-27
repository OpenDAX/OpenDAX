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

