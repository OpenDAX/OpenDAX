/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
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

 * This file contains the code for recording and dealing with the
 * tag groups.
 */

#ifndef __DAX_GROUPS_H
#define __DAX_GROUPS_H 1

#include <common.h>
#include <opendax.h>
#include "daxtypes.h"

/* Tag groups give clients a way to modify multiple tags all in one
 * message.  These groups are defined by the clients and then can be
 * read and written as a whole afterwards.
 *
 * Each group is implemented as an array of tag_handles and an
 * array of groups is allocated as necessary for each module.
 */

int group_add(dax_module *mod, u_int8_t *handles, u_int8_t count);
int group_del(dax_module *mod, int index);
int group_read(dax_module *mod, u_int32_t index, u_int8_t *buff, int size);
int group_write(dax_module *mod, u_int32_t index, u_int8_t *buff);
int groups_cleanup(dax_module *mod);

#endif /* !__DAX_GROUPS_H */
