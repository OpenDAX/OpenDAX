/*  OpenDAX - An open source data acquisition and control system 
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

#include <common.h>
#include <tagbase.h>
#include <func.h>
#include <ctype.h>
#include <string.h>

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

static _dax_tag_db *__db;
static _dax_tag_index *__index;
static long int __tagcount = 0;
static long int __dbsize = 0;

//u_int32_t *__db; /* Primary Point Database */
//static long int __databasesize = 0;

static int __next_event_id = 0;

/* Private function definitions */

/* checks whether type is a valid datatype */
static int
_checktype(unsigned int type)
{
    if(type & DAX_BOOL)  return 1;
    if(type & DAX_BYTE)  return 1;
    if(type & DAX_SINT)  return 1;
    if(type & DAX_WORD)  return 1;
    if(type & DAX_INT)   return 1;
    if(type & DAX_UINT)  return 1;
    if(type & DAX_DWORD) return 1;
    if(type & DAX_DINT)  return 1;
    if(type & DAX_UDINT) return 1;
    if(type & DAX_TIME)  return 1;
    if(type & DAX_REAL)  return 1;
    if(type & DAX_LWORD) return 1;
    if(type & DAX_LINT)  return 1;
    if(type & DAX_ULINT) return 1;
    if(type & DAX_LREAL) return 1;
    /* TODO: Should add custom datatype handling here */
    return 0;
}

/* Determine whether or not the tag name is okay */
static int
_validate_name(char *name)
{
    int n;
    if(strlen(name) > DAX_TAGNAME_SIZE) {
        return -1;
    }
    /* First character has to be a letter or '_' */
    if( !isalpha(name[0]) && name[0] != '_' ) {
        return -1;
    }
    /* The rest of the name can be letters, numbers or '_' */
    for(n = 1; n < strlen(name) ;n++) {
        if( !isalpha(name[n]) && (name[n] != '_') &&  !isdigit(name[n]) ) {
            return -1;
        }
    }
    return 0; /* Return good for now */
}

/* This function searches the __index array to find the tag with
   the given name.  It returns the index into the __index array */
static int
_get_by_name(char *name) 
{
    /* TODO: Better search algorithm */
    int i;
    for(i = 0; i < __tagcount; i++) {
        if(!strcmp(name, __index[i].name))
            return i;
    }
    return ERR_NOTFOUND;
}

/* Grow the database when necessary */
static int
_database_grow(void)
{
    _dax_tag_index *new_index;
    _dax_tag_db *new_db;
    
    new_index = xrealloc(__index, (__dbsize + DAX_DATABASE_INC) * sizeof(_dax_tag_index));
    new_db = xrealloc(__db, (__dbsize + DAX_DATABASE_INC) * sizeof(_dax_tag_db));
    
    if( new_index != NULL && new_db != NULL) {
        __index = new_index;
        __db = new_db;
        __dbsize += DAX_DATABASE_INC;
        return 0;
    } else {
        /* This is to shrink the one that didn't get allocated
           so that they won't be lop sided */
        if(new_index) __index = xrealloc(__index, __dbsize * sizeof(_dax_tag_index));
        if(new_db) __db = xrealloc(__db, __dbsize * sizeof(_dax_tag_db));
        
        return ERR_ALLOC;
    }
}


/* Allocates the symbol table and the database array.  There's no return
   value because failure of this function is fatal */
void
initialize_tagbase(void)
{
    __db = xmalloc(sizeof(_dax_tag_db) * DAX_TAGLIST_SIZE);
    if(!__db) {
        xfatal("Unable to allocate the database");
    }
    __dbsize = DAX_TAGLIST_SIZE;
    /* Allocate the primary database */
    __index = (_dax_tag_index *)xmalloc(sizeof(_dax_tag_index) * DAX_TAGLIST_SIZE);
    if(!__db) {
        xfatal("Unable to allocate the database");
    }
    
    xlog(LOG_MINOR, "Database created with size = %d", __dbsize);
    /* Set all the memory to zero */
    //--memset(__taglist, 0x00, sizeof(_dax_tag) * DAX_TAGLIST_SIZE);
    //--memset(__db, 0x00, sizeof(u_int32_t) * DAX_DATABASE_SIZE);
    
    /* Create the _status tag at handle zero */
    if(tag_add("_status", DAX_DWORD, STATUS_SIZE)) {
        xfatal("_status not created properly");
    }
    /* Write the current size of the database to the _status tag */
    //tag_write_bytes(STAT_DB_SIZE, &__databasesize, sizeof(u_int32_t));
}

