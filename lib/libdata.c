/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
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

 * This file contains some of the database handling code for the library
 */

#include <libdax.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <dax/libcommon.h>

/* Tag Cache Handling Code
 * The tag cache is a doubly linked circular list
 * the head pointer is fixed at the top of the list
 * tag searched for will bubble up one item in the list
 * each time it's found.  This will make the top of the list
 * the most searched for tags and the bottom the lesser used tags.
 */

/* TODO: I need a way to invalidate the tag cache so that changes
   to the tag can be put into the tag cache */

/* This is the structure for our tag cache */
typedef struct Tag_Cnode {
    handle_t handle;
    unsigned int type;
    unsigned int count;
    struct Tag_Cnode *next;
    struct Tag_Cnode *prev;
    char name[DAX_TAGNAME_SIZE + 1];
} tag_cnode;

static tag_cnode *_cache_head; /* First node in the cache list */
static int _cache_limit;       /* Total number of nodes that we'll allocate */
static int _cache_count;       /* How many nodes we actually have */

int
init_tag_cache(void)
{
    _cache_head = NULL;
    _cache_limit = opt_get_cache_limit();
    _cache_count = 0;
    return 0;
}

/****** FOR TESTING ONLY ********
static void
print_cache(void)
{
    int n;
    tag_cnode *this;
    this = _cache_head;
    printf("_cache_head->handle = {%d}\n", _cache_head->handle);
    for(n=0; n<_cache_count; n++) {
        printf("{%d} - %s\t\t", this->handle, this->name);
        printf("this->prev = {%d}", this->prev->handle);
        printf(" : this->next = {%d}\n", this->next->handle);
        this = this->next;
    }
    printf("\n{ count = %d, head = %p }\n", _cache_count, _cache_head);
    //printf("{ head->handle = %d, tail->handle = %d head->prev = %p, tail->next = %p}\n", 
    //       _cache_head->handle, _cache_tail->handle, _cache_head->prev, _cache_tail->next);
}
*/
 
/* This function assigns the data to *tag and bubbles
   this up one node in the list */
static inline void
_cache_hit(tag_cnode *this, dax_tag *tag)
{
    tag_cnode *before, *after;
    
    /* Store the return values in tag */
    strcpy(tag->name, this->name);
    tag->handle = this->handle;
    tag->type = this->type;
    tag->count = this->count;
    
    /* Bubble up:
     after is set to the node that we swap with
     before is set to the node that will be before
     us after the swap.  The special case is when our
     node will become the first one, then we move the head. */
    if(this != _cache_head) { /* Do nothing if we are already at the top */
        /* If there are only two then we do it the easy way */
        if(_cache_count == 2) {
            _cache_head = _cache_head->next;
        } else {
            if(this == _cache_head->next) _cache_head = this;
            after = this->prev;
            before = after->prev;
            before->next = this;
            after->prev = this;
            after->next = this->next;
            this->next->prev = after;
            this->next = after;
            this->prev = before;
        }
    }
    //--print_cache();
}

int
check_cache_handle(handle_t handle, dax_tag *tag)
{
    tag_cnode *this;
    
    if(_cache_head != NULL) {
        this = _cache_head;
    } else {
        return ERR_NOTFOUND;
    }
    if(_cache_head->handle != handle) {
        this = this->next;
        
        while(this != _cache_head && this->handle != handle) {
            this = this->next;
        }
        if(this == _cache_head) {
            return ERR_NOTFOUND;
        }
    }
    
    _cache_hit(this, tag);
    
    return 0;
}


int
check_cache_name(char *name, dax_tag *tag)
{
    tag_cnode *this;
    
    if(_cache_head != NULL) {
        this = _cache_head;
    } else {
        return ERR_NOTFOUND;
    }
    if( strcmp(_cache_head->name, name) ) {
        this = this->next;
        
        while(this != _cache_head && strcmp(this->name, name)) {
            this = this->next;
        }
        if(this == _cache_head) {
            return ERR_NOTFOUND;
        }
    }
    
    _cache_hit(this, tag);
    
    return 0;
}

