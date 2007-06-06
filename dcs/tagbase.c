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
#include <stdio.h>
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
static long int __tagcount=0;
static long int __taglistsize=0;

u_int32_t *__db; /* Primary Point Database */
static int __databasesize=0;

/* Non public function declarations */
static int validate_name(char *);
static int get_by_name(char *);
static int checktype(unsigned int);
static inline handle_t gethandle(unsigned int, unsigned int);
static int taglist_grow(void);
static int database_grow(void);

/* Allocates the symbol table and the database array.  There's no return
   value because failure of this function is fatal */
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
handle_t tag_add(char *name,unsigned int type, unsigned int count) {
    int n;
    handle_t handle;
    if(__tagcount >= __taglistsize) {
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
    if(__tagcount==0)   handle = 0;
    else                handle=gethandle(type,count);
    if(handle<0) {
        xerror("Problem allocating room");
        return -5;
    }
    /* The taglist must be sorted by handle for all of this stuff to work */
    if((handle > __taglist[__tagcount-1].handle) || __tagcount==0) {
        n=__tagcount; /* First or last in the list */
    } else {
        for(n=1;n<__tagcount;n++) {
            if(handle > __taglist[n-1].handle && handle < __taglist[n].handle) {
                /* Make a hole in the array */
                memmove(&__taglist[n+1],&__taglist[n],
                       (__taglistsize-n-2)*sizeof(dcs_tag));
                break;
            }
        }
    }
    /* Assign everything to the new tag, copy the string and git */
    __taglist[n].handle=handle;
    __taglist[n].count=count;
    __taglist[n].type=type;
    if(!strncpy(__taglist[n].name,name,DCS_TAGNAME_SIZE)) {
        xerror("Unable to copy tagname %s");
        return -6;
    }
    __tagcount++;
    return handle;
}

/* Find the next handle for a single bit or bit array.  It starts by
   running through the __taglist[] and looking to see if it can find
   any room between the existing data points.  It does this by figuring
   the size of the new tag and if it's a bit it does a simple subtraction
   if it's bigger than a byte then it rounds the number up to it's proper
   place.  Having the larger datatypes aligned will help with reading and
   writing of the data later.  If it can't find a gap then it'll put the
   handle at the end of the database and then check to see if the database
   is big enough.  If not it grows the database.

   This function assumes that the taglist is sorted by handle lowest first

   TODO: May need to rewrite this thing to accept a size instead of a
         datatype.  That way it could handle custom datatypes later.
         Or may have to write another for custom datatypes.
*/
static inline handle_t gethandle(unsigned int type,unsigned int count) {
    int n,size,mask;
    dcs_tag *this,*next;
    handle_t nextbit;
    /* The lower four bits of type are the size in 2^n format */
    size = TYPESIZE(type);
    for(n=0;n<__tagcount-1;n++) {
        this=&__taglist[n]; next=&__taglist[n+1]; /* just to make it easier */
        /* this is the handle of the bit following this bits allocation */
        nextbit = this->handle + (long)(this->count * (0x01 << (this->type & 0x0F)));
        /* If it's not a single bit type then we need to even up nextbit
            We do this by first calculating a mask to see if we are already
            even and if we are then return.  If not then set the mask bits into
            nextbit and then increment by one to get the even number */
        if((type & 0x0F) == DCS_1BIT) {
            if((next->handle - nextbit) >= (long)(size * count)) {
                return nextbit;
            }
        } else {
            mask=TYPESIZE(type)-1;
            if((nextbit & mask) != 0x00) { /* If not even number */
                nextbit |= mask; /* Round up */
                nextbit ++;
            }
            /* Now see if we have enough room */
            if((next->handle - nextbit) >= (long)(size * count)) {
                return nextbit;
            }
        }
    }
    /* We are at the end of the loops so there are no gaps large enough for our
        new point.  We need to make sure that there is enough room left the 
        database, grow it if necessary, and assign the handle to the end */

    this=&__taglist[__tagcount-1]; /* Get the last tag in the list */
    nextbit=this->handle + (this->count * TYPESIZE(this->type));
    
    if((type & 0x0F) != DCS_1BIT) {/* If it's not a single bit */
        /* ...then even it up */
        mask=(0x0001 << (type & 0x0f))-1;
        if((nextbit & mask) == 0x00) {
            return nextbit;
        }
        nextbit |= mask;
        nextbit ++;
    }
    
    if(nextbit > (__databasesize * 32)) { /* We need more room */
        if(!database_grow()) { /* Make the database bigger */
            xerror("Unable to allocate more database memory");
            return -1;
        }
    }
    return nextbit;
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
static int get_by_name(char *name) {
    int i;
    for(i=0;i<__tagcount;i++) {
        if(!strcmp(name,__taglist[i].name))
            return i;
    }
    return -1;
}

/* This function searches the taglist for the handle.  It uses a bisection
   search and returns the array index of the point that contains the handle.
   The handle may be the handle of a bit within the tag point too. */
static int get_by_handle(handle_t handle) {
    int lime,try,high,low;
    low=lime=0;
    high=__tagcount-1;
    try=high/2;
    if(handle < 0) {
        return -1; /* handle does not exist */
    } else if(handle >= __taglist[__tagcount-1].handle) {
        try=__tagcount-1; /* if the given handle is beyond the array */
    } else {
        while(!(handle > __taglist[try].handle && handle < __taglist[try+1].handle)) {
            try=low+(high-low)/2;
            if(__taglist[try].handle < handle) {
                low=try;
            } else if(__taglist[try].handle > handle) {
                high=try;
            } else {
                return try;
            }
        }
    }
    
    if(handle < __taglist[try].handle + 
             (TYPESIZE(__taglist[try].type) * __taglist[try].count)) {
        return try;
    } else {
        return -1;
    }
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

/* Grow the tagname database when necessary */
static int taglist_grow(void) {
    xlog(8,"Growing the size of the taglist");
    return 0; /* Just return error for now */
}

/* Grow the database when necessary */
static int database_grow(void) {
    xlog(8,"Growing the size of the database");
    return 0; /* Just return error for now */
}

/* Get's a handle from a tagname */
handle_t tag_get_handle(char *name) {
    int x;
    x=get_by_name(name);
    if(x>=0) return __taglist[x].handle;
    else return x;
}

/* Retrieves the type of the point at the given handle */
int tag_get_type(handle_t handle) {
    int x;
    x=get_by_handle(handle);
    if(x<0) return x;
    else return __taglist[x].type;
}

/* This function reads the data starting with handle.  Size is in
 * bytes.  If *data doesn't have enough room we'll be in big trouble.
 * basically this thing shifts handle 3 bits right to make handle a byte
 * index into the __db.  Then we cast __db to a byte and add handle to that
 * to get a pointer to the data that we want.  Then a good 'ol memcpy does 
 * the trick.
 */
int tag_read_bytes(handle_t handle, void *data,size_t size) {
    u_int8_t *src;
    handle /= 8;
    /* Make sure we don't overflow the database */
    if((__databasesize*4)-handle < size) {
        return -1;  /* asking for too much data */
    }
    src=(u_int8_t *)__db; /* Cast __db to byte and set src */
    src+=handle; /* index dest */
    if(memcpy(data,src,size)==NULL) {
        return -2;
    }
    return size;
}

/* This function writes data to the __db just like the above function reads it */
int tag_write_bytes(handle_t handle, void *data, size_t size) {
    u_int8_t *dest;
    handle /= 8; /* ditch the bottom three bits */
    if((__databasesize*4)-handle < size) {
        return -1; /* Whoa don't overflow my buffer */
    }
    dest=(u_int8_t *)__db; /* Cast __db to byte */
    dest += handle;
    if(memcpy(dest,data,size)==NULL) {
        return -2;
    }
    return size;
}

/* TODO: Make this function do something */
int tag_mask_write(handle_t handle, void *data, void *mask, size_t size) {
    return -1; /* just return error for now */
}


/* mostly for debugging */
void tags_list(void) {
    int n;
    for(n=0;n<__tagcount;n++) {
        xlog(0,"%s handle = (%ld,0x%lx) count = %d",__taglist[n].name,
             __taglist[n].handle,__taglist[n].handle,__taglist[n].count);
    }
}

void print_database(void) {
    int n;
    for(n=0;n<20;n++) {
        printf("0x%08x\n",__db[n]);
    }
}
