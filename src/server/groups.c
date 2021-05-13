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

#include "libcommon.h"
#include "groups.h"

/* This adds a tag group to the given module.  It simply determines if there
 * is space on the currently allocated array and if not allocates more space.
 * Returns the index of the new group.
 */
int
add_group(dax_module *mod) {
    u_int32_t n, newsize;
    tag_group *new;
    /* If this is the first group to be added to the module we need to allocate
     * the array. */
    if(mod->groups_size == 0) {
        mod->tag_groups = (tag_group *)malloc(sizeof(tag_group)*TAG_GROUP_START_COUNT);
        if(mod->tag_groups == NULL) return ERR_ALLOC;
        mod->groups_size = TAG_GROUP_START_COUNT;
        for(n=0;n<mod->groups_size;n++) {
            mod->tag_groups[n].size = 0;
            mod->tag_groups[n].head = NULL;
        }
    }
    /* Scan through and look for an empty spot */
    for(n=0;n<mod->groups_size;n++) {
        if(mod->tag_groups[n].head == NULL) return n;
    }
    /* If we get here we are full and need to reallocate the array */
    newsize = mod->groups_size * 2;
    new = (tag_group *)realloc(mod->tag_groups, sizeof(tag_group)*newsize);
    if(new == NULL) return ERR_ALLOC;
    mod->tag_groups = new;
    for(n=mod->groups_size; n<newsize;n++) {
        mod->tag_groups[n].size = 0;
        mod->tag_groups[n].head = NULL;
    }

    n = mod->groups_size;
    mod->groups_size = newsize;
    return n;
}

/* adds a group memeber to the tag group given by the mod and index
 * If the handle is too big for the message it returns ERR_2BIG
 */
int
add_group_member(dax_module *mod, u_int32_t index, tag_handle h) {
    tag_group_member *new, *this;

    /* Check that the returned tag will fit in a message */
    if(mod->tag_groups[index].size + h.size > MSG_TAG_GROUP_DATA_SIZE) {
        return ERR_2BIG;
    }
    new = malloc(sizeof(tag_group_member));
    if(new == NULL) return ERR_ALLOC;
    new->handle = h;
    new->next = NULL;

    this = mod->tag_groups[index].head;
    if(this == NULL) {
        mod->tag_groups[index].head = new;
    } else {
        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new;
    }
    mod->tag_groups[index].size += h.size;
    return 0;
}

/* Recursive function to delete all the tag_group_members */
static void
_delete_member(tag_group_member *this) {
    if(this->next != NULL) {
        _delete_member(this->next);
    }
    free(this);
}

int
del_group(dax_module *mod, u_int32_t index) {
    if(mod->tag_groups[index].head != NULL) {
        _delete_member(mod->tag_groups[index].head);
    }
    mod->tag_groups[index].head = NULL;
    return 0;
}

/* Deletes all of the groups and free's the tag_groups array */
int
groups_cleanup(dax_module *mod) {
    u_int32_t n;

    for(n=0;n<mod->groups_size;n++) {
        del_group(mod, n);
    }
    free(mod->tag_groups);
    mod->tag_groups = NULL;
    mod->groups_size = 0;

    return 0;
}


