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
 */

#include <common.h>
#include <opendax.h>
#include <string.h>

#ifndef __TAGS_H
#define __TAGS_H

void add_random_tags(int count);
int check_tagbase(void);
int check_tag_addition(void);
int check_tag_retrieve(void);
//--int tagtopass(char *name);
//--int tagtofail(char *name);

#endif /* !__TAGS_H */