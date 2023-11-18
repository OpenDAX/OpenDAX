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


#include "daxc.h"
#include <ctype.h>

extern dax_state *ds;
extern int quiet_mode;

static inline void
show_tag(int n, dax_tag temp_tag)
{
    /* Print the name */
    if( n >= 0 ) printf("[%3d] ", n);
    printf("%-32s %s", temp_tag.name, dax_type_to_string(ds, temp_tag.type));
    /* Output the array size of it's greater than 1 */
    if(temp_tag.count > 1) {
        printf("[%d]", temp_tag.count);
    }
    if(temp_tag.attr) {
        if(temp_tag.attr & TAG_ATTR_READONLY) printf(" READONLY");
        if(temp_tag.attr & TAG_ATTR_VIRTUAL)  printf(" VIRTUAL");
        if(temp_tag.attr & TAG_ATTR_RETAIN)   printf(" RETAINED");
        if(temp_tag.attr & TAG_ATTR_OVR_SET) printf(" OVERRIDE");
    }
    printf("\n");
}

static unsigned int
_get_attributes(char **tokens) {
    int n=0;
    unsigned int attr = 0x00;

    while(tokens[n]) {
        if(!strncasecmp(tokens[n], "retain", 6)) {
            attr |= TAG_ATTR_RETAIN;
        } else {
            fprintf(stderr, "ERROR: Unknown attribute %s\n", tokens[n]);
            return 0x00;
        }
        n++;
    }
    return attr;
}

/* Adds a tag to the dax tag database. */
int
tag_add(char **tokens)
{
    int count, result;
    tag_type type;
    tag_handle handle;
    unsigned int attr = 0x00;
    const char usage[] = "Usage: add tag name type [count] [attributes] ... \n";

    /* Make sure that name is not NULL and we get the tagname */
    if( tokens[0] == NULL) {
        fprintf(stderr, "ERROR: No tagname given\n");
        fprintf(stderr, usage);
        return 1;
    }

    if( tokens[1] ) {
        type = dax_string_to_type(ds, tokens[1]);
        if( type < 0 ) {
            fprintf(stderr, "ERROR: Invalid Type '%s' Given\n", tokens[1]);
            return 1;
        }
    } else {
        fprintf(stderr, "ERROR: No Type Given\n");
        fprintf(stderr, usage);
        return 1;
    }

    if( tokens[2] ) {
        count = strtol(tokens[2], NULL, 0);
        if( count == 0 ) { /* this is not a number so it might be attributes */
            count = 1;
            attr = _get_attributes(&tokens[2]);
            if(attr == 0) {
                return 1;
            }
        } else { /* If we have a count then see if we have attributes after */
            attr = _get_attributes(&tokens[3]);
        }
    } else { // If no count is given make it one
        count = 1;
        //fprintf(stderr, "ERROR: No Count Given\n");
        //fprintf(stderr, usage);
    }

    result = dax_tag_add(ds, &handle, tokens[0], type, count, attr);
    if(result == 0) {
        if(!quiet_mode) printf("Tag Added at index %d\n", handle.index);
    } else if(result == ERR_TAG_BAD) {
        fprintf(stderr, "ERROR: Bad Tagname\n");
    } else if(result == ERR_TAG_DUPL) {
        fprintf(stderr, "ERROR: Duplicate Tagname\n");
    } else if(result == ERR_2BIG) {
        fprintf(stderr, "ERROR: Tagname is too long\n");
    } else if(result == ERR_ILLEGAL) {
        fprintf(stderr, "ERROR: Illegal Tagname\n");
    } else {
        /* TODO: Print descriptive error message here */
        printf("OPPS Can't add tag???, error = %d\n", result);
    }
    return 0;
}