int
cache_tag_add(dax_tag *tag)
{
    tag_cnode *new;
    
    if(_cache_head == NULL) { /* First one */
        //--printf("adding {%d} to the beginning\n", tag->handle);
        new = malloc(sizeof(tag_cnode));
        if(new) {
            new->next = new;
            new->prev = new;
            _cache_head = new;
            _cache_count++;
        } else {
            return ERR_ALLOC;
        }
    } else if(_cache_count < _cache_limit) { /* Add to end */
        //--printf("adding {%d} to the end\n", tag->handle);
        new = malloc(sizeof(tag_cnode));
        if(new) {
            new->next = _cache_head;
            new->prev = _cache_head->prev;
            _cache_head->prev->next = new;
            _cache_head->prev = new;
            _cache_count++;
        } else {
            return ERR_ALLOC;
        }
    } else {
        //--printf("Just putting {%d}  last\n", tag->handle);
        new = _cache_head->prev;
    }
    strcpy(new->name, tag->name);
    new->handle = tag->handle;
    new->type = tag->type;
    new->count = tag->count;
    //--print_cache();
    
    return 0;
}


/* Type specific reading and writing functions.  These should be the most common
 * methods to read and write tags to the sever.*/
int
dax_read_tag(handle_t handle, int index, void *data, int count, unsigned int type)
{
    int bytes, result, offset, n, i;
    char *buff;
    
    if(type == DAX_BOOL) {
        bytes = (index + count - 1)  / 8 - index / 8 + 1;
        offset = index / 8;
    } else {
        bytes = (TYPESIZE(type) / 8) * count;
        offset = (TYPESIZE(type) / 8) * index;
    }
    buff = alloca(bytes);
    bzero(buff, bytes);
    bzero(data, (count - 1) / 8 + 1);
    result = dax_read(handle, offset, buff, bytes);
    
    if(result) return result;
    
    switch(type) {
        case DAX_BOOL:
            i = index % 8;
            for(n = 0; n < count; n++) {
                if( (0x01 << i % 8) & buff[i / 8] ) {
                    ((char *)data)[n / 8] |= (1 << (n % 8));
                }
                i++;
            }
            break;
        case DAX_BYTE:
        case DAX_SINT:
            memcpy(data, buff, bytes);
            break;
        case DAX_WORD:
        case DAX_UINT:
            for(n = 0; n < count; n++) {
                ((dax_uint_t *)data)[n] = stom_uint(((dax_uint_t *)buff)[n]);
            }
            break;
        case DAX_INT:
            for(n = 0; n < count; n++) {
                ((dax_int_t *)data)[n] = stom_int(((dax_int_t *)buff)[n]);
            }
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            for(n = 0; n < count; n++) {
                ((dax_udint_t *)data)[n] = stom_udint(((dax_udint_t *)buff)[n]);
            }
            break;
        case DAX_DINT:
            for(n = 0; n < count; n++) {
                ((dax_dint_t *)data)[n] = stom_dint(((dax_dint_t *)buff)[n]);
            }
            break;
        case DAX_REAL:
            for(n = 0; n < count; n++) {
                ((dax_real_t *)data)[n] = stom_real(((dax_real_t *)buff)[n]);
            }
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            for(n = 0; n < count; n++) {
                ((dax_ulint_t *)data)[n] = stom_ulint(((dax_ulint_t *)buff)[n]);
            }
            break;
        case DAX_LINT:
            for(n = 0; n < count; n++) {
                ((dax_lint_t *)data)[n] = stom_lint(((dax_lint_t *)buff)[n]);
            }
            break;
        case DAX_LREAL:
            for(n = 0; n < count; n++) {
                ((dax_lreal_t *)data)[n] = stom_lreal(((dax_lreal_t *)buff)[n]);
            }
            break;
        default:
            return ERR_ARG;
            break;
    }    
    return 0;
}


