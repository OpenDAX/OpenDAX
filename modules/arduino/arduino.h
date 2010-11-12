/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2010 Phil Birkelbach
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
 *  Main header file for the ArduinoIO module
 */

#include <opendax.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef __ARDUINO_H
#define __ARDUINO_H

#define MIN_PIN 2
#define MAX_PIN 54
#define MIN_AI  0
#define MAX_AI  11
#define MAX_NODES 254


struct ar_pin {
    u_int8_t type;
    u_int8_t number;
    u_int8_t pullup;
    char *tagname;
    struct ar_pin *next;
};

struct ar_analog {
    u_int8_t number;
    u_int8_t reference;
    char *tagname;
    struct ar_analog *next;
};

typedef struct ar_node {
    char *name;
    char *address;
    int reconnect;
    struct ar_pin *pins;
    struct ar_analog *analogs;
} ar_node;

#endif