static int
_add_index(char *name, int index)
{
    int n;
    char *temp;
    
    /* Let's allocate the memory for the string first in case it fails */
    temp = strdup(name);
    if(temp == NULL) return ERR_ALLOC;
    /* TODO: Linear search for now.  This needs to be a binary search */
    if(__tagcount == 0) {
        n = 0;
    } else { 
        for(n = 1; n < __tagcount; n++) {
            if(strcmp(__index[n].name, name) > 0) {
                memmove(&__index[n + 1], &__index[n],(__dbsize - n - 1) * sizeof(_dax_tag_index));
                break;
            }
        }
    }
    /* Assign pointer to database node in the index */
    __index[n].handle = index;
    /* The name pointer in the __index and the __db point to the same string */
    __index[n].name = temp;
    __db[index].name = temp;
    return 0;
}


/* This adds a tag to the database. */
handle_t
tag_add(char *name, unsigned int type, unsigned int count)
{
    int n;
    unsigned int size;
    if(count == 0) return ERR_ARG;

    //--printf("tag_add() called with name = %s, type = %d, count = %d\n", name, type, count);
    if(__tagcount >= __dbsize) {
        if(_database_grow()) {
            xerror("Failure to increae database size");
            return ERR_ALLOC;
        } else {
            xlog(LOG_MINOR, "Database increased to %d items", __dbsize);
        }
    }
    
    if(!_checktype(type)) {
        xerror("Unknown datatype %x", type);
        return ERR_ARG; /* is the datatype valid */
    }
    if(_validate_name(name)) {
        xerror("%s is not a valid tag name", name);
        return ERR_TAG_BAD;
    }
    
    /* Check for an existing tagname in the database */
    if( (n = _get_by_name(name)) >= 0) {
        /* If the tag is identical then just return the handle */
        if(__db[n].type == type && __db[n].count == count) {
            return n;
        } else {
            xerror("Duplicate tag name %s", name);
            return ERR_TAG_DUPL;
        }    
    } else {
        n = __tagcount;
    }
 /* Assign everything to the new tag, copy the string and git */
    //__taglist[n].handle = handle;
    printf("Tag to be added at index %d\n", n);
    __db[n].count = count;
    __db[n].type = type;
    
    /* Figure the size in bytes */
    if(type == DAX_BOOL) size = count / 8 + 1;
    else                 size = (TYPESIZE(type) / 8) * count;
    /* Allocate the data area */
    if((__db[n].data = xmalloc(size)) == NULL)
        return ERR_ALLOC;
    else
        bzero(__db[n].data, size);
    
    __db[n].events = NULL;
    
    if(_add_index(name, n)) {
        /* free up our previous allocation if we can't put this in the __index */
        free(__db[n].data);
        return ERR_ALLOC;
    }
    /* Only if everything works will we increment the count */
    __tagcount++;
    /* TODO: replace this with a real time build of the status tag when it's requested */    
    //tag_write_bytes(STAT_TAG_CNT, &__tagcount, sizeof(u_int32_t));
    
    xlog(LOG_MINOR, "tag_add() - Tag %s added at handle 0x%X", name, n);
    return n;
}


/* TODO: Make this function do something.  We don't want to move the tags up
   in the array so this function will have to leave a hole.  We'll have to mark
   the hole somehow. */
int
tag_del(char *name)
{
    /* TODO: No deleting handle 0x0000 */
    return 0; /* Return good for now */
}


/* Finds a tag based on it's name.  Basically just a wrapper for get_by_name().
 Returns a pointer to the tag given by 'name' NULL if not found */
int tag_get_name(char *name, dax_tag *tag) {
    int i, handle;
    
    i = _get_by_name(name);
    if( i < 0) {
        return ERR_NOTFOUND;
    } else {
        handle = __index[i].handle;
        tag->handle = handle;
        tag->type = __db[handle].type;
        tag->count = __db[handle].count;
        strcpy(tag->name, __db[handle].name);
        return 0;
    }
}

/* Finds a tag based on it's index in the array.  Returns a pointer to 
 the tag give by index, NULL if index is out of range.  Incex should
 not be assumed to remain constant throughout the programs lifetiem. */