int
dax_write_tag(handle_t handle, int index, void *data, int count, unsigned int type)
{
    int bytes, result, offset, n, i;
    char *buff, *mask;
    
    if(type == DAX_BOOL) {
        bytes = (index + count - 1)  / 8 - index / 8 + 1;
        offset = index / 8;
    } else {
        bytes = (TYPESIZE(type) / 8) * count;
        offset = (TYPESIZE(type) / 8) * index;
    }
    buff = alloca(bytes);
    
    switch(type) {
        case DAX_BOOL:
            mask = alloca(bytes);
            bzero(buff, bytes);
            bzero(mask, bytes);
            
            i = index % 8;
            for(n = 0; n < count; n++) {
                if( (0x01 << n % 8) & ((char *)data)[n / 8] ) {
                    buff[i / 8] |= (1 << (i % 8));
                }
                mask[i / 8] |= (1 << (i % 8));
                i++;
            }            
            break;
        case DAX_BYTE:
        case DAX_SINT:
            memcpy(buff, data, bytes);
            break;
        case DAX_WORD:
        case DAX_UINT:
            for(n = 0; n < count; n++) {
                ((dax_uint_t *)buff)[n] = mtos_uint(((dax_uint_t *)data)[n]);
            }
            break;
        case DAX_INT:
            for(n = 0; n < count; n++) {
                ((dax_int_t *)buff)[n] = mtos_int(((dax_int_t *)data)[n]);
            }
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            for(n = 0; n < count; n++) {
                ((dax_udint_t *)buff)[n] = mtos_udint(((dax_udint_t *)data)[n]);
            }
            break;
        case DAX_DINT:
            for(n = 0; n < count; n++) {
                ((dax_dint_t *)buff)[n] = mtos_dint(((dax_dint_t *)data)[n]);
            }
            break;
        case DAX_REAL:
            for(n = 0; n < count; n++) {
                ((dax_real_t *)buff)[n] = mtos_real(((dax_real_t *)data)[n]);
            }
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            for(n = 0; n < count; n++) {
                ((dax_ulint_t *)buff)[n] = mtos_ulint(((dax_ulint_t *)data)[n]);
            }
            break;
        case DAX_LINT:
            for(n = 0; n < count; n++) {
                ((dax_lint_t *)buff)[n] = mtos_lint(((dax_lint_t *)data)[n]);
            }
            break;
        case DAX_LREAL:
            for(n = 0; n < count; n++) {
                ((dax_lreal_t *)buff)[n] = mtos_lreal(((dax_lreal_t *)data)[n]);
            }
            break;
        default:
            return ERR_ARG;
            break;
    }
    if(type == DAX_BOOL) {
        result = dax_mask(handle, offset, buff, mask, bytes);
    } else {    
        result = dax_write(handle, offset, buff, bytes);
    }
    return result;
}

int
dax_mask_tag(handle_t handle, int index, void *data, void *mask, int count, unsigned int type)
{
    int bytes, result, offset, n, i;
    char *buff, *maskbuff;
    
    if(type == DAX_BOOL) {
        bytes = (index + count - 1)  / 8 - index / 8 + 1;
        offset = index / 8;
    } else {
        bytes = (TYPESIZE(type) / 8) * count;
        offset = (TYPESIZE(type) / 8) * index;
    }
    buff = alloca(bytes);
    maskbuff = alloca(bytes);
    
    switch(type) {
        case DAX_BOOL:
            bzero(buff, bytes);
            bzero(maskbuff, bytes);
            
            i = index % 8;
            for(n = 0; n < count; n++) {
                if( (0x01 << n % 8) & ((char *)data)[n / 8] ) {
                    buff[i / 8] |= (1 << (i % 8));
                }
                maskbuff[i / 8] |= (1 << (i % 8));
                i++;
            }            
            break;
        case DAX_BYTE:
        case DAX_SINT:
            memcpy(buff, data, bytes);
            memcpy(maskbuff, mask, bytes);
            break;
        case DAX_WORD:
        case DAX_UINT:
            for(n = 0; n < count; n++) {
                ((dax_uint_t *)buff)[n] = mtos_uint(((dax_uint_t *)data)[n]);
                ((dax_uint_t *)maskbuff)[n] = mtos_uint(((dax_uint_t *)mask)[n]);
            }
            break;
        case DAX_INT:
            for(n = 0; n < count; n++) {
                ((dax_int_t *)buff)[n] = mtos_int(((dax_int_t *)data)[n]);
                ((dax_int_t *)maskbuff)[n] = mtos_int(((dax_int_t *)mask)[n]);
            }
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            for(n = 0; n < count; n++) {
                ((dax_udint_t *)buff)[n] = mtos_udint(((dax_udint_t *)data)[n]);
                ((dax_udint_t *)maskbuff)[n] = mtos_udint(((dax_udint_t *)mask)[n]);
            }
            break;
        case DAX_DINT:
            for(n = 0; n < count; n++) {
                ((dax_dint_t *)buff)[n] = mtos_dint(((dax_dint_t *)data)[n]);
                ((dax_dint_t *)maskbuff)[n] = mtos_dint(((dax_dint_t *)mask)[n]);
            }
            break;
        case DAX_REAL:
            for(n = 0; n < count; n++) {
                ((dax_real_t *)buff)[n] = mtos_real(((dax_real_t *)data)[n]);
                ((dax_real_t *)maskbuff)[n] = mtos_real(((dax_real_t *)mask)[n]);
            }
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            for(n = 0; n < count; n++) {
                ((dax_ulint_t *)buff)[n] = mtos_ulint(((dax_ulint_t *)data)[n]);
                ((dax_ulint_t *)maskbuff)[n] = mtos_ulint(((dax_ulint_t *)mask)[n]);
            }
            break;
        case DAX_LINT:
            for(n = 0; n < count; n++) {
                ((dax_lint_t *)buff)[n] = mtos_lint(((dax_lint_t *)data)[n]);
                ((dax_lint_t *)maskbuff)[n] = mtos_lint(((dax_lint_t *)mask)[n]);
            }
            break;
        case DAX_LREAL:
            for(n = 0; n < count; n++) {
                ((dax_lreal_t *)buff)[n] = mtos_lreal(((dax_lreal_t *)data)[n]);
                ((dax_lreal_t *)maskbuff)[n] = mtos_lreal(((dax_lreal_t *)mask)[n]);
            }
            break;
        default:
            return ERR_ARG;
            break;
    }
    result = dax_mask(handle, offset, buff, maskbuff, bytes);
    return result;
}

