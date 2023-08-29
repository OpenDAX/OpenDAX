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

/* This test adds a bunch of tags to the database and then makes sure that
 * we can find them all with thier name.
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
    char tagname[DAX_TAGNAME_SIZE + 1];
    int n, result;

    initialize_tagbase();

    for(n=0;n<1000;n++) {
        sprintf(tagname, "TestTag%d", n);
        assert(tag_add(-1, tagname, DAX_INT, 1, 0) > 0);
    }
    for(n=0;n<1000;n++) {
        sprintf(tagname, "TestTag%d", n);
        assert(tag_get_name(tagname, &tag) == 0);
    }

    return 0;
}
