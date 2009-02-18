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
 *
 * EDIT: !! Actually none of the above is true at the moment.  The tag database is
 * an array that describes each tag individually.  The tags data is also contained
 * within that structure.
 * 
 */

static _dax_tag_db *_db;
static _dax_tag_index *_index;
static long int _tagcount = 0;
static long int _dbsize = 0;
static datatype *_datatypes;
static unsigned int _datatype_index; /* Next datatype index */
static unsigned int _datatype_size;

//static int _next_event_id = 0;

/* Private function definitions */

/* checks whether type is a valid datatype */
/* TODO: Should this return 0 on success instead of 1? */
static int
_checktype(unsigned int type)
{
    if(type & DAX_BOOL)
        return 1;
    if(type & DAX_BYTE)
        return 1;
    if(type & DAX_SINT)
        return 1;
    if(type & DAX_WORD)
        return 1;
    if(type & DAX_INT)
        return 1;
    if(type & DAX_UINT)
        return 1;
    if(type & DAX_DWORD)
        return 1;
    if(type & DAX_DINT)
        return 1;
    if(type & DAX_UDINT)
        return 1;
    if(type & DAX_TIME)
        return 1;
    if(type & DAX_REAL)
        return 1;
    if(type & DAX_LWORD)
        return 1;
    if(type & DAX_LINT)
        return 1;
    if(type & DAX_ULINT)
        return 1;
    if(type & DAX_LREAL)
        return 1;
    /* NOTE: This will only work as long as we don't allow CDT's to be deleted */
    if(CDT_TO_INDEX(type) >= 0 && CDT_TO_INDEX(type) < _datatype_index) {
        return 1;
    }
    return 0;
}

/* Determine the size of the tag in bytes.  It'll
 be big trouble if the handle is out of bounds.
 This will also return 0 when the tag has been deleted
 which is designated by zero type and zero count */
static inline size_t
_get_tag_size(handle_t h)
{
    if(_db[h].type == DAX_BOOL)
        return _db[h].count / 8 + 1;
    else
        return type_size(_db[h].type)  * _db[h].count;
}

/* Determine whether or not the tag name is okay */
static int
_validate_name(char *name)
{
    int n;
    if(strlen(name) > DAX_TAGNAME_SIZE) {
        return ERR_2BIG;
    }
    /* First character has to be a letter or '_' */
    if( !isalpha(name[0]) && name[0] != '_') {
        return ERR_ARG;
    }
    /* The rest of the name can be letters, numbers or '_' */
    for(n = 1; n < strlen(name) ; n++) {
        if( !isalpha(name[n]) && (name[n] != '_') && !isdigit(name[n]) ) {
            return ERR_ARG;
        }
    }
    return 0;
}

/* This function searches the _index array to find the tag with
 the given name.  It returns the index into the _index array */
static int
_get_by_name(char *name)
{
    /* TODO: Better search algorithm.  This is pretty sad. */
    int i;
    for(i = 0; i < _tagcount; i++) {
        if(!strcmp(name, _index[i].name))
            return _index[i].handle;
    }
    return ERR_NOTFOUND;
}

/* This function incrememnts the reference counter for the
 * custom data type.  It assumes that the type is valid, if
 * the type is not valid, bad things will happen */
static inline void
_cdt_inc_refcount(unsigned int type) {
    _datatypes[CDT_TO_INDEX(type)].refcount++;
}

/* This funtion decrements the reference counter */
static inline void
_cdt_dec_refcount(unsigned int type) {
    if(_datatypes[CDT_TO_INDEX(type)].refcount != 0) {
        _datatypes[CDT_TO_INDEX(type)].refcount--;
    }
}


/* Grow the database when necessary */
static int
_database_grow(void)
{
    _dax_tag_index *new_index;
    _dax_tag_db *new_db;

    new_index = xrealloc(_index, (_dbsize + DAX_DATABASE_INC) * sizeof(_dax_tag_index));
    new_db = xrealloc(_db, (_dbsize + DAX_DATABASE_INC) * sizeof(_dax_tag_db));

    if(new_index != NULL && new_db != NULL) {
        _index = new_index;
        _db = new_db;
        _dbsize += DAX_DATABASE_INC;
        return 0;
    } else {
        /* This is to shrink the one that didn't get allocated
         so that they won't be lop sided */
        if(new_index)
            _index = xrealloc(_index, _dbsize * sizeof(_dax_tag_index));
        if(new_db)
            _db = xrealloc(_db, _dbsize * sizeof(_dax_tag_db));

        return ERR_ALLOC;
    }
}


