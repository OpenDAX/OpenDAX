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
 *  source code file for the database commands OpenDAX Command Line Client  module
 *
 *  The DB commands in daxc replicate the low level OpenDAX read / write functions
 */

#include <daxc.h>
#include <ctype.h>

#define FMT_HEX 0x00
#define FMT_DEC 0x01
#define FMT_CHAR 0x02

static int format;
static int width = 8;
static int dsize = 8;

/* Examples:
   DB READ tagname / handle [count]
   DB READBIT tagname / handle [count]
   DB WRITE tagname / handle [count]
   DB WRITEBIT tagname / handle [count]
   DB WRITEMASK ?? - still dont' know how to do the mask
   Tagnames are converted to handles and no data type.  I guess there
   is a little bug here because now we can't name a tag 'bit'
 */

/* This is just an internal function for the db_read* functions to retrieve the
   starting handle and the count (in bits) of the request */
static int _db_read_getparams(handle_t *handle, size_t *count, char bitsorbytes) {
    dax_tag tag;
    char tag_given = 0;
    char *tok;
    
    tok = strtok(NULL, " ");
    if(tok == NULL) {
        fprintf(stderr,"ERROR: Handle not given\n");
        return 1;
    }
    if(isdigit(tok[0])) {
        *handle = strtol(tok, NULL, 0);
    } else {
        if( dax_tag_byname(tok, &tag) ) {
            fprintf(stderr, "ERROR: Bad Tagname %s\n", tok);
            return 1;
        }
        
        tag_given = 1;
        *handle = tag.handle;
    }
    /* Get the count next.  If not count then  the count will be either the size
     of the tag if the tag was given.  If not then we just make it one byte. */
    tok = strtok(NULL, " ");
    if(tok == NULL) { /* No count given */
        if( tag_given ) {
            *count = tag.count * TYPESIZE(tag.type);
            if(bitsorbytes) *count /= 8;
        } else {
            *count = 1;
        }
    } else {
        *count = strtol(tok, NULL, 0);
    }
    printf("Returning handle = 0x%lX, count = %d\n", *handle, *count);
    return 0;
}

/* Abstracts the db_read_bits() and db_read_bit() dax library function calls */
int db_read_bit(void) {
    handle_t handle;
    void *buff;
    size_t count, n;
    
    if(_db_read_getparams(&handle, &count, 0)) {
        return 1;
    }
    
    /* Allocate the buffer on the stack */
    buff = alloca(count/8 +1);
    handle = handle / 8 * 8;
    /* Read the data */
    if(count == 1) {
        /* TODO: Error check here */
        ((u_int8_t *)buff)[0] = dax_tag_read_bit(handle);
    } else {
        dax_tag_read_bits(handle, buff, count);
    }
    
    /* Display the data */
    /* TODO: Format option for MSB first or LSB first: How to display??? */
    for(n = 0; n < count; n++) {
        if(n % width == 0) {
            if(n > 0) printf("\n");
            printf("0x%lX - ", handle + n);
        }
        if( ((u_int8_t *)buff)[n / 8] & (1 << (n % 8)) ) {
            printf("1 ");
        } else {
            printf("0 ");
        }
    }
    printf("\n");
    return 0;
}

/* Abstracts the db_read_bytes() dax library function call */
int db_read(void) {
    handle_t handle;
    void *buff;
    size_t count, n;

    if(_db_read_getparams(&handle, &count, 1)) {
        return 1;
    }
    
    printf("Count = %d\n", count);
    /* Allocate the buffer on the stack */
    buff = alloca(count);

    handle = handle / 8 * 8;
    
    /* Read the data */
    dax_read(handle, 0, buff, count);
    
    /* Display the data */
    /* TODO: Use the dsize format to display here */
    for(n = 0; n < count; n++) {
        if(n % width == 0) {
            if(n > 0) printf("\n");
            printf("0x%lX - ", handle + n*8);
        }
        if(format == FMT_DEC) {
            printf("%4d",((u_int8_t *)buff)[n]);
        } else if(format == FMT_CHAR) {
            printf(" '%1c'",((u_int8_t *)buff)[n]);
        } else { /* HEX */
            printf("  0x%02X",((u_int8_t *)buff)[n]);
        }
    }
    printf("\n");
    return 0;
}


/* Encapsulate the db_write_bytes() dax library function call */
int db_write(void) {
    /* If number then get the number and use format to write?? */
    /* If ASCII treat it like a string of bytes and write them?? */
    /* If HEX see how many digits follow 0x and set the appropriate */
    printf("Okay we write now\n");
    return 0;
}

/* The DB FORMAT command allows the user to change the format that the
   data base is displayed in.  */
int db_format(void) {
    char *tok;
    int oldsize;
    
    tok = strtok(NULL, " ");
    if(tok == NULL) {
        /* TODO: Maybe print the current formatting here instead */
        printf("Format: ");
        if(format == FMT_HEX) {
            printf("HEX\n");
        } else if(format == FMT_DEC) {
            printf("DEC\n");
        } else if(format == FMT_CHAR) {
            printf("CHAR\n");
        }
        printf("Width:  %d\n", width);
        printf("Size:   %d\n", dsize);
        //--fprintf(stderr, "ERROR: No Format Given\n");
        return 0;
    /* How to display the data */
    } else if(!strcasecmp(tok, "hex")) {
        format = FMT_HEX;
    } else if(!strcasecmp(tok, "dec")) {
        format = FMT_DEC;
    } else if(!strcasecmp(tok, "char")) {
        format = FMT_CHAR;
    /* width is how many columns will be displayed */
    } else if(!strcasecmp(tok, "width")) {
        tok = strtok(NULL, " ");
        if(tok == NULL) {
            fprintf(stderr,"ERROR: No Width Given\n");
            return 1;
        }
        width = strtoul(tok, NULL, 0);
        if(width <= 0) width = 1;
    /* Size sets the size that each token in a write command will represent in bits */
    } else if(!strcasecmp(tok, "size")) {
        oldsize = dsize;
        tok = strtok(NULL, " ");
        if(tok == NULL) {
            fprintf(stderr,"ERROR: No Data Size Given\n");
            return 1;
        }
        /* TODO: Probably should implement all the data types here */
        if(!strcasecmp(tok, "bit")) {
            dsize = 1;
        } else if(!strcasecmp(tok, "byte")) {
            dsize = 8;
        } else if(!strcasecmp(tok, "word")) {
            dsize = 16;
        } else {
            dsize = strtoul(tok, NULL, 0);
            if(dsize <= 0) dsize = 1;
            if(dsize != 1 && dsize != 8 && dsize != 16 && dsize != 32 && dsize != 64) {
                fprintf(stderr, "ERROR: Given size is not correct\n");\
                dsize = oldsize;
                return 1;
            }
        }
    } else {
        fprintf(stderr, "ERROR: Unknown Format - %s\n", tok);
        return 1;
    }
    return 0;
}