int
tag_del(char **tokens)
{
    dax_tag temp_tag;
    int result;

    if(isdigit(tokens[0][0])) {
        result = dax_tag_byindex(ds, &temp_tag, atoi(tokens[0]));
    } else {
        result = dax_tag_byname(ds, &temp_tag, tokens[0]);
    }

    if(result ==  ERR_NOTFOUND) {
        fprintf(stderr, "ERROR: Unknown Tagname %s\n", tokens[0]);
        return 1;
    } else if(result == ERR_ARG) {
        fprintf(stderr, "ERROR: Unknown Tag Index %d\n", atoi(tokens[0]));
        return 1;
    } else if(result == ERR_DELETED) {
        fprintf(stderr, "ERROR: Tag was already deleted\n");
        return 1;
    }
    result = dax_tag_del(ds, temp_tag.idx);
    if(result == ERR_ILLEGAL) {
        fprintf(stderr, "ERROR: Not allowed to delete that tag\n");
        return 1;
    } else if(result) {
        fprintf(stderr, "ERROR: Unable to delete tag\n");
        return 1;
    }
    return 0;
}


/* TAG LIST command function
 * If we have no arguments then we list all the tags
 * If we have a single argument then we see if it's a tagname
 * or a number.  If a tagname then we list that tag if it's a number
 * then we check for a second argument.  If we have two numbers then
 * we list from the first to the first + second if not then we list
 * the next X tags and increment nextindex. */

int
list_tags(char **tokens)
{
    dax_tag temp_tag;
    tag_handle h;
    static int nextindex;
    int lastindex;
    int result;
    char *arg[] = {NULL, NULL};
    char *end_ptr;
    int n, start, count;

    if(tokens) {
    	arg[0] = tokens[0];
    	if(arg[0] != NULL) arg[1] = tokens[1];
    }

    result = dax_tag_handle(ds, &h, "_lastindex", 1);
    if(result) return result;
    result = dax_read_tag(ds, h, &lastindex);
    if(result) return result;
    if(nextindex >= lastindex) nextindex=0;

    if(arg[0]) {
        start = strtol(arg[0], &end_ptr, 0);
        /* If arg[0] is text then it's a tagname instead of an index */
        if(end_ptr == arg[0]) {
            if( dax_tag_byname(ds, &temp_tag, arg[0]) ) {
                fprintf(stderr, "ERROR: Unknown Tagname %s\n", arg[0]);
                return 1;
            } else {
                show_tag(-1, temp_tag);
            }
            /* arg[0] should be an integer here and start is the first one */
        } else {
            /* List tags from start to start + count */
            if(arg[1]) {
                count = strtol(arg[1], &end_ptr, 0);
                nextindex = start;
                n=0;
                while( n < count && nextindex <= lastindex) {
                    result = dax_tag_byindex(ds, &temp_tag, nextindex);
                    if(result == 0) {
                        show_tag(nextindex, temp_tag);
                        n++;
                        nextindex++;
                    } else if(result == ERR_DELETED) {
                        nextindex++;
                    } else {
                        printf("No More Tags To List\n");
                        return 1;
                    }
                }
            } else {
                /* List the next 'start' amount of tags */
                n = 0;
                while(n < start && nextindex <= lastindex) {
                    result = dax_tag_byindex(ds, &temp_tag, nextindex);
                    if(result == 0) {
                        show_tag(nextindex, temp_tag);
                        n++;
                        nextindex++;
                    } else if(result == ERR_DELETED) {
                        nextindex++;
                    } else {
                        printf("No More Tags To List\n");
                        return 1;
                    }
                }
            }
        }
    } else {
        /* List all tags */
        for(n=0; n<=lastindex; n++) {
            result = dax_tag_byindex(ds, &temp_tag, n);
            if(result == ERR_DELETED) {
                ; /* do nothing */
            } else if(result) {
                printf("Error: %d\n", result);
                return result;
            } else {
                /* Otherwise show it */
                show_tag(n, temp_tag);
            }
        }
        nextindex = 0; /* Reset this static variable so other tag lists start at the beginning */
    }
    return 0;
}

