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

/* TODO Would be much better to pack the BOOLs together. */

#include "libcommon.h"
#include "groups.h"
#include "tagbase.h"

static void
_init_group(tag_group *grp) {
    grp->size = 0;
    grp->flags = 0x00;
    grp->members = NULL;
}


int
_group_add(dax_module *mod) {
    uint32_t n, newsize;
    tag_group *new;

    /* If this is the first group to be added to the module we need to allocate
     * the array. */
    if(mod->groups_size == 0) {
        mod->tag_groups = (tag_group *)malloc(sizeof(tag_group)*TAG_GROUP_START_COUNT);
        if(mod->tag_groups == NULL) return ERR_ALLOC;
        mod->groups_size = TAG_GROUP_START_COUNT;
        for(n=0;n<mod->groups_size;n++) {
            _init_group(&mod->tag_groups[n]);
        }
    }
    /* Scan through and look for an empty spot */
    for(n=0;n<mod->groups_size;n++) {
        if((mod->tag_groups[n].flags & GRP_FLAG_NOT_EMPTY) == 0) {
            return n;
        }
    }
    /* If we get here we are full and need to reallocate the array */
    newsize = mod->groups_size * 2;
    if(newsize > TAG_GROUP_MAX_COUNT) return ERR_2BIG;
    new = (tag_group *)realloc(mod->tag_groups, sizeof(tag_group)*newsize);
    if(new == NULL) return ERR_ALLOC;
    mod->tag_groups = new;
    for(n=mod->groups_size; n<newsize;n++) {
        _init_group(&mod->tag_groups[n]);
    }

    n = mod->groups_size;
    mod->groups_size = newsize;
    return n;
}


/* This adds a tag group to the given module.  It simply determines if there
 * is space on the currently allocated array and if not allocates more space.
 * The handles array is the part of the message buffer that contains the
 * tag handle data
 * Returns the index of the new group.
 */
int
group_add(dax_module *mod, uint8_t *handles, uint8_t count) {
    int index, datasize, offset;

    if(count > TAG_GROUP_MAX_MEMBERS) return ERR_ARG;
    index = _group_add(mod);
    if(index < 0) return index; /* Pass the error on up */
    mod->tag_groups[index].members = (tag_handle *)malloc(sizeof(tag_handle) * count);
    if(mod->tag_groups[index].members == NULL) return ERR_ALLOC;
    /* Check to make sure the size of the group is within bounds */
    datasize = 0;
    for(int n=0; n<count; n++) {
        offset = 21*n;
        memcpy(&mod->tag_groups[index].members[n].index, &handles[offset], 4);
        memcpy(&mod->tag_groups[index].members[n].byte, &handles[offset+4], 4);
        mod->tag_groups[index].members[n].bit = handles[offset+8];
        memcpy(&mod->tag_groups[index].members[n].count, &handles[offset+9], 4);
        memcpy(&mod->tag_groups[index].members[n].size, &handles[offset+13], 4);
        memcpy(&mod->tag_groups[index].members[n].type, &handles[offset+17], 4);

        datasize += mod->tag_groups[index].members[n].size;
        if(datasize > MSG_TAG_GROUP_DATA_SIZE) {
            free(mod->tag_groups[index].members);
            mod->tag_groups[index].members = NULL;
            return ERR_2BIG;
        }
    }

    mod->tag_groups[index].flags |= GRP_FLAG_NOT_EMPTY;
    mod->tag_groups[index].count = count;
    dax_log(LOG_MSG, "Group Add message from %s", mod->name);
    return index;
}


/* Deletes a single tag group from the given module */
int
group_del(dax_module *mod, int index) {
    if((mod->tag_groups[index].flags & GRP_FLAG_NOT_EMPTY) == 0) return ERR_NOTFOUND;

    free(mod->tag_groups[index].members);
    mod->tag_groups[index].flags = 0x00;
    mod->tag_groups[index].count = 0;
    mod->groups_size--;
    dax_log(LOG_MSG, "Group Delete message from %s", mod->name);
    return 0;
}

/* loop through the array of members and populate buff with
 * the data.*/
int
group_read(dax_module *mod, uint32_t index, uint8_t *buff, int size) {
    int n, offset=0, result;
    tag_group *group;

    group = &mod->tag_groups[index];
    if((group->flags & GRP_FLAG_NOT_EMPTY) == 0) return ERR_NOTFOUND;
    if(group->size > size) return ERR_ARG;
    for(n = 0;n<group->count;n++) {
        result = tag_read(group->members[n].index, group->members[n].byte, &buff[offset], group->members[n].size);
        if(result) return result;
        offset += group->members[n].size;
    }
    return offset;
}

/* loop through the array of members and populate buff with
 * the data.*/
int
group_write(dax_module *mod, uint32_t index, uint8_t *buff) {
    int n, offset=0, result;
    tag_group *group;

    group = &mod->tag_groups[index];
    if((group->flags & GRP_FLAG_NOT_EMPTY) == 0) return ERR_NOTFOUND;
    for(n = 0;n<group->count;n++) {
        result = tag_write(group->members[n].index, group->members[n].byte, &buff[offset], group->members[n].size);
        if(result) return result;
        offset += group->members[n].size;
    }
    return offset;
}


/* Deletes all of the groups and free's the tag_groups array
 * This is called when we are removing the module */
int
groups_cleanup(dax_module *mod) {
    uint32_t n;

    for(n=0;n<mod->groups_size;n++) {
        group_del(mod, n);
    }
    free(mod->tag_groups);
    mod->tag_groups = NULL;
    mod->groups_size = 0;

    return 0;
}