/* This adds the name of the tag to the index */
static int
_add_index(char *name, int index)
{
    int n;
    char *temp;

    /* Let's allocate the memory for the string first in case it fails */
    temp = strdup(name);
    if(temp == NULL)
        return ERR_ALLOC;
    /* TODO: Linear search for now.  This needs to be a binary search */
    if(_tagcount == 0) {
        n = 0;
    } else {
        for(n = 1; n < _tagcount; n++) {
            if(strcmp(_index[n].name, name) > 0) {
                memmove(&_index[n + 1], &_index[n], (_dbsize - n - 1) * sizeof(_dax_tag_index));
                break;
            }
        }
    }
    /* Assign pointer to database node in the index */
    _index[n].handle = index;
    /* The name pointer in the __index and the __db point to the same string */
    _index[n].name = temp;
    _db[index].name = temp;
    return 0;
}

/* Allocates the symbol table and the database array.  There's no return
 value because failure of this function is fatal */
void
initialize_tagbase(void)
{
    //int result;
    unsigned int type;
    //char *str;
    //int cdt1,cdt2,cdt3;

    _db = xmalloc(sizeof(_dax_tag_db) * DAX_TAGLIST_SIZE);
    if(!_db) {
        xfatal("Unable to allocate the database");
    }
    _dbsize = DAX_TAGLIST_SIZE;
    /* Allocate the primary database */
    _index = (_dax_tag_index *)xmalloc(sizeof(_dax_tag_index)
            * DAX_TAGLIST_SIZE);
    if(!_db) {
        xfatal("Unable to allocate the database");
    }

    xlog(LOG_MINOR, "Database created with size = %d", _dbsize);

    /* Create the _status tag at handle zero */
    if(tag_add("_status", DAX_DWORD, STATUS_SIZE)) {
        xfatal("_status not created properly");
    }

    /* Allocate the datatype array and set the initial counters */
    _datatypes = xmalloc(sizeof(datatype) * DAX_DATATYPE_SIZE);
    if(!_datatypes) {
        xfatal("Unable to allocate array for datatypes");
    }
    _datatype_index = 0;
    _datatype_size = DAX_DATATYPE_SIZE;

    type = cdt_create("System", NULL);
    //index = cdt_create("TimeStruct");

    /*
    result = cdt_add_member(index, "TimeStamp", DAX_TIME, 1);
    result = cdt_add_member(index, "Year", DAX_INT, 1);
    result = cdt_add_member(index, "Month", DAX_INT, 1);
    result = cdt_add_member(index, "Day", DAX_INT, 1);
    result = cdt_add_member(index, "Hour", DAX_INT, 1);
    result = cdt_add_member(index, "Minute", DAX_INT, 1);
    result = cdt_add_member(index, "Second", DAX_INT, 1);
    result = cdt_add_member(index, "uSecond", DAX_DINT, 1);

    result = cdt_add_member(0, "StartTime", cdt_get_type("TimeStruct"), 1);
    
    result = cdt_add_member(index, "StartTime", DAX_TIME, 1);
    if(result) printf("OOPs - Unable to Add StartTime\n");
    result = cdt_add_member(index, "Modules", DAX_DINT, 1);
    if(result) printf("OOPs - Unable to Add Modules\n");
    
    
    result = tag_add("System", cdt_get_type("System"), 1);
    result = cdt_add_member(index, "Failure", DAX_DINT, 1);
    if(result) printf("OK - Unable to Add Failure\n");
    
    serialize_datatype(index, &str);
    printf("%s\n", str);
    free(str);
    */
}