static void _print_compound_tag(tag_type type, void *buff);

static int indent;

static void
_read_cdt_callback(cdt_iter iter, void *buff)
{
    char str[32];
    char *s;
    uint8_t *x;
    int n;

    for(n=0;n<indent;n++) fprintf(stdout, " ");

    fprintf(stdout, "%s ", iter.name);
    if(IS_CUSTOM(iter.type)) {
        indent += 2;
        fprintf(stdout, "\n");
        _print_compound_tag(iter.type, buff+iter.byte);
        indent -= 2;
    } else if(iter.type == DAX_CHAR) {
        s = (char *)(buff+iter.byte);
        for(n=0;n<iter.count;n++) {
            if(s[n] == 0x00) {
                fprintf(stdout, " ");
            }
            fprintf(stdout, "%c", s[n]);
        }
        fprintf(stdout, "\n");
    } else if(iter.type == DAX_BOOL) {
        x = buff + iter.byte;
        for(n=0;n<iter.count;n++) {
            if(0x01 <<((iter.bit+n) % 8) & (x[(iter.bit+n) / 8])) {
                fprintf(stdout, "true ");
            } else {
                fprintf(stdout, "false ");
            }
        }
        fprintf(stdout, "\n");
    } else {
        for(n = 0; n < iter.count; n++) {
            x = buff + iter.byte;
            dax_val_to_string(str, 32, iter.type, x, n);
            fprintf(stdout, "%s ", str);
        }
        fprintf(stdout,"\n");
    }

}


static void
_print_compound_tag(tag_type type, void *buff) {
    dax_cdt_iter(ds, type, buff, _read_cdt_callback);
}

/* This is the tag read function.  One or two parameters can be given. The
 * first parameter is the tagname and the second if given is the number of
 * points to be returned */
int
tag_read(char **tokens)
{
    tag_handle handle;
    int result, n, count = 0;
    char *name;
    void *buff;
    char str[32];

    if(tokens[0]) {
        name = tokens[0];
        if(tokens[1]) {
            count = (int)strtoul(tokens[1], NULL, 0);
        }
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }

    result = dax_tag_handle(ds, &handle, name, count);
    if(result) {
        if(result == ERR_2BIG) {
            fprintf(stderr, "ERROR: %s is too large a request\n", name);
        } else {
            /* TODO: More descriptive error messages here based on result */
            fprintf(stderr, "ERROR: %s Not a Valid Tag\n", name);
        }
        return ERR_ARG;
    }

    buff = malloc(handle.size + 1);
    if(buff == NULL) {
        fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
        return ERR_ALLOC;
    }

    result = dax_read_tag(ds, handle, buff);
    if(result == 0) {
        if(IS_CUSTOM(handle.type)) {
            /* Print Custom Stuff Here */
            for(n=0; n<handle.count; n++) {
                indent = 0;
                _print_compound_tag(handle.type, buff+dax_get_typesize(ds, handle.type)*n);
            }
        } else if(handle.type == DAX_CHAR) {
            ((char *)buff)[handle.size] = 0x00;
            for(n=0;n<handle.size;n++) {
                if(((char *)buff)[n] == 0x00) {
                    fprintf(stdout, " ");
                }
                fprintf(stdout, "%c", ((char *)buff)[n]);
            }
            fprintf(stdout, "\n");
        } else {
            for(n = 0; n < handle.count; n++) {
                dax_val_to_string(str, 32, handle.type, buff, n);
                fprintf(stdout, "%s\n", str);
            }
        }
    } else if(result == ERR_DELETED) {
        fprintf(stderr, "ERROR: Tag '%s' has been deleted\n", tokens[0]);
    } else if(result == ERR_NOTFOUND) {
        fprintf(stderr, "ERROR: Tag '%s' could not be found\n", tokens[0]);
    } else {
        fprintf(stderr, "ERROR: Unable to Read Tag %s\n", tokens[0]);
    }
    free(buff);
    return result;
}

