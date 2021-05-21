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

#include <common.h>
#include "func.h"

/* type definition of the virtual tag function prototype */
typedef int vfunction(int offset, void *data, int size);

/* This structure is used to hold the two virtual functions
 * that would define a tag.  rf = the read function, wf = the
 * write function.  If rf is NULL then the tag is write only
 * and if rf is NULL then the tag is read only
 */
typedef struct virt_functions {
    vfunction *rf;
    vfunction *wf;
} virt_functions;

/* retrieve the current time on the server */
int server_time(int offset, void *data, int size);