/* This adds a tag to the database. */
handle_t
tag_add(char *name, unsigned int type, unsigned int count)
{
    int n;
    void *newdata;
    unsigned int size;
    if(count == 0)
        return ERR_ARG;

    //--printf("tag_add() called with name = %s, type = %d, count = %d\n", name, type, count);
    if(_tagcount >= _dbsize) {
        if(_database_grow()) {
            xerror("Failure to increae database size");
            return ERR_ALLOC;
        } else {
            xlog(LOG_MINOR, "Database increased to %d items", _dbsize);
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

    /* Figure the size in bytes */
    if(type == DAX_BOOL) {
        size = count / 8 + 1;
    } else {
        size = type_size(type) * count;
    }
    

    /* Check for an existing tagname in the database */
    if( (n = _get_by_name(name)) >= 0) {
        /* If the tag is identical or bigger then just return the handle */
        if(_db[n].type == type && _db[n].count >= count) {
            return n;
        } else if(_db[n].type == type && _db[n].count < count) {
            /* If the new count is greater than the existing count then lets
             try to increase the size of the tags data */
            newdata = realloc(_db[n].data, size);
            if(newdata) {
                _db[n].data = newdata;
                /* TODO: Zero the new part of the allocation */
                _db[n].count = count;
                return n;
            } else {
                return ERR_ALLOC;
            }
        } else {
            xerror("Duplicate tag name %s", name);
            return ERR_TAG_DUPL;
        }
    } else {
        n = _tagcount;
    }
    /* Assign everything to the new tag, copy the string and git */
    _db[n].count = count;
    _db[n].type = type;

    /* Allocate the data area */
    if((_db[n].data = xmalloc(size)) == NULL)
        return ERR_ALLOC;
    else
        bzero(_db[n].data, size);

    _db[n].events = NULL;

    if(_add_index(name, n)) {
        /* free up our previous allocation if we can't put this in the __index */
        free(_db[n].data);
        return ERR_ALLOC;
    }
    /* Only if everything works will we increment the count */
    if(IS_CUSTOM(type)) {
        _cdt_inc_refcount(type);
    }
    _tagcount++;
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
int
tag_get_name(char *name, dax_tag *tag)
{
    int i; //, handle;

    i = _get_by_name(name);
    if(i < 0) {
        return ERR_NOTFOUND;
    } else {
        //handle = __index[i].handle;
        //handle = i; //__index[i].handle;
        tag->handle = i;
        tag->type = _db[i].type;
        tag->count = _db[i].count;
        strcpy(tag->name, _db[i].name);
        return 0;
    }
}

/* Finds a tag based on it's index in the array.  Returns a pointer to 
 the tag give by index, NULL if index is out of range.  Index should
 not be assumed to remain constant throughout the programs lifetime. */
int
tag_get_index(int index, dax_tag *tag)
{
    if(index < 0 || index >= _tagcount) {
        return ERR_ARG;
    } else {
        tag->handle = index;
        tag->type = _db[index].type;
        tag->count = _db[index].count;
        strcpy(tag->name, _db[index].name);
        return 0;
    }
}

/* These are the low level tag reading / writing interface to the
 * database.
 * 
 * This function reads the data from the tag given by handle at
 * the byte offset.  It will write size bytes into data and will
 * return 0 on success and some negative number on failure.
 */
int
tag_read(handle_t handle, int offset, void *data, size_t size)
{
    /* Bounds check handle */
    if(handle < 0 || handle >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(handle)) {
        return ERR_2BIG;
    }
    /* Copy the data into the right place. */
    memcpy(data, &(_db[handle].data[offset]), size);
    return 0;
}

/* This function writes data to the _db just like the above function reads it */
int
tag_write(handle_t handle, int offset, void *data, size_t size)
{
    /* Bounds check handle */
    if(handle < 0 || handle >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(handle)) {
        return ERR_2BIG;
    }
    /* Copy the data into the right place. */
    memcpy(&(_db[handle].data[offset]), data, size);
    return 0;
}

/* Writes the data to the tagbase but only if the corresponding mask bit is set */
int
tag_mask_write(handle_t handle, int offset, void *data, void *mask, size_t size)
{
    char *db, *newdata, *newmask;
    int n;

    /* Bounds check handle */
    if(handle < 0 || handle >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(handle)) {
        return ERR_2BIG;
    }
    /* Just to make it easier */
    db = &_db[handle].data[offset];
    newdata = (char *)data;
    newmask = (char *)mask;

    for(n = 0; n < size; n++) {
        db[n] = (newdata[n] & newmask[n]) | (db[n] & ~newmask[n]);
    }
    return 0;
}

/* Creates a new CDT with 'name'  Returns zero on error.
 * If there is an error an error code is left in 'error' if
 * it is non NULL. 0 will be left in 'error' if succesfull */
unsigned int
cdt_create(char *name, int *error)
{
    int retval;

    if( (retval = _validate_name(name)))
        return retval;

    /* Check for duplicates */
    if(cdt_get_type(name) != 0) {
        xerror("Duplicate name '%s' given to cdt_create()");
        if(error) *error = ERR_DUPL;
        return 0;
    }

    /* Do we have space in the array */
    if(_datatype_index == _datatype_size) {
        /* Allocate more space for the array */
        datatype *new_datatype;

        new_datatype = xrealloc(_datatypes, (_datatype_size + DAX_DATATYPE_SIZE) * sizeof(datatype));

        if(new_datatype != NULL) {
            _datatypes = new_datatype;
            _datatype_size += DAX_DATATYPE_SIZE;
        } else {
            if(error) *error = ERR_ALLOC;
            return 0;
        }
    }

    /* Add the datatype */
    _datatypes[_datatype_index].name = strdup(name);
    if(_datatypes[_datatype_index].name == NULL) {
        if(error) *error = ERR_ALLOC;
        return 0;
    }
    _datatypes[_datatype_index].members = NULL;
    _datatypes[_datatype_index].refcount = 0;
    _datatype_index++;

    if(error) *error = 0;
    return CDT_TO_TYPE((_datatype_index - 1));
}

/* Returns the type of the datatype with given name
 * If the datatype isn't found it returns 0 */
unsigned int
cdt_get_type(char *name)
{
    int n;

    if(!strcasecmp(name, "BOOL"))
        return DAX_BOOL;
    if(!strcasecmp(name, "BYTE"))
        return DAX_BYTE;
    if(!strcasecmp(name, "SINT"))
        return DAX_SINT;
    if(!strcasecmp(name, "WORD"))
        return DAX_WORD;
    if(!strcasecmp(name, "INT"))
        return DAX_INT;
    if(!strcasecmp(name, "UINT"))
        return DAX_UINT;
    if(!strcasecmp(name, "DWORD"))
        return DAX_DWORD;
    if(!strcasecmp(name, "DINT"))
        return DAX_DINT;
    if(!strcasecmp(name, "UDINT"))
        return DAX_UDINT;
    if(!strcasecmp(name, "TIME"))
        return DAX_TIME;
    if(!strcasecmp(name, "REAL"))
        return DAX_REAL;
    if(!strcasecmp(name, "LWORD"))
        return DAX_LWORD;
    if(!strcasecmp(name, "LINT"))
        return DAX_LINT;
    if(!strcasecmp(name, "ULINT"))
        return DAX_ULINT;
    if(!strcasecmp(name, "LREAL"))
        return DAX_LREAL;

    for(n = 0; n < _datatype_index; n++) {
        if(!strcasecmp(name, _datatypes[n].name)) {
            return CDT_TO_TYPE(n);
        }
    }
    return 0;
}

/* Returns a pointer to the name of the datatype given
 * by 'type'.  Returns NULL on failure */
char *
cdt_get_name(unsigned int type)
{
    int index;

    if(IS_CUSTOM(type)) {
        index = CDT_TO_INDEX(type);
        if(index >= 0 && index < _datatype_index) {
            return _datatypes[index].name;
        } else {
            return NULL;
        }
    } else {
        switch (type) {
            case DAX_BOOL:
                return "BOOL";
            case DAX_BYTE:
                return "BYTE";
            case DAX_SINT:
                return "SINT";
            case DAX_WORD:
                return "WORD";
            case DAX_INT:
                return "INT";
            case DAX_UINT:
                return "UINT";
            case DAX_DWORD:
                return "DWORD";
            case DAX_DINT:
                return "DINT";
            case DAX_UDINT:
                return "UDINT";
            case DAX_TIME:
                return "TIME";
            case DAX_REAL:
                return "REAL";
            case DAX_LWORD:
                return "LWORD";
            case DAX_LINT:
                return "LINT";
            case DAX_ULINT:
                return "ULINT";
            case DAX_LREAL:
                return "LREAL";
            default:
                return NULL;
        }
    }
}

/* This is a recursive function to check that needle is not
 * contained anywhere in any of the members of haystack.
 * Both arguments are assumed to be custom datatype id's
 * returns 0 if nothing is found and -1 if it is found.  This
 * function assumes that only custom datatypes that really exist
 * will be passed.  If not then bad things will happen */
static int
_cdt_does_contain(unsigned int needle, unsigned int haystack) {
    cdt_member *this;
    int result;
    
    if(haystack == needle) return -1;
    
    this = _datatypes[CDT_TO_INDEX(haystack)].members;
    while(this != NULL) {
        if(IS_CUSTOM(this->type)) {
            if(this->type == needle) {
                return -1;
            } else {
                result = _cdt_does_contain(needle, this->type);
                if(result) return result;
            }
        }
        this = this->next;
    }
    return 0;
}

int
cdt_add_member(int cdt_index, char *name, unsigned int type, unsigned int count)
{
    int retval;
    cdt_member *this, *new;

    /* Check bounds of cdt_index */
    if(cdt_index < 0 || cdt_index >= _datatype_index) {
        return ERR_ARG;
    }
    
    if(_datatypes[cdt_index].refcount > 0) 
        return ERR_INUSE;
        /* Check that the name is okay */
    if( (retval = _validate_name(name))) {
        return retval;
    }

    /* Check that the type is valid */
    if( !_checktype(type)) {
        return ERR_ARG;
    }

    /* Check for duplicates */
    this = _datatypes[cdt_index].members;
    while(this != NULL) {
        if( !strcasecmp(name, this->name) ) {
            return ERR_DUPL;
        }
        this = this->next;
    }
    
    /* This checks for recursion in the members.  Bad things
     * will happen if we include a member to ourselves. */
    if( IS_CUSTOM(type) && _cdt_does_contain(CDT_TO_TYPE(cdt_index), type)) {
        return ERR_ILLEGAL;
    }
    
    /* Allocate the new member */
    new = xmalloc(sizeof(cdt_member));
    if(new == NULL) {
        return ERR_ALLOC;
    }
    /* Assign everything to the new datatype member */
    new->name = strdup(name);
    if(new->name == NULL) {
        free(new);
        return ERR_ALLOC;
    }
    new->type = type;
    new->count = count;
    new->next = NULL;

    /* Find the end of the linked list and put it there */
    if(_datatypes[cdt_index].members == NULL) {
        _datatypes[cdt_index].members = new;
    } else {
        this = _datatypes[cdt_index].members;

        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new;
    }
    return 0;
}

/* This function builds a string that is a serialization of
 * the datatype and all of it's members.  The pointer passed
 * as str will be set to the string.  This pointer will have
 * to be freed by the calling program.  Returns the size
 * of the string. <0 on error */
/* CRITICAL: This is a critical function.  It must run in
 * the same thread as any function that manipulates the cdt 
 * array, or it will have to be protected by a Mutex */
int
serialize_datatype(int cdt_index, char **str)
{
    int size;
    char test[DAX_TAGNAME_SIZE + 1];
    cdt_member *this;

    if(cdt_index < 0 || cdt_index >= _datatype_index) {
        return ERR_ARG;
    }

    /* The first thing we do is figure out how big it
     * will all be. */
    size = strlen(_datatypes[cdt_index].name);
    this = _datatypes[cdt_index].members;

    while(this != NULL) {
        size += strlen(this->name);
        size += strlen(cdt_get_name(this->type));
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%ld", this->count);
        size += strlen(test);
        size += 3; /* This is for the ':' and the two commas */
        this = this->next;
    }
    size += 1; /* For Trailing NULL */

    *str = xmalloc(size);
    if(*str == NULL)
        return ERR_ALLOC;
    *str[0] = '\0'; /* Make the string zero length */

    /* Now build the string */
    strlcat(*str, _datatypes[cdt_index].name, size);
    this = _datatypes[cdt_index].members;

    while(this != NULL) {
        strlcat(*str, ":", size);
        strlcat(*str, this->name, size);
        strlcat(*str, ",", size);
        strlcat(*str, cdt_get_name(this->type), size);
        strlcat(*str, ",", size);
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%ld", this->count);
        strlcat(*str, test, size);

        this = this->next;
    }
    return 0;
}

/* Figures out how large the datatype is and returns
 * that size in bytes.  This function is recursive */
int
type_size(unsigned int type)
{
    int size = 0;
    unsigned int pos = 0; /* Bit position within the data area */
    cdt_member *this;

    if( ! _checktype(type) )
        return ERR_ARG;
    
    if(IS_CUSTOM(type)) {
        this = _datatypes[CDT_TO_INDEX(type)].members;
        while (this != NULL) {
            if(this->type == DAX_BOOL) {
                pos += this->count; /* BOOLs are easy just add the number of bits */
            } else {
                /* Since it's not a bool we need to align to the next byte. 
                 * To align it we set all the lower three bits to 1 and then
                 * increment. */
                if(pos % 8 != 0) { /* Do nothing if already aligned */
                    pos |= 0x07;
                    pos++;
                }
                if(IS_CUSTOM(this->type)) {
                    pos += (type_size(this->type) * this->count) * 8;
                } else {
                    /* This gets the size in bits */
                    pos += TYPESIZE(this->type) * this->count;
                }
            }
            this = this->next;
        }
        if(pos) {
            size = (pos - 1)/8 + 1;
        } else {
            size = 0;
        }
    } else { /* Not IS_CUSTOM() */
        size = TYPESIZE(type) / 8; /* Size in bytes */
    }
    return size;
}


#ifdef DAX_DIAG
void
diag_list_tags(void)
{
    int n;
    for (n=0; n<_tagcount; n++) {
        printf("__db[%d] = %s[%d] type = %d\n", n, _db[n].name, _db[n].count, _db[n].type);
    }
}
#endif DAX_DIAG