/* This is the tag write function.  It is pretty lame at the moment.  It
 * takes a tagname as the first argument and then as many arguments after that
 * as needed to represent the data to add.*/
int
tag_write(char **tokens, int tcount) {
    tag_handle handle;
    int result, n, points;
    char *name;
    uint8_t use_mask = 0;
    void *buff, *mask;
    int count;

    if(tcount < 2) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    if(strstr(tokens[1], "\"")) { /* We are assuming this is a string */
        count = strlen(tokens[1]);
        if(tokens[1][0] == '\"') { /* remove the leading " */
            count--;
            tokens[1]++;
        }
        if(tokens[1][count-1] =='\"') { /* remove the trailing " */
            tokens[1][count-1] = '\0';
            count--;
        }
        if(count <= 0) {
            fprintf(stderr, "ERROR: Zero length string argument\n");
            return ERR_ARG;
        }
        result = dax_tag_handle(ds, &handle, name, 0);
        if(result) {
            if(result == ERR_2BIG) {
                fprintf(stderr, "ERROR: %s is too large a request\n", name);
            } else {
                /* TODO: More descriptive error messages here based on result */
                fprintf(stderr, "ERROR: %s Not a Valid Tag\n", name);
            }
            return ERR_ARG;
        }
        if(TYPESIZE(handle.type) != 8) {
            fprintf(stderr, "ERROR: Strings can only be written to 8 bit data types\n");
            return ERR_ARG;
        }
        buff = malloc(handle.size);
        bzero(buff, handle.size);
        memcpy(buff, (void *)tokens[1], MIN(count, handle.size));
        result = dax_write_tag(ds, handle, buff);
    } else {
        result = dax_tag_handle(ds, &handle, name, tcount-1);
        if(result) {
            /* TODO: More descriptive error messages here based on result */
            fprintf(stderr, "ERROR: %s Not a Valid Tag\n", tokens[0]);
            return ERR_ARG;
        }

        buff = malloc(handle.size);
        mask = malloc(handle.size);
        if(buff == NULL || mask == NULL) {
            if(buff) free(buff);
            if(mask) free(mask);
            fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
            return ERR_ALLOC;
        }
        bzero(buff, handle.size);
        bzero(mask, handle.size);

        points = tcount - 1;
        if(handle.count < points) {
            points = handle.count;
        }
        if(handle.type == DAX_BOOL) use_mask = 1;
        for(n = 0; n < points; n++) {
            if(strncmp(tokens[n+1], "~", 1)) {
                dax_string_to_val(tokens[n + 1], handle.type, buff, mask, n);
            } else {
                use_mask = 1; /* If we skipped any then we need to use the masked write */
            }
        }
        if(use_mask) {
            result = dax_mask_tag(ds, handle, buff, mask);
        } else {
            result = dax_write_tag(ds, handle, buff);
        }
        free(buff);
        free(mask);
    }

    if(result) {
        if(result == ERR_READONLY) {
            fprintf(stderr, "ERROR: Tag %s is read-only\n", name);
        } else {
            fprintf(stderr, "ERROR: Unable to Write to tag %s code = %d\n", name, result);
        }
    }
    return 0;
}

/* adds a compound datatype to the server.
 * Usage:
 * >cdt typename mem1name mem1type mem1count mem2name mem2type mem2count ...
 */