int tag_get_index(int index, dax_tag *tag) {
    if(index < 0 || index >= __tagcount) {
        return ERR_ARG;
    } else {
        tag->handle = index;
        tag->type = __db[index].type;
        tag->count = __db[index].count;
        strcpy(tag->name, __db[index].name);
        return 0;
    }
}


#ifdef DELETE_ALL_THIS_STUFF_ERIUEBVIUWEIUGASDJBFVIERW

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
 
 TODO: Will have to be rewritten for custom datatypes.  The size of a custom
 datatype will not be as simple as the TYPESIZE() macro.
 */
static inline handle_t getnewhandle(unsigned int type, unsigned int count) {
    int n, size, mask;
    _dax_tag *this, *next;
    handle_t nextbit;
    /* The lower four bits of type are the size in 2^n format */
    size = TYPESIZE(type);
    for(n = 0; n < __tagcount - 1; n++) {
        this = &__taglist[n]; next = &__taglist[n + 1]; /* just to make it easier */
        /* this is the handle of the bit following this bits allocation */
        nextbit = this->handle + (long)(this->count * TYPESIZE(this->type));
        
        /* If it's not a single bit type then we need to even up nextbit
         We do this by first calculating a mask to see if we are already
         even and if we are then return.  If not then set the mask bits into
         nextbit and then increment by one to get the even number */
        if((type & 0x0F) == DAX_1BIT) { /* bits can go anywhere */
            if((next->handle - nextbit) >= (long)(size * count)) {
                return nextbit;
            }
        } else { /* Other types need to be aligned with memory to make other function easier */
            mask = TYPESIZE(type) - 1; /* Lower bits will be ones */
            if((nextbit & mask) != 0x00) { /* If not even number */
                nextbit |= mask; /* Make the lower bits ones */
                nextbit ++; /* then add one to get to the even number */
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
    
    this = &__taglist[__tagcount - 1]; /* Get the last tag in the list */
    nextbit = this->handle + (this->count * TYPESIZE(this->type));
    
    if((type & 0x0F) != DAX_1BIT) {/* If it's not a single bit */
        /* ...then even it up */
        mask = TYPESIZE(type) - 1;
        if((nextbit & mask) != 0x00) {
            nextbit |= mask;
            nextbit ++;
        }
    }
    if((nextbit + (TYPESIZE(type) * count)) > (__databasesize * 32)) { /* We need more room */
        if(database_grow()) { /* Make the database bigger */
            xerror("gethandle() - Unable to allocate more database memory");
            return -1;
        } else {
            xlog(8,"Increasing size of database to %d wordss", __databasesize);
        }
    }
    return nextbit;
}



/* This function searches the taglist for the handle.  It uses a bisection
   search and returns the array index of the tag that contains the handle.
   The handle may be the handle of a bit within the tag too. */
static int get_by_handle(handle_t handle) {
    int try, high, low;
    low = 0;
    high = __tagcount-1;
    try = high / 2;
    if(handle < 0) {
        return -1; /* handle does not exist */
    } else if(handle >= __taglist[__tagcount-1].handle) {
        try = __tagcount - 1; /* if the given handle is beyond the array */
    } else {
        while(!(handle > __taglist[try].handle && handle < __taglist[try+1].handle)) {
            try = low + (high-low) / 2;
            if(__taglist[try].handle < handle) {
                low = try;
            } else if(__taglist[try].handle > handle) {
                high = try;
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






/* Get's a handle from a tagname */
handle_t tag_get_handle(char *name) {
    int x;
    x = get_by_name(name);
    if(x >= 0) return __taglist[x].handle;
    else return x;
}




/* This function reads the data starting with handle.  Size is in
 * bytes.  If *data doesn't have enough room we'll be in big trouble.
 * basically this thing shifts handle 3 bits right to make handle a byte
 * index into the __db.  Then we cast __db to a byte and add handle to that
 * to get a pointer to the data that we want.  Then a good 'ol memcpy does 
 * the trick.
 */
int tag_read_bytes(handle_t handle, void *data, size_t size) {
    u_int8_t *src;
    handle /= 8;
    /* Make sure we don't overflow the database */
    if((__databasesize*4)-handle < size) {
        return -1;  /* asking for too much data */
    }
    src = (u_int8_t *)__db; /* Cast __db to byte and set src */
    src += handle; /* index dest */
    if(memcpy(data, src, size)==NULL) {
        return -2;
    }
    return size;
}

/* This function writes data to the __db just like the above function reads it */
int tag_write_bytes(handle_t handle, void *data, size_t size) {
    u_int8_t *dest;
    handle /= 8; /* ditch the bottom three bits */
    if((__databasesize * 4) - handle < size) {
        return -1; /* Whoa don't overflow my buffer */
    }
    dest = (u_int8_t *)__db; /* Cast __db to byte */
    dest += handle;
    if(memcpy(dest, data, size)==NULL) {
        return -2;
    }
    return size;
}

/* Writes the data to the tagbase but only if the corresponding mask bit is set */
int tag_mask_write(handle_t handle, void *data, void *mask, size_t size) {
    u_int8_t *db, *newdata, *newmask;
    size_t n;
    handle /= 8; /* ditch the bottom three bits */
    if((__databasesize * 4) - handle < size) {
        return -1; /* Whoa don't overflow my buffer */
    }
    
    db = (u_int8_t *)__db; /* Cast __db to byte */
    newdata = (u_int8_t *)data;
    newmask = (u_int8_t *)mask;
    
    for(n = 0; n < size; n++) {
        db[handle + n] = (newdata[n] & newmask[n]) | (db[handle + n] & ~newmask[n]);
    }
    return size;
}

/* Event handling code. Events are simply a notification mechanism when
   a tag (or a portion) of a tag changes.  The events are linked lists 
   that have the head pointer in the __taglist[] array.  Within each linked
   list of events is a linked list of pointers to modules that want to recieve
   notification messages for the change.  Each event is basically a database
   handle and a size (in bits) of the data that is in question.  It is assumed
   that the event handle and size are contained within the tag. */

/* This adds a module to the linked list of modules in the
   event whose pointer is passed as *event.  Will return
   the id of the event that has been passed or -1 on error. */
static int event_add_module(_dax_event *event, dax_module *module) {
    struct mod_list *this, *new;
    xlog(10, "Adding module to event");

    if( (this = event->notify) == NULL) {
        event->notify = new;
    } else {
        while(this->next != NULL) {
            if(this->module == module) {
                return event->id;
            }
            this = this->next;
        }
        new = malloc(sizeof(struct mod_list));
        if(new == NULL) return -1; /* The only error that we know is no more memory */
        new->module = module;
        new->next = NULL;
        this->next = new;
    }
    
    return event->id;
}

/* Add an event to the tag array.  It is assumed that the event will be
   contained in a single tag. */
int event_add(handle_t handle, size_t size, dax_module *module) {
    int index;
    _dax_event *this = NULL, *new = NULL;
    
    index = get_by_handle(handle);
    xlog(10, "Found tag at index %d", index);
    /* get_by_handle() will return -1 if the handle is no good */
    if( index < 0 ) {
        return ERR_NOTFOUND;
    }
    xlog(10, "Got a good handle");
    this = __taglist[index].events;
    
    /* If we get this far then the starting handle is contained in a tag.
       Now we need to see if the ending bit is also contained. */
    if( (handle + size) > (__taglist[index].handle + __taglist[index].count * TYPESIZE(__taglist[index].type)) ) {
        /* Return error if the notification definition exceeds the tags boundries */
        xlog(8, "Event tag definition is out of bounds");
        return ERR_2BIG;
    }
    
    /* Check to see if there is already a notification like this contained
       in this tag */
    while(this != NULL) {
        if( this->handle == handle && this->size == size ) {
            xlog(10, "Found an event just like this one");
            return event_add_module(this, module);
        }
        this = this->next;
    }
    
    new = malloc(sizeof(_dax_event));
    if(new == NULL) return ERR_ALLOC;
    
    /* Create and initialize the new event */
    new->id = __next_event_id++;
    new->handle = handle;
    new->notify = NULL;
    new->size = size;
    new->checksum = 0x00;
    new->next = NULL;
    
    this = __taglist[index].events;
    
    if( (this = __taglist[index].events) == NULL) {
        __taglist[index].events = new;
    } else {
        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new;
    }
    
    return event_add_module(new, module);
}

int event_del(int id) {
    return 0;
}

/* Compares the tagbase given by handle and size against the array of
   events and sends the event messages when necessary */
static int event_check(handle_t handle, size_t size) {
    return 0;
}


#endif //--DELTETING STUFF

#ifdef DAX_DIAG
void 
diag_list_tags(void)
{
    int n;
    for(n=0; n<__tagcount; n++) {
        printf("__db[%d] = %s[%d] type = %d\n",n, __db[n].name, __db[n].count, __db[n].type);
    }
}
#endif DAX_DIAG
