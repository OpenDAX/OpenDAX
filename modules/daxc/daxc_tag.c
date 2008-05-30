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

static inline void
show_tag(int n, dax_tag temp_tag)
{
    /* Print the name */
    if( n >=0 ) printf("[%d] ", n);
    printf("%s \t %s", temp_tag.name, dax_type_to_string(temp_tag.type));
    /* Output the array size of it's greater than 1 */
    if(temp_tag.count > 1) {
        printf("[%d]", temp_tag.count);
    }
    /* Output the handle and the carriage return */
    //printf(" \t 0x%08X\n", temp_tag.handle);
    //--printf(" \t %d\n", temp_tag.handle);
    printf("\n");
}




/* Adds a tag to the dax tag database. */
int
tag_add(char **tokens)
{
    int type, count;
    handle_t handle;
    const char usage[] = "Usage: add type count\n";
    
    /* Make sure that name is not NULL and we get the tagname */
    if( tokens[0] == NULL) {
        fprintf(stderr, "ERROR: No tagname given\n");
        fprintf(stderr, usage);
        return 1;
    }
    
    if( tokens[1] ) {
        type = dax_string_to_type(tokens[1]);
        if( type < 0 ) {
            fprintf(stderr, "ERROR: Invalid Type Given\n");
            return 1;
        }
    } else {
        fprintf(stderr, "ERROR: No Type Given\n");
        fprintf(stderr, usage);
        return 1;
    }
        
    if( tokens[2] ) {
        count = strtol(tokens[2], NULL, 0);
        if( count == 0 ) {
            fprintf(stderr, "ERROR: Invalid Count Given\n");
        }
    } else {
        fprintf(stderr, "ERROR: No Count Given\n");
        fprintf(stderr, usage);
        return 1;
    }
    
    handle = dax_tag_add(tokens[0], type, count);
    if(handle > 0) {
        printf("Tag Added at handle %d\n", handle);
    } else {
        /* TODO: Print descriptive error message here */
        printf("OPPS Can't add tag???\n");
    }
    return 0;
}

/* TAG LIST command function 
 If we have no arguments then we list all the tags
 If we have a single argument then we see if it's a tagname
 or a number.  If a tagname then we list that tag if it's a number
 then we check for a second argument.  If we have two numbers then
 we list from the first to the first + second if not then we list
 the next X tags and increment lastindex. */

/* TODO: If we allow tags to be deleted then gaps will appear in the list.
 * We should deal with these appropriately */
