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
 *  This test checks the atomic complement operation
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

static int
_bool_test(dax_state *ds) {
    int result;
    tag_handle h, h2;
    dax_byte temp[4];
    dax_byte temp1[4];
    dax_byte output[4];

    if(dax_tag_add(ds, &h, "bool_test", DAX_BOOL, 32, 0)) return -1;
    temp[0] = 0xAA; temp[1] = 0x55;
    if(dax_write_tag(ds, h, temp)) return -1;
    temp1[0] = 0x0F; temp1[1] = 0xF0;
    if(dax_atomic_op(ds, h, temp1, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;
    DF("in = %02X %02X", temp[0], temp[1]);
    DF("out= %02X %02X", output[0], output[1]);
    if(output[0] != 0x0A || output[1] != 0x50) {
        return -1;
    }

    /* No we do a partial subset of the bits */
    if(dax_tag_handle(ds, &h2, "bool_test[3]", 10)) return -1;
    temp[0] = 0x55; temp[1] = 0x55; temp[2] = 0x55;
    if(dax_write_tag(ds, h, temp)) return -1;
    temp1[0] = 0xFF; temp1[1] = 0x00;
    if(dax_atomic_op(ds, h2, temp1, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;
    DF("in     = %02X %02X", temp1[0], temp1[1]);
    DF("output = %02X %02X", output[0], output[1]);
    if(output[0] != 0x55 || output[1] != 0x45) return -1;

    /* No we do a partial subset of the bits Even %8 but offset by odd amount*/
    if(dax_tag_handle(ds, &h2, "bool_test[1]", 16)) return -1;
    temp[0] = 0x55; temp[1] = 0x55; temp[2] = 0x55;
    if(dax_write_tag(ds, h, temp)) return -1;
    temp1[0] = 0xFF; temp1[1] = 0x00; temp1[2] = 0x00;
    if(dax_atomic_op(ds, h2, temp1, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;
    DF("in     = %X %X %X", temp1[2], temp1[1], temp1[0]);
    DF("output = %X %X %X", output[2], output[1], output[0]);
    if(output[0] != 0x55 || output[1] != 0x01 || output[2] != 0x54) return -1;

    return 0;
}

static int
_byte_test(dax_state *ds) {
    int result;
    tag_handle h;
    dax_byte input1[] = {0xAA, 0x55, 0x0F, 0xF0};
    dax_byte input2[] = {0x0F, 0xF0, 0xF0, 0x0F};
    dax_byte output[4];

    if(dax_tag_add(ds, &h, "byte_test", DAX_BYTE, 4, 0)) return -1;
    if(dax_write_tag(ds, h, input1)) return -1;
    if(dax_atomic_op(ds, h, input2, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;
    for(int n=0;n<4;n++) {
        if(output[n] != (input1[n] & input2[n])) {
            DF("n=%d, output = %X, in1 = %X, in2 = %X, answer = %X", n, output[n], input1[n], input2[n], input1[n] & input2[n]);
            return -1;
        }
    }
    return 0;
}

static int
_sint_test(dax_state *ds) {
    tag_handle h;
    dax_sint input1[] = {-128, 0, 85, 0};
    dax_sint input2[] = {0, -128, -86, 0x0F};
    dax_sint output[4];

    if(dax_tag_add(ds, &h, "sint_test", DAX_BYTE, 4, 0)) return -1;
    if(dax_write_tag(ds, h, input1)) return -1;
    if(dax_atomic_op(ds, h, input2, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;

    for(int n=0;n<4;n++) {
        if(output[n] != (input1[n] & input2[n])) {
            DF("n=%d, output = %X, in1 = %X, in2 = %X, answer = %X", n, output[n], input1[n], input2[n], input1[n] & input2[n]);
            return -1;
        }
    }
    return 0;
}

static int
_dint_test(dax_state *ds) {
    int result;
    tag_handle h;
    dax_dint input1[] = {0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, -2147483648, -1};
    dax_dint input2[] = {1234,       -3453,      -1,         2151686160, 1,            15};
    dax_dint output[6];

    if(dax_tag_add(ds, &h, "dint_test", DAX_DINT, 6, 0)) return -1;
    if(dax_write_tag(ds, h, input1)) return -1;
    if(dax_atomic_op(ds, h, input2, ATOMIC_OP_AND)) return -1;
    if(dax_read_tag(ds, h, output)) return -1;
    for(int n=0;n<6;n++) {
        DF("n=%d, output = %d, in1 = %d, in2 = %d, answer = %d", n, output[n], input1[n], input2[n], input1[n] & input2[n]);
        if(output[n] != (input1[n] & input2[n])) {
            return -1;
        }
    }
    return 0;
}

/* Since complementing a floating point value doesn't make any sense we should
 * get illegal operation errors back from the server */
static int
_real_test(dax_state *ds) {
    int result;
    tag_handle h;
    dax_real temp1[4];
    dax_lreal temp2[4];

    result = dax_tag_add(ds, &h, "real_test", DAX_REAL, 4, 0);
    if(result) return -1;
    temp1[0] = 3.141592; temp1[1] = -43234.23455; temp1[2] = -1; temp1[3] = 0;
    result = dax_write_tag(ds, h, temp1);
    if(result) return -1;
    result = dax_atomic_op(ds, h, temp2, ATOMIC_OP_AND);
    if(result != ERR_BADTYPE) return -1;

    result = dax_tag_add(ds, &h, "lreal_test", DAX_LREAL, 4, 0);
    if(result) return -1;
    temp2[0] = 3.141592; temp2[1] = -43234.23455; temp2[2] = -1; temp2[3] = 0;
    result = dax_write_tag(ds, h, temp2);
    if(result) return -1;
    result = dax_atomic_op(ds, h, temp2, ATOMIC_OP_AND);
    if(result != ERR_BADTYPE) return -1;

    return 0;
}


int
do_test(int argc, char *argv[])
{
    dax_state *ds;
    int result = 0;


    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    }
    if(_bool_test(ds)) return -1;
    if(_byte_test(ds)) return -1;
    if(_sint_test(ds)) return -1;
    if(_dint_test(ds)) return -1;
    if(_real_test(ds)) return -1;
    dax_disconnect(ds);

    return 0;
}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    if(run_test(do_test, argc, argv, 0)) {
        DF("Test Failed");
        exit(-1);
    } else {
        DF("Test Passed");
        exit(0);
    }
}
