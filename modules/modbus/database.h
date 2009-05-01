/* database.h - Modbus (tm) Database Handling Library
 * Copyright (C) 2006 Phil Birkelbach
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
 */

#ifndef __DATABASE_H
#define __DATABASE_H

#include <common.h>
#include <opendax.h>

int dt_add_tag(char *name, int index, int function, int length);
int dt_getwords(tag_idx_t handle, int index, void *data, int length);
int dt_setwords(tag_idx_t handle, int index, void *data, int length);
int dt_getbits(tag_idx_t handle, int index, void *data, int length);
int dt_setbits(tag_idx_t handle, int index, void *data, int length);

#endif
