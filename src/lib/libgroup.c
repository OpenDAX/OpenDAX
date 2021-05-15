/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libdax.h>
#include <libcommon.h>

static void
_init_group(group_db *grp) {
    grp->size = 0;
    grp->used = 0x00;
    grp->head = NULL;
}


int
lib_group_add(dax_state *ds, u_int32_t index) {
    u_int32_t n, newsize;
    group_db *new;

    /* If this is the first group to be added to the module we need to allocate
     * the array. */
    if(ds->group_size == 0) {
        ds->group_db = (group_db *)malloc(sizeof(group_db)*TAG_GROUP_START_COUNT);
        if(ds->group_db == NULL) return ERR_ALLOC;
        ds->group_size = TAG_GROUP_START_COUNT;
        for(n=0;n<ds->group_size;n++) {
            _init_group(&ds->group_db[n]);
        }
    }
    if(index >= ds->group_size) {
        newsize = ds->group_size * 2;
        if(newsize > TAG_GROUP_MAX_COUNT) return ERR_2BIG;
        new = (group_db *)realloc(ds->group_db, sizeof(group_db)*newsize);
        if(new == NULL) return ERR_ALLOC;
        ds->group_db = new;
        for(n=ds->group_size; n<newsize;n++) {
            _init_group(&ds->group_db[n]);
        }
        n = ds->group_size;
        ds->group_size = newsize;
    }
    /* If the group that we are adding is already used then we have a problem */
    assert(ds->group_db[index].used == 0x00);
    ds->group_db[index].used = 0x01;
    /* If we get here we are full and need to reallocate the array */
    return 0;
}

int
lib_group_add_member(dax_state *ds, u_int32_t index, tag_handle h) {
    group_db_member *new, *this;

    /* make sure index is within range */
    if(index >= ds->group_size) return ERR_BADINDEX;
    if(ds->group_db[index].used == 0) return ERR_BADINDEX;
    /* Check that the returned tag will fit in a message */
    new = malloc(sizeof(group_db_member));
    if(new == NULL) return ERR_ALLOC;
    new->handle = h;
    new->next = NULL;

    this = ds->group_db[index].head;
    if(this == NULL) {
        ds->group_db[index].head = new;
    } else {
        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new;
    }
    ds->group_db[index].size += h.size;
    return 0;

}