int
cdt_add(char **tokens, int tcount)
{
    dax_cdt *type;
    tag_type memtype;
    int count, n;
    int result;

    if(tcount < 4 || (tcount - 1) % 3 != 0) {
        fprintf(stderr, "ERROR: Wrong number of arguments\n");
        return ERR_ARG;
    }
    type = dax_cdt_new(tokens[0], &result);

    if(type == 0) {
        fprintf(stderr, "ERROR: Unable to create type %s\n", tokens[0]);
        return result;
    }
    for(n = 1; n < tcount - 2; n += 3) {
        memtype = dax_string_to_type(ds, tokens[n+1]);
        count = strtol(tokens[n+2], NULL, 0);
        result = dax_cdt_member(ds, type, tokens[n], memtype, count);

        if(result) {
            fprintf(stderr, "ERROR: Unable to add member %s\n", tokens[n]);
        }
    }

    result = dax_cdt_create(ds, type, NULL);
    if(result) {
        fprintf(stderr, "ERROR: Unable to create type %s\n", tokens[0]);
    }
    return 0;
}

static void
_print_cdt_callback(cdt_iter iter, void *udata)
{
    if(udata == NULL) {
        printf("%s\n", iter.name);
    } else {
        printf("%-32s  %s", iter.name, dax_type_to_string(ds, iter.type));
        if(iter.count > 0) {
            printf("[%d]", iter.count);
        }
        printf("\n");
    }
}


int
list_types(char **tokens)
{
    tag_type type;

    if(tokens[0] == NULL) {
        dax_cdt_iter(ds, 0, NULL, _print_cdt_callback);
    } else {
        type = dax_string_to_type(ds, tokens[0]);
        dax_cdt_iter(ds, type, &type, _print_cdt_callback);
    }
    return 0;
}

int
map_add(char **tokens, int count)
{
    int srcCount, destCount, result;
    tag_handle src, dest;
    if(count < 4) {
        fprintf(stderr, "ERROR: Too few arguments\n");
        return ERR_ARG;
    }
    srcCount = strtol(tokens[1], NULL, 0);
    destCount = strtol(tokens[3], NULL, 0);
    if(srcCount <= 0) {
        fprintf(stderr, "ERROR: Bad count of %s given for %s\n", tokens[1], tokens[0]);
        return ERR_ARG;
    }
    if(destCount <= 0) {
        fprintf(stderr, "ERROR: Bad count of %s given for %s\n", tokens[3], tokens[2]);
        return ERR_ARG;
    }
    result = dax_tag_handle(ds, &src, tokens[0], srcCount);
    if(result < 0) {
        fprintf(stderr, "ERROR: Bad tag %s\n", tokens[0]);
        return ERR_ARG;
    }
    result = dax_tag_handle(ds, &dest, tokens[2], destCount);
    if(result < 0) {
        fprintf(stderr, "ERROR: Bad tag %s\n", tokens[2]);
        return ERR_ARG;
    }
    result = dax_map_add(ds, &src, &dest, NULL);
    if(result) {
        if(result == ERR_READONLY) {
            fprintf(stderr, "ERROR: Destination is read only\n");
        } else {
            fprintf(stderr, "ERROR: Problem Adding Map.  Error %d\n", result);
        }
    }
    return 0;
}

int
map_del(char **tokens, int count)
{
    printf("Map delete not implemented\n");
    return 0;
}

int
map_get(char **tokens, int count)
{
    printf("Map get is not implemented\n");
    return 0;
}