int
tag_list(char **tokens)
{
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
                    if(dax_tag_byhandle(n, &temp_tag)) {
                        printf("No More Tags To List\n");
                        return 1;
                    } else {
                        show_tag(n, temp_tag);
                    }
                }
            } else {
                /* List the next 'start' amount of tags */
                for(n = lastindex; n < (start + lastindex); n++) {
                    if(dax_tag_byhandle(n, &temp_tag)) {
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
        while( !dax_tag_byhandle(n, &temp_tag) ) {
            show_tag(n, temp_tag);
            n++;
        }
    }
    return 0;
}


/* This function figures out what type of data the tag is and translates
 * buff appropriately and pushes the value onto the lua stack. */
static void inline
dax_to_string(unsigned int type, void *buff, int index)
{
    switch (type) {
     /* Each number has to be cast to the right datatype then dereferenced */
        case DAX_BOOL:
            if((0x01 << index % 8) & ((int8_t *)buff)[index / 8]) {
                printf("1\n");
            } else {
                printf("0\n");
            }
            break;
        case DAX_BYTE:
        case DAX_SINT:
            printf("%hhd\n", ((dax_sint_t *)buff)[index]);
            break;
        case DAX_WORD:
        case DAX_UINT:
            printf("%d\n", ((dax_uint_t *)buff)[index]);
            break;
        case DAX_INT:
            printf("%hd\n", ((dax_int_t *)buff)[index]);
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            printf("%d\n", ((dax_udint_t *)buff)[index]);
            break;
        case DAX_DINT:
            printf("%d\n", ((dax_dint_t *)buff)[index]);
            break;
        case DAX_REAL:
            printf("%g\n", ((dax_real_t *)buff)[index]);
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            printf("%lld\n", ((dax_ulint_t *)buff)[index]);
            break;
        case DAX_LINT:
            printf("%lld\n", ((dax_lint_t *)buff)[index]);
            break;
        case DAX_LREAL:
            printf("%g\n", ((dax_lreal_t *)buff)[index]);
            break;
    }
}

/* This function figures out how to format the data from the string given
 * by *val and places the result in *buff.  If *mask is NULL it is ignored */
static inline void
string_to_dax(char *val, unsigned int type, void *buff, void *mask, int index)
{   
    long temp;
    switch (type) {
        case DAX_BOOL:
            temp = strtol(val, NULL, 0);
            if(temp == 0) {
                ((int8_t *)buff)[index / 8] &= ~(0x01 << index % 8);
            } else {
                ((int8_t *)buff)[index / 8] |= (0x01 << index % 8);
            }
            break;
        case DAX_BYTE:
        case DAX_SINT:
            ((dax_sint_t *)buff)[index] = (dax_sint_t)strtol(val, NULL, 0);
            if(mask) ((dax_sint_t *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            ((dax_uint_t *)buff)[index] = (dax_uint_t)strtol(val, NULL, 0);
            if(mask) ((dax_uint_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            ((dax_int_t *)buff)[index] = (dax_int_t)strtol(val, NULL, 0);
            if(mask) ((dax_int_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            ((dax_udint_t *)buff)[index] = (dax_udint_t)strtol(val, NULL, 0);
            if(mask) ((dax_udint_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            ((dax_dint_t *)buff)[index] = (dax_dint_t)strtol(val, NULL, 0);
            if(mask) ((dax_dint_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((dax_real_t *)buff)[index] = strtof(val, NULL);
            if(mask) ((u_int32_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            ((dax_ulint_t *)buff)[index] = (dax_ulint_t)strtol(val, NULL, 0);
            if(mask) ((dax_ulint_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LINT:
            ((dax_lint_t *)buff)[index] = (dax_lint_t)strtol(val, NULL, 0);
            if(mask) ((dax_lint_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LREAL:
            ((dax_lreal_t *)buff)[index] = strtod(val, NULL);
            if(mask) ((u_int64_t *)mask)[index] = DAX_64_ONES;
            break;
    }
}


int
tag_read(char **tokens)
{
    char name[DAX_TAGNAME_SIZE+ 1];
    handle_t handle;
    int index = -1, result, points = 0, n;
    size_t size;
    dax_tag tag;
    void *buff;
    
    if(tokens[0]) {
        /* If token is numeric then it's a handle that's being passed */
        if(tokens[0][0] >= '0' && tokens[0][0] <= '9') {
            handle = strtol(tokens[0], NULL, 0);
            result = dax_tag_byhandle(handle, &tag);
            if(result) {
                fprintf(stderr, "ERROR: No Tag at Handle: %d\n", handle);
                return 1;
            }
            /* otherwise the token is a tagname */
        } else {
            if(dax_tag_parse(tokens[0], name, &index) || dax_tag_byname(name, &tag)) {
                fprintf(stderr, "ERROR: Bad Tagname Given - %s\n", tokens[0]);
                return 1;
            } else {
                handle = tag.handle;
            }
        }
        
        /* If we have not found an index by this point then... */
        if(index < 0) {
            if( tokens[1] && (tokens[1][0] == 'i' || tokens[1][0] == 'I' )) {
                if(tokens[2] != NULL) {
                    index = strtol(tokens[2], NULL, 0);
                    if(tokens[3] != NULL) {
                        points = strtol(tokens[3], NULL, 0);
                    } else {
                        points = 1;
                    }
                } else {
                    fprintf(stderr, "ERROR: Missing Index\n");
                    return 1;
                }
            } else {
                index = 0;
                points = tag.count;
            }
        } else {
            if(tokens[1] != NULL) {
                points = strtol(tokens[1], NULL, 0);
            }
        }
    } else {
        fprintf(stderr, "ERROR: No Tagname or handle given\n");
        return 1;
    }
    
    /* Get the smaller value */
    if(tag.count < points) {
        points = tag.count;
        /* Should I print a warning??? */
    }
    
    /* Determine how much memory we need */
    if(tag.type == DAX_BOOL) size = points / 8 + 1;
    else                     size = (TYPESIZE(tag.type) / 8) * points;
    
    buff = malloc(size);
    //--printf("buff = %p\n", buff);
    if(!buff) {
        fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
        return 1;
    }
    
    //--printf("Attempting read: handle = %d, index = %d, count = %d, type = %d\n", handle, index, points, tag.type);
    result = dax_read_tag(handle, index, buff, points, tag.type);
    if(result) {
        fprintf(stderr, "ERROR: Problem reading tag from opendax: %d\n", result);
        result = 1;
    } else {
    /*
        for(n = 0; n < size; n++) {
            printf("0x%hhX ", ((char *)buff)[n]);
        }
        printf("\n");
     */   
        for(n = 0; n < points; n++) {
            dax_to_string(tag.type, buff, n);
        }
        result = 0;
    }
    free(buff);
    return result;
}

/* Write data to the dax database.
 * Syntax: handle <i index> data0 data1 ...
 * Or    : tagname <i index> data0 data1 ...
 * Or    : tagname[i] data0 data1 ...
 */
int
tag_write(char **tokens, int tcount)
{
    char name[DAX_TAGNAME_SIZE+ 1 ];
    handle_t handle;
    int index = -1, result, points, next, n;
    size_t size;
    dax_tag tag;
    void *buff;
    
    if(tokens[0]) {
        /* If token is numeric then it's a handle that's being passed */
        if(tokens[0][0] >= '0' && tokens[0][0] <= '9') {
            handle = strtol(tokens[0], NULL, 0);
            result = dax_tag_byhandle(handle, &tag);
            if(result) {
                fprintf(stderr, "ERROR: No Tag at Handle: %d\n", handle);
                return 1;
            }
        /* otherwise the token is a tagname */
        } else {
            if(dax_tag_parse(tokens[0], name, &index) || dax_tag_byname(name, &tag)) {
                fprintf(stderr, "ERROR: Bad Tagname Given - %s\n", tokens[0]);
                return 1;
            } else {
                handle = tag.handle;
            }
        }
        /* If we have not found an index by this point then... */
        if(index < 0) {
            if( tokens[1][0] == 'i' || tokens[1][0] == 'I' ) {
                if(tokens[2] != NULL) {
                    index = strtol(tokens[2], NULL, 0);
                    points = tcount - 3;
                    next = 3;
                } else {
                    fprintf(stderr, "ERROR: Missing Index\n");
                    return 1;
                }
            } else {
                index = 0;
                points = tcount - 1;
                next = 1;
            }
        } else {
            points = tcount - 1;
            next = 1;
        }
    } else {
        fprintf(stderr, "ERROR: No Tagname or handle given\n");
        return 1;
    }
    /* Get the smaller value */
    if(tag.count < points) {
        points = tag.count;
        /* Should I print a warning??? */
    }
    
    if(tag.type == DAX_BOOL) size = points / 8 + 1;
    else                     size = (TYPESIZE(tag.type) / 8) * points;
    
    buff = alloca(size);
    if(!buff) {
        fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
        return 1;
    }
    /* TODO: I might want to add the ability to skip points with a '-'
     * I'd have to search for any '-' and then use dax_mask_tag() instead. */
    for(n = 0; n < points; n++) {
        string_to_dax(tokens[next + n], tag.type, buff, NULL, n);
    }
    result = dax_write_tag(handle, index, buff, points, tag.type);
    if(result) {
        fprintf(stderr, "ERROR: Problem writing data to opendax\n");
        return result;
    }
    
    /**** Get data here *****/
    //printf("Tag Write Command handle = %d, index = %d, points = %d\n", handle, index, points);
    return 0;
}