#ifdef DELETE_BIT_STUFF_IEYTHQGJHRIUEHWLGIURHEWQL

/* TODO: I need to do something with all these bit manipulation functions */
/* returns zero if the bit is clear, 1 if it's set */
/* TODO: Shouldn't we return an error somehow? */
char dax_tag_read_bit(handle_t handle) {
    u_int8_t data;
    /* read the one byte that contains the handle the bit we want */
    dax_read(handle & ~0x07, 0, &data, 1);
    if(data & (1 << (handle % 8))) {
        return 1;
    }
    return 0;
}

/* sets the bit if data evaluates true and clears it otherwise */
int dax_tag_write_bit(handle_t handle, u_int8_t data) {
    u_int8_t input, mask;
    if(data) {
        input = 0xFF;
    } else {
        input = 0x00;
    }
    mask = 1 << (handle % 8);
    dax_mask(handle & ~0x07, 0, &input, &mask, 1);
    return 0;
}


/* Reads bits starting at handle, size is in bits not bytes.  This function
   will be more efficient if the size and handle are evenly divisible by 8 */
int dax_tag_read_bits(handle_t handle, void *data, size_t size) {
    void *buffer;
    size_t n, buff_len, buffer_idx, data_idx, data_bit, buffer_bit;
    /* Screw it I'm gonna allocate the whole thing and see what happens.
       I'm also gonna use alloca() because it should be faster on the stack
       none of this may matter and I'd be easy enough to change */
    buff_len = size / 8; /* Start here */
    
 /* If handle and size are even bytes then this is just a byte read */
    if( handle % 8 == 0 && size % 8 == 0) {
        return dax_read(handle, 0, data, buff_len);
    }

    buff_len = (((handle + size - 1) & ~0x07) - (handle & ~0x07)) / 8 + 1;
    buffer = alloca(buff_len);
    if(buffer == NULL) {
        return -1;
    }
    if(dax_read(handle, 0, buffer, buff_len)) {
        return -1;
    }
    buffer_bit = handle % 8;
    buffer_idx = 0;
    data_bit = 0;
    data_idx = 0;
 /* Loop through the bits moving from buffer to data */
    for(n = 0; n< size; n++) {
        if(((u_int8_t *)buffer)[buffer_idx] & (1 << buffer_bit)) { /* If *buff bit is set */
            ((u_int8_t *)data)[data_idx] |= (u_int8_t)(1 << data_bit);
        } else {  /* If the bit in the buffer is not set */
            ((u_int8_t *)data)[data_idx] &= ~(u_int8_t)(1 << data_bit);
        }
        data_bit++;
        if(data_bit == 8) {
            data_bit = 0;
            data_idx++;
        }
        buffer_bit++;
        if(buffer_bit == 8) {
            buffer_bit = 0;
            buffer_idx++;
        }
    }
    return 0;
}

