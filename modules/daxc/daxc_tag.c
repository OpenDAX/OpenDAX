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
 *  source code file for the tag commands OpenDAX Command Line Client  module
 */

#include <daxc.h>

static inline void show_tag(int n, dax_tag temp_tag) {
    /* Print the name */
    if( n >=0 ) printf("[%d] ", n);
    printf("%s \t %s", temp_tag.name, dax_type_to_string(temp_tag.type));
    /* Output the array size of it's greater than 1 */
    if(temp_tag.count > 1) {
        printf("[%d]", temp_tag.count);
    }
    /* Output the handle and the carriage return */
    printf(" \t 0x%08X\n", temp_tag.handle);
    //--printf(" \t %d\n", temp_tag.handle);
}

/* TAG LIST command function 
 If we have no arguments then we list all the tags
 If we have a single argument then we see if it's a tagname
 or a number.  If a tagname then we list that tag if it's a number
 then we check for a second argument.  If we have two numbers then
 we list from the first to the first + second if not then we list
 the next X tags and increment lastindex. */
int tag_list(void) {
    dax_tag temp_tag;
    static int lastindex;
    char *arg[] = {NULL, NULL};
    char *end_ptr;
    int n, argcount, start, count;
    
    argcount = 0;
    
    arg[0] = strtok(NULL, " ");
    
    if(arg[0]) {
        start = strtol(arg[0], &end_ptr, 0);
        /* If arg[0] is text then it's a tagname instead of an index */
        if(end_ptr == arg[0]) {
            if( dax_tag_byname(arg[0], &temp_tag) ) {
                printf("ERROR: Unknown Tagname %s\n", arg[0]);
                return 1;
            } else {
                show_tag(-1, temp_tag);
            }
            /* arg[0] should be an integer here and start is the first one */
        } else {
            arg[1] = strtok(NULL, " ");
            /* List tags from start to start + count */
            if(arg[1]) {
                count = strtol(arg[1], &end_ptr, 0);
                for(n = start; n < (start + count); n++) {
                    if(dax_tag_byindex(n, &temp_tag)) {
                        printf("No More Tags To List\n");
                        return 1;
                    } else {
                        show_tag(n, temp_tag);
                    }
                }
            } else {
                /* List the next 'start' amount of tags */
                for(n = lastindex; n < (start + lastindex); n++) {
                    if(dax_tag_byindex(n, &temp_tag)) {
                        printf("No More Tags To List\n");
                        lastindex = 0;
                        return 1;
                    } else {
                        show_tag(n, temp_tag);
                    }
                }
                lastindex += start;
            }
        }
    } else {
        /* List all tags */
        n = 0;
        while( !dax_tag_byindex(n, &temp_tag) ) {
            show_tag(n,temp_tag);
            n++;
        }
    }
    return 0;
}


