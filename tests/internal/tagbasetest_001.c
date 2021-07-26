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
 *
 *  Main source code file for the OpenDAX Bad Module
 */

/* This test tests for a bug where searching for a tag by name after it was
 * deleted would cause the tagserver to seg fault
 */

#include <tagbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <opendax.h>

int
main(int argc, char *argv[])
{
    dax_tag tag;
   
    initialize_tagbase();
    tag_add("dummy", DAX_INT, 1, 0);
    tag_add("dopey", DAX_INT, 1, 0);
    tag_add("dipey", DAX_INT, 1, 0);
    
    assert(tag_get_name("dummy", &tag) == 0);
    assert(tag_del(tag.idx) == 0);
    assert(tag_get_name("dummy", &tag) == ERR_NOTFOUND);

    return 0;
}
