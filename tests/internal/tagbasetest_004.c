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

/* Simple test of the tag queue"
 */

#include <stdlib.h>
#include <stdio.h>
#include <opendax.h>
#include <tagbase.h>

int
main(int argc, char *argv[])
{
    dax_tag tag;
    int result;
    dax_dint temp, n;

    initialize_tagbase();
    dax_log_set_default_mask(LOG_ALL);
    result = tag_add(-1, "queue_test", DAX_DINT | DAX_QUEUE, 1, 0);
    if(result < 0) exit(-1);
    for(temp = 1; temp<25; temp++) {
        assert(tag_write(-1, result, 0, &temp, sizeof(dax_dint))==0);
    }

    for(n = 1; n<25; n++) {
        assert(tag_read(-1, result, 0, &temp, sizeof(dax_dint))==0);
        printf("temp = %d\n", temp);
        assert(temp == n);
    }

    for(temp = 10; temp<16; temp++) {
        assert(tag_write(-1, result, 0, &temp, sizeof(dax_dint))==0);
    }

    for(n = 10; n<16; n++) {
        assert(tag_read(-1, result, 0, &temp, sizeof(dax_dint))==0);
        printf("temp = %d\n", temp);
        assert(temp == n);
    }

    return 0;
}