static inline void string_to_dax(char *val, unsigned int type, void *buff, void *mask, int index) {
    
    switch (type) {
        case DAX_BYTE:
        case DAX_SINT:
            ((u_int8_t *)buff)[index] = (u_int8_t)strtol(val, NULL, 0);
            ((u_int8_t *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            ((u_int16_t *)buff)[index] = (u_int16_t)strtol(val, NULL, 0);
            ((u_int16_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            ((int16_t *)buff)[index] = (int16_t)strtol(val, NULL, 0);
            ((u_int16_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            ((u_int32_t *)buff)[index] = (u_int32_t)strtol(val, NULL, 0);
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            ((int32_t *)buff)[index] = (int32_t)strtol(val, NULL, 0);
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((float *)buff)[index] = strtof(val,NULL);
            ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            ((u_int64_t *)buff)[index] = (u_int64_t)strtol(val, NULL, 0);
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LINT:
            ((int64_t *)buff)[index] = (int64_t)strtol(val, NULL, 0);
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LREAL:
            ((double *)buff)[index] = strtod(val,NULL);
            ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
    }
}


int tag_set(void) {
    char *name;
    char *val;
    int n,x,index,result;
    size_t size;
    void *buff, *mask;
    dax_tag tag;
    
    name = strtok(NULL, " ");
    if(name == NULL) {
        fprintf(stderr, "ERROR: No Tagname or Handle Given\n");
        return 1;
    }
    /* TODO: Need to decide if this is a handle or a tag */
    if( dax_tag_byname(name, &tag) ) {
        fprintf(stderr, "ERROR: Tagname %s not found\n", name);
        return 1;
    }
    /* Figure the size of the bufer and the mask */
    if( tag.type == DAX_BOOL ) {
        if( tag.handle % 8 == 0) {
            size = tag.count / 8 + 1;
        } else {
            size = tag.count / 8 + 2;
        }
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
    }
    
    /* Allocate a buffer and a mask */
    buff = alloca(size);
    bzero(buff, size);
    mask = alloca(size);
    bzero(mask, size);
    if(buff == NULL || mask == NULL) {
        fprintf(stderr, "ERROR: Unable to allocate memory\n");
        return 1;
    }
    
    val = strtok(NULL, " ");
    if(val == NULL) {
        fprintf(stderr, "ERROR: No Value Given\n");
        return 1;
    }
    
    for(n = 0; n < tag.count; n++ ) {
        /* We'll use a single hyphen as a way to leave that index alone */
        if(strcasecmp(val, "-")) {        
            if(tag.type == DAX_BOOL) {
                /* booleans are special */
                if(!strcasecmp(val, "true")) {
                    result = 1;
                } else if(!strcasecmp(val, "false")) {
                    result = 0;
                } else {
                    result = strtol(val, NULL, 0);
                }
                x = (n + tag.handle) % 8;
                index = ((tag.handle % 8) + n) / 8;
                if( result ) { /* true */
                    ((u_int8_t *)buff)[index] |= (1 << (x % 8));
                } else {  /* false */
                    ((u_int8_t *)buff)[index] &= ~(1 << (x % 8));
                }
                ((u_int8_t *)mask)[index] |= (1 << (x % 8));
            } else {
                /* Not a boolean */
                string_to_dax(val, tag.type, buff, mask, n);
            }
        }
        val = strtok(NULL, " ");
        if(val == NULL) { /* If no more then get out */
            n = tag.count;
        }
    }
    //--printf("byte 0x%X, mask 0x%X\n",((char *)buff)[0], ((char *)mask)[0]);
    dax_tag_mask_write(tag.handle, buff, mask, size);
    return 0;
}


/* This function figures out what type of data the tag is and translates
 *buff appropriately and pushes the value onto the lua stack */
static inline void dax_to_string(unsigned int type, void *buff) {
    switch (type) {
            /* Each number has to be cast to the right datatype then dereferenced
             and then cast to double for pushing into the Lua stack */
        case DAX_BYTE:
        case DAX_SINT:
            printf("%d\n",*((u_int8_t *)buff));
            break;
        case DAX_WORD:
        case DAX_UINT:
            printf("%d\n",*((u_int16_t *)buff));
            break;
        case DAX_INT:
            printf("%d\n",*((int16_t *)buff));
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            printf("%d\n",*((u_int32_t *)buff));
            break;
        case DAX_DINT:
            printf("%d\n",*((int32_t *)buff));
            break;
        case DAX_REAL:
            printf("%g\n",*((float *)buff));
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            printf("%lld\n",*((u_int64_t *)buff));
            break;
        case DAX_LINT:
            printf("%lld\n",*((int64_t *)buff));
            break;
        case DAX_LREAL:
            printf("%g\n",*((double *)buff));
            break;
    }
}


int tag_get(void) {
    char *name;
    dax_tag tag;
    int size, n;
    size_t buff_idx, buff_bit;
    void *buff;
    
    name = strtok(NULL, " ");
    /* Make sure that name is not NULL and we get the tagname */
    if( name == NULL) {
        fprintf(stderr, "ERROR: No tagname given\n");
        return 1;
    }
    /* TODO: Need to decide if this is a handle or a tag */
    if(dax_tag_byname(name, &tag) ) {
        fprintf(stderr, "ERROR: Unable to retrieve tag - %s", name);
        return 1;
    }
    /* We have to treat Booleans differently */
    if(tag.type == DAX_BOOL) {
        /* Check to see if it's an array */
        size = tag.count / 8 + 1;  //--@#$%@#^$%  Might not need the +1
        buff = alloca(size);
        if(buff == NULL) {
            fprintf(stderr, "Unable to allocate buffer size = %d", size);
            return 1;
        }
        dax_tag_read_bits(tag.handle, buff, tag.count);
        buff_idx = buff_bit = 0;
        
        for(n = 0; n < tag.count ; n++) {
            if(((u_int8_t *)buff)[buff_idx] & (1 << buff_bit)) { /* If *buff bit is set */
                printf("1\n");
            } else {  /* If the bit in the buffer is not set */
                printf("0\n");
            }
            
            buff_bit++;
            if(buff_bit == 8) {
                buff_bit = 0;
                buff_idx++;
            }
        }
        /* Not a boolean */
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
        buff = alloca(size);
        if(buff == NULL) {
            fprintf(stderr, "Unable to allocate buffer size = %d", size);
            return 1;
        }
        dax_tag_read(tag.handle, buff, size);
        //--DEBUG: printf("tag.count = %d, tag.type = %s\n", tag.count, dax_type_to_string(tag.type));
        
        for(n = 0; n < tag.count ; n++) {
            dax_to_string( tag.type, buff + (TYPESIZE(tag.type) / 8) * n);
        }
    }
    return 0;
}