int
override_add(char **tokens, int count)
{
    tag_handle handle;
    int result, n, points;
    char *name;
    void *buff, *mask;

    if(count < 2) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    if(strstr(tokens[1], "\"")) { /* We are assuming this is a string */
        count = strlen(tokens[1]);
        if(tokens[1][0] == '\"') { /* remove the leading " */
            count--;
            tokens[1]++;
        }
        if(tokens[1][count-1] =='\"') { /* remove the trailing " */
            tokens[1][count-1] = '\0';
            count--;
        }
        if(count <= 0) {
            fprintf(stderr, "ERROR: Zero length string argument\n");
            return ERR_ARG;
        }
        result = dax_tag_handle(ds, &handle, name, 0);
        if(result) {
            if(result == ERR_2BIG) {
                fprintf(stderr, "ERROR: %s is too large a request\n", name);
            } else {
                /* TODO: More descriptive error messages here based on result */
                fprintf(stderr, "ERROR: %s Not a Valid Tag\n", name);
            }
            return ERR_ARG;
        }
        if(TYPESIZE(handle.type) != 8) {
            fprintf(stderr, "ERROR: Strings can only be written to 8 bit data types\n");
            return ERR_ARG;
        }
        buff = malloc(handle.size);
        bzero(buff, handle.size);
        memcpy(buff, (void *)tokens[1], MIN(count, handle.size));
        result = dax_tag_add_override(ds, handle, buff);
    } else {
        result = dax_tag_handle(ds, &handle, name, count-1);
        if(result) {
            /* TODO: More descriptive error messages here based on result */
            fprintf(stderr, "ERROR: %s Not a Valid Tag\n", tokens[0]);
            return ERR_ARG;
        }

        buff = malloc(handle.size);
        mask = malloc(handle.size);
        if(buff == NULL || mask == NULL) {
            if(buff) free(buff);
            if(mask) free(mask);
            fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
            return ERR_ALLOC;
        }
        bzero(buff, handle.size);
        bzero(mask, handle.size);

        points = count - 1;
        if(handle.count < points) {
            points = handle.count;
        }
        for(n = 0; n < points; n++) {
            if(strncmp(tokens[n+1], "~", 1)) {
                dax_string_to_val(tokens[n + 1], handle.type, buff, mask, n);
            }
        }
        result = dax_tag_add_override(ds, handle, buff);

        free(buff);
        free(mask);
    }

    if(result) {
        fprintf(stderr, "ERROR: Unable to add override to tag %s code = %d\n", name, result);
    }
    return result;
}

int
override_get(char **tokens, int count)
{
    printf("Get override is not implemented\n");
    return 0;
}

int
override_set(char **tokens, int count)
{
    tag_handle handle;
    int result;
    char *name;

    if(count < 1) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    result = dax_tag_handle(ds, &handle, name, 0);
    if(result) {
        fprintf(stderr, "ERROR: Unable to retrieve tag %s code = %d\n", name, result);
    }

    result = dax_tag_set_override(ds, handle);

    if(result) {
        fprintf(stderr, "ERROR: Unable to set override to tag %s code = %d\n", name, result);
    }
    return result;
}

int
override_clr(char **tokens, int count)
{
    tag_handle handle;
    int result;
    char *name;

    if(count < 1) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    result = dax_tag_handle(ds, &handle, name, 0);
    if(result) {
        fprintf(stderr, "ERROR: Unable to retrieve tag %s code = %d\n", name, result);
    }

    result = dax_tag_clr_override(ds, handle);

    if(result) {
        fprintf(stderr, "ERROR: Unable to clear override to tag %s code = %d\n", name, result);
    }
    return result;
}

int
override_del(char **tokens, int count)
{
    tag_handle handle;
    int result;
    char *name;

    if(count < 1) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    result = dax_tag_handle(ds, &handle, name, 0);
    if(result) {
        fprintf(stderr, "ERROR: Unable to retrieve tag %s code = %d\n", name, result);
    }

    result = dax_tag_del_override(ds, handle);

    if(result) {
        fprintf(stderr, "ERROR: Unable to del override to tag %s code = %d\n", name, result);
    }
    return result;
}

static uint16_t
_get_atomic_op_type(char *op) {
    if(!strncasecmp(op, "inc", 3)) return ATOMIC_OP_INC;
    else if(!strncasecmp(op, "dec", 3)) return ATOMIC_OP_DEC;
    else if(!strncasecmp(op, "not", 3)) return ATOMIC_OP_NOT;
    else if(!strncasecmp(op, "or", 2)) return ATOMIC_OP_OR;
    else if(!strncasecmp(op, "and", 3)) return ATOMIC_OP_AND;
    else if(!strncasecmp(op, "nor", 3)) return ATOMIC_OP_NOR;
    else if(!strncasecmp(op, "nand", 4)) return ATOMIC_OP_NAND;
    else if(!strncasecmp(op, "xor", 3)) return ATOMIC_OP_XOR;
    else if(!strncasecmp(op, "xnor", 3)) return ATOMIC_OP_XOR;
    else  return 0;
}

