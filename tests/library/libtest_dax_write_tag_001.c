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
 */

/*
 *  This tests a regression where the dax_write_tag function was
 *  not choosing to send a masked write message and dealing with
 *  the bit offset in a proper way
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

void
print_buffer(dax_byte *buff, int len) {
    for(int i=0; i<len; i++) {
        printf("[0x%X]", buff[i]);
    }
    printf("\n");
}

int
do_test(int argc, char *argv[])
{
    tag_handle tag, h;
    int result = 0;
    dax_byte x, buff[16];
    dax_id id;
    dax_state *ds;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    } else {
        dax_tag_add(ds, &tag, "Dummy", DAX_BOOL, 128, 0);
        /* Set the whole tag to 1s */
        memset(buff, '\xFF', tag.size);
        dax_write_tag(ds, tag, buff);
        dax_read_tag(ds, tag, buff);
        /* Manually set the handle to clear a single bit */
        h.index = tag.index;
        h.byte = 0;
        h.bit = 0;
        h.count = 1;
        h.size = 1;
        h.type = DAX_BOOL;
        x = 0x00;
        dax_write_tag(ds, h, &x);
        /* Read them to make sure */
        dax_read_tag(ds, tag, buff);
        print_buffer(buff, tag.size);
        if(buff[0] != 0xFE) return -1;

        /* Now try setting a single bit */
        memset(buff, '\xFF', tag.size);
        buff[0] = 0xFE; /* Set the lowest bit to zero */
        dax_write_tag(ds, tag, buff);
        dax_read_tag(ds, tag, buff);
        /* Manually set the handle to clear a single bit */
        h.index = tag.index;
        h.byte = 0;
        h.bit = 0;
        h.count = 1;
        h.size = 1;
        h.type = DAX_BOOL;
        x = 0x01;
        dax_write_tag(ds, h, &x);
        /* Read them to make sure */
        dax_read_tag(ds, tag, buff);
        print_buffer(buff, tag.size);
        if(buff[0] != 0xFF) return -1;

        /* Now try clearing 8 bits at an offset */
        memset(buff, '\xFF', tag.size);
        dax_write_tag(ds, tag, buff);
        dax_read_tag(ds, tag, buff);
        /* Manually set the handle to clear a single bit */
        h.index = tag.index;
        h.byte = 0;
        h.bit = 2;
        h.count = 8;
        h.size = 1;
        h.type = DAX_BOOL;
        x = 0x00;
        dax_write_tag(ds, h, &x);
        /* Read them to make sure */
        dax_read_tag(ds, tag, buff);
        print_buffer(buff, tag.size);
        if(buff[0] != 0x03 || buff[1] != 0xFC) return -1;
    }
    return 0;
}

/* main inits and then calls run_test */
int
main(int argc, char *argv[])
{
    if(run_test(do_test, argc, argv)) {
        exit(-1);
    } else {
        exit(0);
    }
}
