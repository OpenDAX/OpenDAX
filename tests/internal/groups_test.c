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
 *  Main source code file for the OpenDAX Bad Module
 */
/* This is a test to make sure that adding and deleting tag groups
 * from a module will work properly.
 */

#include "opendax.h"
#include "daxtypes.h"
#include "groups.h"
#include "libcommon.h"

static void
_test_simple(void) {
    dax_module mod;
    u_int32_t index, n, offset;
    tag_handle h;

    /* Initialize the module */
    mod.groups_size = 0;
    mod.tag_groups = NULL;

    index = group_add(&mod);
    if(index < 0) exit(index);
    assert(mod.groups_size == TAG_GROUP_START_COUNT);
    assert(index==0); /* Zero should be the first one */
    n=1;
    for(n = 0;n<100;n++) {
        h.index = n;
        h.size = 4;
        group_add_member(&mod, 0, h, &offset);
    }
    assert(mod.tag_groups[0].size == 400);
    groups_cleanup(&mod);
}

static void
_test_group_array_growth(void) {
    dax_module mod;
    u_int32_t index, n;
    tag_handle h;

    /* Initialize the module */
    mod.groups_size = 0;
    mod.tag_groups = NULL;

    for(n=0;n<TAG_GROUP_START_COUNT;n++) {
        index = group_add(&mod);
        if(index < 0) exit(index);
        assert(index == n);
    }
    /* When all that is done we should still have the same size array */
    assert(mod.groups_size == TAG_GROUP_START_COUNT);

    /* Now add one more and make sure it doubles in size */
    index = group_add(&mod);
    if(index < 0) exit(index);
    assert(index == TAG_GROUP_START_COUNT);

    /* Just add one group member to take our place in the array */
    assert(mod.groups_size == TAG_GROUP_START_COUNT*2);
    groups_cleanup(&mod);
}


static void
_test_group_size(void) {
    dax_module mod;
    u_int32_t index, n, offset;
    tag_handle h;
    int result, count;

    /* Initialize the module */
    mod.groups_size = 0;
    mod.tag_groups = NULL;

    index = group_add(&mod);
    if(index < 0) exit(index);

    count = MSG_TAG_GROUP_DATA_SIZE / 4;
    for(n=0;n<count;n++) {
        h.index = n;
        h.size = 4;
        result = group_add_member(&mod, index, h, &offset);
        assert(result == 0);
    }
    /* At this point we should be full and adding one more should fail */
    h.index = n;
    h.size = 4;
    result = group_add_member(&mod, index, h, &offset);
    assert(result == ERR_2BIG);

    /* Just add one group member to take our place in the array */
    groups_cleanup(&mod);
}

static void
_test_group_add_member_errors(void) {
    dax_module mod;
    u_int32_t index, n, offset;
    tag_handle h;
    int result, count;

    /* Initialize the module */
    mod.groups_size = 0;
    mod.tag_groups = NULL;

    index = group_add(&mod);
    if(index < 0) exit(index);

    h.index = n;
    h.size = 4;
    result = group_add_member(&mod, index, h, &offset);
    assert(result == 0);

    result = group_add_member(&mod, index + 1, h, &offset);
    assert(result == ERR_BADINDEX);

    index = TAG_GROUP_START_COUNT;
    result = group_add_member(&mod, index, h, &offset);
    assert(result == ERR_BADINDEX);

    /* Just add one group member to take our place in the array */
    groups_cleanup(&mod);
}
/* Add some members to the list and verify that they are actually
 * there and in the proper order */
void
_test_group_list(void) {
    dax_module mod;
    u_int32_t index, n, offset;
    tag_handle h;
    tag_group_member *this;

    /* Initialize the module */
    mod.groups_size = 0;
    mod.tag_groups = NULL;

    index = group_add(&mod);
    if(index < 0) exit(index);
    assert(index==0); /* Zero should be the first one */
    n=1;
    for(n = 0;n<100;n++) {
        h.index = n;
        h.size = 4;
        group_add_member(&mod, 0, h, &offset);
    }
    this = mod.tag_groups[0].head;
    for(n=0;n<100;n++) {
        assert(this->handle.index == n);
        this = this->next;
    }
    assert(this==NULL);
    groups_cleanup(&mod);
}

int
main(int argc, char *argv[]) {
    _test_simple();
    _test_group_array_growth();
    _test_group_size();
    _test_group_list();
    _test_group_add_member_errors();
    exit(0);
}