/* This is how many bytes we'll send in each call to the masked write. */
#define DAX_BIT_MSG_SIZE (MSG_TAG_DATA_SIZE / 2)

/* Writes bits starting at handle, size is in bits not bytes */
/* This function could be a lot simpler if we'd just allocate a buffer
   that is big enough for all the bits to fit and then let the 
   dax_mask() function handle splitting them up.  I'm betting
   on this being quite a bit faster.  It might not matter. */
int dax_tag_write_bits(handle_t handle, void *data, size_t size) {
    u_int8_t buffer[DAX_BIT_MSG_SIZE];
    u_int8_t mask[DAX_BIT_MSG_SIZE];
    size_t buffer_idx, buffer_bit, data_idx, data_bit;
    handle_t next_handle;
    int n;
    
    buffer_idx = 0;
    buffer_bit = handle % 8;
    next_handle = handle & ~0x07;
    data_idx = 0;
    data_bit = 0;
    
    bzero(mask, DAX_BIT_MSG_SIZE);
    
    for(n=0; n<size; n++) {
        if(((u_int8_t *)data)[data_idx] & (1 << data_bit)) { /* If *buff bit is set */
            buffer[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
        } else {  /* If the bit in the buffer is not set */
            buffer[buffer_idx] &= ~(u_int16_t)(1 << buffer_bit);
        }
        /* This sets the mask bit */
        mask[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
    
        data_bit++;
        if(data_bit == 8) {
            data_bit = 0;
            data_idx++;
        }
        
        buffer_bit++;
        if(buffer_bit == 8) {
            buffer_bit = 0;
            buffer_idx++;
            if(buffer_idx == DAX_BIT_MSG_SIZE) {
                /* Out of room in the buffer, send the message */
                dax_mask(next_handle, 0, buffer, mask, buffer_idx);
                next_handle += DAX_BIT_MSG_SIZE;
                buffer_idx = 0;
                for(n=0; n < DAX_BIT_MSG_SIZE; n++) mask[n]=0x00;
            }
        }
    }
    dax_mask(next_handle, 0, buffer, mask, buffer_idx + 1);
    /* TODO: Need to do some real error checking here and return a good number */
    return size;
}

int dax_tag_mask_write_bits(handle_t handle, void *data, void *mask, size_t size) {
    u_int8_t buffer[DAX_BIT_MSG_SIZE];
    u_int8_t newmask[DAX_BIT_MSG_SIZE];
    size_t buffer_idx, buffer_bit, data_idx, data_bit;
    handle_t next_handle;
    int n;
    
    buffer_idx = 0;
    buffer_bit = handle % 8;
    next_handle = handle & ~0x07;
    data_idx = 0;
    data_bit = 0;
    
    bzero(newmask, DAX_BIT_MSG_SIZE);
    
    for(n=0; n<size; n++) {
        if(((u_int8_t *)mask)[data_idx] & (1 << data_bit)) {
            if(((u_int8_t *)data)[data_idx] & (1 << data_bit)) { /* If *buff bit is set */
                buffer[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
            } else {  /* If the bit in the buffer is not set */
                buffer[buffer_idx] &= ~(u_int16_t)(1 << buffer_bit);
            }
            /* This sets the mask bit */
            newmask[buffer_idx] |= (u_int16_t)(1 << buffer_bit);
        }
        data_bit++;
        if(data_bit == 8) {
            data_bit = 0;
            data_idx++;
        }
        
        buffer_bit++;
        if(buffer_bit == 8) {
            buffer_bit = 0;
            buffer_idx++;
            if(buffer_idx == DAX_BIT_MSG_SIZE) {
                /* Out of room in the buffer, send the message */
                dax_mask(next_handle, 0, buffer, mask, buffer_idx);
                next_handle += DAX_BIT_MSG_SIZE;
                buffer_idx = 0;
                bzero(mask, DAX_BIT_MSG_SIZE);
                //--for(n=0; n < DAX_BIT_MSG_SIZE; n++) mask[n]=0x00;
            }
        }
    }
    dax_mask(next_handle, 0, buffer, mask, buffer_idx + 1);
    /* TODO: Need to do some real error checking here and return a good number */
    return size;
}

#endif