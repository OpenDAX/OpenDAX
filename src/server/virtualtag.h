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

 *  Header file for the virtual tag functions
 */

#ifndef __VIRTUALTAG_H
#define __VIRTUALTAG_H

#include <common.h>
#include "func.h"

/* type definition of the virtual tag function prototype */
typedef int vfunction(tag_index idx, int offset, void *data, int size, void *userdata);

/* This structure is used to hold the two virtual functions
 * that would define a virtual tag as well as the userdata for
 * that tag.
 * rf = the read function
 * wf = the write function
 * userdata = userdata that may or may not be needed.
 * If rf is NULL then the tag is write only
 * and if rf is NULL then the tag is read only
 */
typedef struct virt_functions {
    vfunction *rf;
    vfunction *wf;
    void *userdata;
} virt_functions;

#define START_QUEUE_SIZE 16
#define MAX_QUEUE_SIZE 1024

/* This structure represents a single tag queue.  A copy of
 * this would be placed in the *userdata pointer of the virt_function
 * structure in the data area of the tag.
 */
typedef struct tag_queue {
    tag_type type;   /* Type of the tag items */
    u_int32_t count; /* The number of tags of each item */
    u_int32_t size;  /* The size of the tag item */
    int qsize;       /* Total size of the queue in number of tag items */
    int qcount;      /* Current number of items in the queue */
    int qread;       /* Nest item that needs to be read */
    void **queue;    /* Pointer to the actual queue */
} tag_queue;

/* Set virtual function execution environment variables */
void virt_set_fd(int fd);


/* retrieve the current time on the server */
int server_time(tag_index idx, int offset, void *data, int size, void *userdata);

/* retrieve the calling modules tag name */
int get_module_tag_name(tag_index idx, int offset, void *data, int size, void *userdata);

/* queue handling functions */
int write_queue(tag_index idx, int offset, void *data, int size, void *userdata);
int read_queue(tag_index idx, int offset, void *data, int size, void *userdata);

#endif  /* !__VIRTUALTAG_H */
