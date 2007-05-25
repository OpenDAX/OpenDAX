/*  opendcs - An open source distributed control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 
 * This is the source file for the tagname database handling routines
 */

#include <tagbase.h>
#include <func.h>
//#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Notes:
 * The tagname symbol table list is implemented as a simple array that
 * is allocated at run time.  This array starts as a fixed size and
 * can be increased in size by a predetermined increment once the original
 * array is filled.  The array is sorted by the handle so that it can be used
 * in memory allocation.
 *
 * The database is implemented as a large block of 32 bit unsigned integers.
 * It is really nothing more than a memory area for storing tags.  The
 * functions that access this database will worry about types.  This allows
 * the database to be as small as possible.  There are several different
 * datatypes (IEC61131) and are 1,8,16,32 or 64 bits in length.  The lower
 * five bits of the handle point to a bit offset within this database and
 * the rest of the handle points to the UINT location.  Single bits can be
 * located anywhere in the database.  8 and 16 bit numbers will be at even
 * 8 or 16 bit offsets and 32 and 64 bit numbers will always start at zero.
 * This may create some fragmentation but it'll make the logic simpler.
 */

dcs_tag *__taglist;
static long int __tagindex=0;
static long int __taglistsize=0;

u_int32_t *__db; /* Primary Point Database */
static int __databasesize=0;

/* Non public function declarations */
static int validate_name(char *);
static long int get_by_name(char *);
static int checktype(unsigned int);
static long int gethandle(unsigned int, unsigned int);
/*static long int gethandle_8(unsigned int);
static long int gethandle_16(unsigned int);
static long int gethandle_32(unsigned int);
static long int gethandle_64(unsigned int);*/
static int taglist_grow(void);
static int database_grow(void);

/* Allocates the symbol table and the database array */
void initialize_tagbase(void) {
    __taglist=(dcs_tag *)xmalloc(sizeof(dcs_tag)*DCS_TAGLIST_SIZE);
    if(!__taglist) {
        xfatal("Unable to allocate the symbol table");
    }
    __taglistsize=DCS_TAGLIST_SIZE;
    /* Allocate the primary database */
    __db=(u_int32_t *)xmalloc(sizeof(u_int32_t)*DCS_DATABASE_SIZE);
    if(!__db) {
        xfatal("Unable to allocate the database");
    }
    __databasesize=DCS_DATABASE_SIZE;
    /* Set all the memory to zero */
    memset(__taglist,0x00,sizeof(dcs_tag)*DCS_TAGLIST_SIZE);
    memset(__db,0x00,sizeof(u_int32_t)*DCS_DATABASE_SIZE);
}


/* This adds a tag to the database. */
long int tag_add(char *name,unsigned int type, unsigned int count) {
    long handle;
    if(__tagindex >= __taglistsize) {
        if(!taglist_grow()) {
            xerror("Out of memory for symbol table");
            return -1;
        }
    }
    if(!checktype(type)) {
        xerror("Unknown datatype %x",type);
        return -2; /* is the datatype valid */
    }
    if(validate_name(name)) {
        xerror("%s is not a valid tag name");
        return -3;
    }
    if(get_by_name(name)>=0) {
        xerror("Duplicate tag name %s",name);
        return -4;
    }
    /* Get the first available handle for the size of the type */
    if(__tagindex==0)   handle = 0;
    else                handle=gethandle(type,count);
    if(handle<0) {
        xerror("Problem allocating room");
        return -5;
    }
    /* Assign everything to the new tag, copy the string and git */
    __taglist[__tagindex].handle=handle;
    __taglist[__tagindex].count=count;
    __taglist[__tagindex].type=type;
    if(!strncpy(__taglist[__tagindex].name,name,DCS_TAGNAME_SIZE)) {
        xerror("Unable to copy tagname %s");
        return -6;
    }
    __tagindex++;
    return handle;
}

/* TODO: Make this function do something */
int tag_del(char *name) {
    return 0; /* Return good for now */
}

