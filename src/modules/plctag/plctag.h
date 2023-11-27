/* Dax Modbus - Modbus (tm) Communications Module
 * Copyright (C) 2023 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Main header file for the plctag client module
 */

#ifndef __PLCTAG_H__
#define __PLCTAG_H__

#include <opendax.h>
#include <common.h>
#include <pthread.h>

/* This should probably be in the configuration */
#define QUEUE_SIZE 256

struct tagdef {
    char *daxtag;
    char *plctag;
    tag_handle h;
    uint8_t *buff;
    int count;
    int elem_size;
    uint16_t *byte_map;
    int read_update;
    int write_update;
    int32_t tag_id;
    struct tagdef *next;
};


int plctag_configure(int, char **);
void tag_thread_start(void);
int tag_queue_push(struct tagdef *tag);

#endif