int
atomic_op(char **tokens, int count)
{
    tag_handle handle;
    int result, n, points;
    uint16_t operation;
    char *name;
    void *buff, *mask;

    if(count < 2) {
        fprintf(stderr, "ERROR: Not enough arguments\n");
        return ERR_ARG;
    }
    if(tokens[0]) {
        name = tokens[0];
    } else {
        fprintf(stderr, "ERROR: No Tagname Given\n");
        return ERR_NOTFOUND;
    }
    result = dax_tag_handle(ds, &handle, name, count - 2);
    if(result) {
        /* TODO: More descriptive error messages here based on result */
        fprintf(stderr, "ERROR: '%s' Not a Valid Tag - %s\n", tokens[0], dax_errstr(result));
        return ERR_ARG;
    }

    operation = _get_atomic_op_type(tokens[1]);
    if(operation == 0) {
        fprintf(stderr, "ERROR: %s Not a Valid operation\n", tokens[1]);
        return ERR_ARG;
    }
    buff = malloc(handle.size);
    mask = malloc(handle.size);
    if(buff == NULL || mask == NULL) {
        if(buff) free(buff);
        if(mask) free(mask);
        fprintf(stderr, "ERROR: Unable to Allocate Memory\n");
        return ERR_ALLOC;
    }
    bzero(buff, handle.size);
    bzero(mask, handle.size);

    if(tokens[2] == NULL) { /* No value argument given */
        if(operation == ATOMIC_OP_INC || operation == ATOMIC_OP_DEC) {
            /* We only operate on one member at a time */
            switch(handle.type) {
                case DAX_BYTE:
                    *(dax_byte *)buff = 1;
                    return 0;
                case DAX_SINT:
                case DAX_CHAR:
                    *(dax_sint *)buff = 1;
                    return 0;
                case DAX_UINT:
                case DAX_WORD:
                    *(dax_uint *)buff = 1;
                    return 0;
                case DAX_INT:
                    *(dax_int *)buff = 1;
                    return 0;
                case DAX_UDINT:
                case DAX_DWORD:
                case DAX_TIME:
                    *(dax_udint *)buff = 1;
                    return 0;
                case DAX_DINT:
                    *(dax_dint *)buff = 1;
                    return 0;
                case DAX_ULINT:
                case DAX_LWORD:
                    *(dax_ulint *)buff = 1;
                    return 0;
                case DAX_LINT:
                    *(dax_lint *)buff = 1;
                    return 0;
                case DAX_REAL:
                    *(dax_real *)buff = 1.0;
                    return 0;
                case DAX_LREAL:
                    *(dax_lreal *)buff = 1.0;
                    return 0;
                default:
                    fprintf(stderr, "ERROR: Bad type");
                    free(buff);
                    free(mask);
                    return -1;
            }
        } else if(operation != ATOMIC_OP_NOT) {
            fprintf(stderr,"ERROR: Value required for '%s' operation", tokens[1]);
            free(buff);
            free(mask);
            return -1;
        }
    } else {
        points = count - 2;
        if(handle.count < points) {
            points = handle.count;
        }
        for(n = 0; n < points; n++) {
            if(strncmp(tokens[n+1], "~", 1)) {
                dax_string_to_val(tokens[n + 2], handle.type, buff, mask, n);
            }
        }
    }

    result = dax_atomic_op(ds, handle, buff, operation);
    free(buff);
    free(mask);

    if(result) {
        fprintf(stderr, "ERROR: Atomic operation failed on tag %s - %s\n", name, dax_errstr(result));
    }
    return result;

}