/* Determine whether or not the tag name is okay */
/* TODO: Make this function do something */
static int validate_name(char *name) {
    return 0; /* Return good for now */
}


/* This function retrieves the index of the tag identified by name */
static long int get_by_name(char *name) {
    long int i;
    for(i=0;i<__taglistsize;i++) {
        if(!strcmp(name,__taglist[i].name))
            return i;
    }
    return -1;
}

/* checks whether type is a valid datatype */
static int checktype(unsigned int type) {
    if(type & DCS_BOOL)  return 1;
    if(type & DCS_BYTE)  return 1;
    if(type & DCS_SINT)  return 1;
    if(type & DCS_WORD)  return 1;
    if(type & DCS_INT)   return 1;
    if(type & DCS_UINT)  return 1;
    if(type & DCS_DWORD) return 1;
    if(type & DCS_DINT)  return 1;
    if(type & DCS_UDINT) return 1;
    if(type & DCS_TIME)  return 1;
    if(type & DCS_REAL)  return 1;
    if(type & DCS_LWORD) return 1;
    if(type & DCS_LINT)  return 1;
    if(type & DCS_ULINT) return 1;
    if(type & DCS_LREAL) return 1;
    /* TODO: Should add custom datatype handling here */
    return 0;
}



/* Find the next handle for a single bit or bit array */
static long int gethandle(unsigned int type,unsigned int count) {
    int n,size;
    dcs_tag *this,*next;
    long nextbit;
    /* The lower four bits of type are the size in 2^n format */
    size = 0x01 << (type & 0x0F);
    for(n=0;n<__taglistsize-2;n++) {
        this=&__taglist[n]; next=&__taglist[n+1]; /* just to make it easier */
        /* this is the handle of the bit following this bits allocation */
        nextbit=this->handle + (this->count * (0x01 << (this->type & 0x0F)));
        
        if((type & DCS_1BIT) && ((next->handle - nextbit) > (size * count))) {
            return nextbit;
        /* The database is an array of 32bit numbers this code sets the nextbit
            handle to the next even 8,16 0r 32 bit offset within the database */
        } else if((type & DCS_8BITS)) {
            if((nextbit & 0x07) == 0x00) { /* Even 8 bit number */
                return nextbit;
            }
            nextbit |= 0x07; /* turn last three bits to 1's */
            nextbit++;      /* increment to the next even number */
        } else if((type & DCS_16BITS)) {
            if((nextbit & 0x0F) == 0x00) { /* Even 16 bit number */
                return nextbit;
            }
            nextbit |= 0x0F; /* turn last four bits to 1's */
            nextbit++;      /* increment to the next even number */
        } else if((type & DCS_32BITS) || (type & DCS_64BITS)) {
            if((nextbit & 0x1F) == 0x00) { /* Even 32 bit number */
                return nextbit;
            }
            nextbit |= 0x1F; /* turn last five bits to 1's */
            nextbit++;      /* increment to the next even number */
        }
        /* Now see if we have enough room */
        if((next->handle - nextbit) > (size * count)) {
            return nextbit;
        }
    }
    /* We are at the end of the loops so there are no gaps large enough for our
        new point.  We need to make sure that there is enough room left the 
        database, grow it if necessary, and assign the handle to the end */
    this=&__taglist[__taglistsize-1]; /* Get the last tag in the list */
    nextbit=this->handle + (this->count * (0x01 << (this->type & 0x0F)));
    if(nextbit > (__databasesize * 32)) { /* We need more room */
        if(!database_grow()) { /* Make the database bigger */
            xerror("Unable to allocate more database memory");
            return -1;
        }
    }
    return nextbit;
}

/* Grow the tagname database when necessary */
static int taglist_grow(void) {
    return 0; /* Just return error for now */
}

/* Grow the database when necessary */
static int database_grow(void) {
    return 0; /* Just return error for now */
}
