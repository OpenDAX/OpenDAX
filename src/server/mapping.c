/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2019 Phil Birkelbach
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

 * This is the source file for the data mapping routines
 */


#include <common.h>
#include <tagbase.h>
#include <func.h>

extern _dax_tag_db *_db;

static tag_index _first_tag = -1;
static int _mapping_hops = 0;

/* Allocates and initializes a data map node */
static _dax_datamap *_new_map(Handle src, Handle dest)
{
    _dax_datamap *new;

    new = (_dax_datamap *)malloc(sizeof(_dax_datamap));
    new->id = _db[src.index].nextmap++;
    new->source = src;
    new->dest = dest;
    new->mask = NULL;
    new->next = NULL;

    return new;
}

static void _free_map(_dax_datamap *map) {
    if(map->mask != NULL) {
        free(map->mask);
    }
    free(map);
}


int
map_add(Handle src, Handle dest, int *error)
{
    _dax_datamap *new_map;
    int bit, offset;
    u_int8_t *mask;
    /* Bounds check handles */
    if(src.index < 0 || src.index >= tag_get_count()) {
       xlog(LOG_ERROR, "Source tag index %d for new mapping is out of bounds", src.index);
       return ERR_ARG;
    }
    if(dest.index < 0 || dest.index >= tag_get_count()) {
       xlog(LOG_ERROR, "Destination tag index %d for new mapping is out of bounds", dest.index);
       return ERR_ARG;
    }
    /* Bounds check source size */
    if( (src.byte + src.size) > tag_get_size(src.index)) {
        xlog(LOG_ERROR, "Size of the affected source data in the new mapping is too large");
        return ERR_2BIG;
    }
    /* Bounds check destination size */
    if( (dest.byte + dest.size) > tag_get_size(dest.index)) {
        xlog(LOG_ERROR, "Size of the affected destination data in the new mapping is too large");
        return ERR_2BIG;
    }

    new_map = _new_map(src, dest);

    if(src.type == DAX_BOOL && src.bit > 0) {
        bit = src.bit;
        offset = 0;
        mask = (u_int8_t *)malloc(src.size);
        for(int i=0; i<src.count; i++) {
            mask[offset] |= (0x01 << bit);
            bit++;
            if(bit == 8) {
                bit=0;
                offset++;
            }
        }
        new_map->mask = mask;
    }
    new_map->next = _db[src.index].mappings;
    _db[src.index].mappings = new_map;

    printf("map_add() function called\n");
    printf("src.index = %d\n", src.index);
    printf("src.size = %d\n", src.size);
    printf("dest.index = %d\n", dest.index);
    printf("dest.size = %d\n", dest.size);

    return new_map->id;
}


int
map_del(tag_index index, int id) {
    _dax_datamap *this, *prev;

    if(index < 0 || index >=tag_get_count()) {
        return ERR_ARG;
    }
    this = _db[index].mappings;
    if(this->id == id) {
        _db[index].mappings = this->next;
        _free_map(this);
        return 0;
    } else {
        prev = this;
        this = this->next;
        while(this != NULL) {
            if(this->id == id) {  /* Found it */
                prev->next = this->next;
                _free_map(this);
                return 0;
            }
        }
    }
    return ERR_NOTFOUND;
}


int
map_check(tag_index idx, int offset, u_int8_t *data, int size) {
    _dax_datamap *this;

    this = _db[idx].mappings;
    /* If this is the first time we've been called then it means that is is the tag that started
     * the mapping.  If we chain too many then we'll tell the user which tag started it all. */
    if(_first_tag == -1) _first_tag = idx;
    while(this != NULL) {
        if(offset <= (this->source.byte + this->source.size - 1) && (offset + size -1 ) >= this->source.byte) {
            /* Mapping Hit */
            _mapping_hops++;
            if(_mapping_hops > MAX_MAP_HOPS) {
                /* TODO: conditional compilation of program exit */
                xerror("Maximum number of chained mappings has been reached for tag %s", _db[_first_tag].name);
                return ERR_OVERFLOW;
            }
            if(this->mask != NULL) {
                tag_mask_write(this->dest.index, this->dest.byte, data, this->mask, this->dest.size);
            } else {
                tag_write(this->dest.index, this->dest.byte, data, this->dest.size);
            }
        }
        this = this->next;
    }
    /* if we get here either there were no mapping hits or we are done.  In either case
     * we reset our tracking variables. */
    _first_tag = -1;
    _mapping_hops = 0;
    return 0;
}
