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
#include "tagbase.h"
#include "func.h"

extern _dax_tag_db *_db;

/* Allocates and initializes a data map node */
static _dax_datamap *
_new_map(tag_handle src, tag_handle dest)
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

static void
_free_map(_dax_datamap *map) {
    if(map->mask != NULL) {
        free(map->mask);
    }
    free(map);
}

static inline int
_handles_equal(tag_handle h1, tag_handle h2)
{
    return(h1.index == h2.index &&
           h1.type == h2.type &&
           h1.count == h2.count &&
           h1.byte == h2.byte &&
           h1.bit == h2.bit &&
           h1.size == h2.size );
}

int
map_add(tag_handle src, tag_handle dest)
{
    _dax_datamap *new_map;
    _dax_datamap *this;
    int bit, offset;
    uint8_t *mask;

    DF("map added index1=%d, byte1, %d, size1=%d, index2=%d, byte2=%d, size2=%d", src.index, src.byte, src.size, dest.index, dest.byte, dest.size);
    /* Bounds check handles */
    if(src.index < 0 || src.index >= get_tagindex()) {
       dax_log(DAX_LOG_ERROR, "Source tag index %d for new mapping is out of bounds", src.index);
       return ERR_ARG;
    }

    if(dest.index < 0 || dest.index >= get_tagindex()) {
       dax_log(DAX_LOG_ERROR, "Destination tag index %d for new mapping is out of bounds", dest.index);
       return ERR_ARG;
    }
    if(is_tag_virtual(src.index)){
        return ERR_ILLEGAL;
    }
    if(is_tag_virtual(dest.index)){
        return ERR_ILLEGAL;
    }
    if(is_tag_readonly(dest.index)) {
        dax_log(DAX_LOG_ERROR, "Destination tag is read only");
       return ERR_READONLY;
    }
    /* Bounds check source size */
    if( (src.byte + src.size) > tag_get_size(src.index)) {
        dax_log(DAX_LOG_ERROR, "Size of the affected source data in the new mapping is too large");
        return ERR_2BIG;
    }
    /* Bounds check destination size */
    if( (dest.byte + dest.size) > tag_get_size(dest.index)) {
        dax_log(DAX_LOG_ERROR, "Size of the affected destination data in the new mapping is too large");
        return ERR_2BIG;
    }
    if( src.size > dest.size ) {
        dax_log(DAX_LOG_ERROR, "Size of the source data in the new mapping is too large");
        return ERR_2BIG;
    }
    /* Check for duplicates */
    this = _db[src.index].mappings;
    while(this != NULL) {
        if(_handles_equal(src, this->source) && _handles_equal(dest, this->dest)) {  /* Found a duplicate */
            dax_log(DAX_LOG_WARN, "Duplicate mapping found at index %d, id %d", src.index, this->id);
            return this->id;
        }
        this = this->next;
    }

    new_map = _new_map(src, dest);

    if(src.type == DAX_BOOL) {
        /* This basically creates a mask to write the bits that we want.
         * We are putting the bits in mask according to the destination.
         * The data itself will have to be shifted later when we write
         * the data.
         */
        bit = dest.bit;
        offset = 0;
        mask = (uint8_t *)malloc(src.size);
        bzero(mask, src.size);
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
    _db[src.index].attr |= TAG_ATTR_MAPPING;

    return new_map->id;
}


int
map_del(tag_index index, int id) {
    _dax_datamap *this, *prev;

    if(index < 0 || index >= get_tagindex()) {
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
            this = this->next;
        }
    }
    /* If there are no more mappings reset the attribute flag */
    if(_db[index].mappings == NULL) {
        _db[index].attr &= ~TAG_ATTR_MAPPING;
    }
    return ERR_NOTFOUND;
}

int
map_get(tag_handle *src, tag_handle *dest, tag_index index, int id) {
    _dax_datamap *this;

    if(index < 0 || index >= get_tagindex()) {
        return ERR_ARG;
    }

    this = _db[index].mappings;
    while(this != NULL) {
        if(this->id == id) {  /* Found it */
            memcpy(src, &this->source, sizeof(tag_handle));
            memcpy(dest, &this->dest, sizeof(tag_handle));
            return 0;
        }
        this = this->next;
    }
    return ERR_NOTFOUND;
}


/* Traverse the linked list of maps and delete them all */
int
map_del_all(_dax_datamap *head) {
    _dax_datamap *this, *next;
    this = head;
    while(this != NULL) {
        next = this->next;
        _free_map(this);
        this = next;
    }
    return 0;
}

int
map_check(tag_index idx, int offset, int size) {
    _dax_datamap *this;
    uint8_t *new_data, *srcdb;
    int srcByte;
    int srcBit;
    int destByte;
    int destBit;
    int result;

    this = _db[idx].mappings;
    /* If this is the first time we've been called then it means that is is the tag that started
     * the mapping.  If we chain too many then we'll tell the user which tag started it all. */
    while(this != NULL) {

        if(offset <= (this->source.byte + this->source.size - 1) && (offset + size -1 ) >= this->source.byte) {
            srcdb = &_db[this->source.index].data[this->source.byte];
            /* Mapping Hit */
            if(this->mask != NULL) {
                /* Move the bits from their position in the source to
                 * where they should go in the destination */
                srcByte = 0;
                srcBit = this->source.bit;
                destByte = 0;
                destBit = this->dest.bit;
                new_data = (uint8_t *)malloc(this->source.size);
                if(new_data == NULL) {
                    dax_log(DAX_LOG_ERROR, "Unable to allocate memory for map mask");
                    return ERR_ALLOC;
                }
                bzero(new_data, this->source.size);
                for(int n=0; n<this->source.count; n++) {
                    if(srcdb[srcByte] & (0x01 << srcBit)) {
                        new_data[destByte] |= (0x01 << destBit);
                    }
                    srcBit++;
                    destBit++;
                    if(srcBit == 8) {
                        srcBit = 0;
                        srcByte++;
                    }
                    if(destBit == 8) {
                        destBit = 0;
                        destByte++;
                    }
                }
                result = tag_mask_write(-1, this->dest.index, this->dest.byte, new_data, this->mask, this->source.size);
                /* If the destination tag has been deleted then delete this map */
                if(result == ERR_DELETED) {
                    dax_log(DAX_LOG_DEBUG, "Destination tag has been deleted, removing map %d from tag index = %d", this->id, this->source.index);
                    map_del(this->source.index, this->id);
                }
                free(new_data);
            } else {
                result = tag_write(-1, this->dest.index, this->dest.byte, srcdb, this->source.size);
                /* If the destination tag has been deleted then delete this map */
                if(result == ERR_DELETED) {
                    map_del(this->source.index, this->id);
                }
            }
        }
        this = this->next;
    }
    return 0;
}
