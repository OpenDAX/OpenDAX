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
#include <assert.h>

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
static int
_checktype(tag_type type)
{
    int index;
    
    if(type & DAX_BOOL)
        return 0;
    if(type & DAX_BYTE)
        return 0;
    if(type & DAX_SINT)
        return 0;
    if(type & DAX_WORD)
        return 0;
    if(type & DAX_INT)
        return 0;
    if(type & DAX_UINT)
        return 0;
    if(type & DAX_DWORD)
        return 0;
    if(type & DAX_DINT)
        return 0;
    if(type & DAX_UDINT)
        return 0;
    if(type & DAX_TIME)
        return 0;
    if(type & DAX_REAL)
        return 0;
    if(type & DAX_LWORD)
        return 0;
    if(type & DAX_LINT)
        return 0;
    if(type & DAX_ULINT)
        return 0;
    if(type & DAX_LREAL)
        return 0;
    /* NOTE: This will only work as long as we don't allow CDT's to be deleted */
    index = CDT_TO_INDEX(type);
    if( index >= 0 && index < _datatype_index) {
        return 0;
    }
    return ERR_NOTFOUND;
}

/* Determine the size of the tag in bytes.  It'll
 * be big trouble if the index is out of bounds.
 * This will also return 0 when the tag has been deleted
 * which is designated by zero type and zero count */
static inline size_t
_get_tag_size(tag_index idx)
{
    if(_db[idx].type == DAX_BOOL)
        return _db[idx].count / 8 + 1;
    else
        return type_size(_db[idx].type)  * _db[idx].count;
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
 * the given name.  It returns the index into the _index array */
static int
_get_by_name(char *name)
{
    int i, min, max, try;
    
    min = 0;
    max = _tagcount;
    
    while(min < max) {
        try = (min + max) / 2;
        i = strcmp(name, _index[try].name);
        if(i > 0) {
            min = try + 1;
        } else if(i < 0) {
            max = try;
        } else {
            return _index[try].tag_idx;
        }
    }
    return ERR_NOTFOUND;
}

/* This function incrememnts the reference counter for the
 * custom data type.  It assumes that the type is valid, if
 * the type is not valid, bad things will happen */
static inline void
_cdt_inc_refcount(tag_type type) {
    _datatypes[CDT_TO_INDEX(type)].refcount++;
}

/* This funtion decrements the reference counter */
static inline void
_cdt_dec_refcount(tag_type type) {
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
    int i, min, max, try;
    
    /* Let's allocate the memory for the string first in case it fails */
    temp = strdup(name);
    if(temp == NULL)
        return ERR_ALLOC;
    
    if(_tagcount == 0) {
        n = 0;
    } else {
        min = 0;
        max = _tagcount - 1;
        
        while(min < max) {
            try = (min + max) / 2;
            i = strcmp(name, _index[try].name);
            if(i > 0) {
                min = try + 1;
            } else if(i < 0) {
                max = try;
            } else {
                /* It can't really get here because duplicates were checked in add_tag()
                 * before this function was called */
                assert(0);
            }
        }
        n = min;
        memmove(&_index[n + 1], &_index[n], (_dbsize - n - 1) * sizeof(_dax_tag_index));
    }
    /* Assign pointer to database node in the index */
    _index[n].tag_idx = index;
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
    //tag_type type;
    int result;
    
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
    if( (result = tag_add("_status", DAX_DWORD, STATUS_SIZE)) ) {
        xfatal("_status not created properly: Error %d", result);
    }

    /* Allocate the datatype array and set the initial counters */
    _datatypes = xmalloc(sizeof(datatype) * DAX_DATATYPE_SIZE);
    if(!_datatypes) {
        xfatal("Unable to allocate array for datatypes");
    }
    _datatype_index = 0;
    _datatype_size = DAX_DATATYPE_SIZE;

    /* TODO: These can be done by manually entering the description string
     * in the argument */
    //type = cdt_create("System", NULL);
    //index = cdt_create("TimeStruct");

    
}


/* This adds a tag to the database. */
tag_index
tag_add(char *name, tag_type type, unsigned int count)
{
    int n;
    void *newdata;
    unsigned int size;
    int result;
    
    if(count == 0)
        return ERR_ARG;

    printf("tag_add() called with name = %s, type = 0x%X, count = %d\n", name, type, count);
    if(_tagcount >= _dbsize) {
        if(_database_grow()) {
            xerror("Failure to increae database size");
            return ERR_ALLOC;
        } else {
            xlog(LOG_MINOR, "Database increased to %d items", _dbsize);
        }
    }
    result = _checktype(type);
    if( result ) {
        xerror("Unknown datatype %x", type);
        return result; /* is the datatype valid */
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

/* Finds a tag based on it's name.  Basically just a wrapper for _get_by_name().
 * Fills in the structure 'tag' and returns zero on sucess */
int
tag_get_name(char *name, dax_tag *tag)
{
    int i;

    i = _get_by_name(name);
    if(i < 0) {
        return ERR_NOTFOUND;
    } else {
        tag->idx = i;
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
        tag->idx = index;
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
tag_read(tag_index idx, int offset, void *data, size_t size)
{
    /* Bounds check handle */
    if(idx < 0 || idx >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(idx)) {
        return ERR_2BIG;
    }
    /* Copy the data into the right place. */
    memcpy(data, &(_db[idx].data[offset]), size);
    return 0;
}

/* This function writes data to the _db just like the above function reads it */
int
tag_write(tag_index idx, int offset, void *data, size_t size)
{
    /* Bounds check handle */
    if(idx < 0 || idx >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(idx)) {
        return ERR_2BIG;
    }
    /* Copy the data into the right place. */
    memcpy(&(_db[idx].data[offset]), data, size);
    return 0;
}

/* Writes the data to the tagbase but only if the corresponding mask bit is set */
int
tag_mask_write(tag_index idx, int offset, void *data, void *mask, size_t size)
{
    char *db, *newdata, *newmask;
    int n;

    /* Bounds check handle */
    if(idx < 0 || idx >= _tagcount) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > _get_tag_size(idx)) {
        return ERR_2BIG;
    }
    /* Just to make it easier */
    db = &_db[idx].data[offset];
    newdata = (char *)data;
    newmask = (char *)mask;

    for(n = 0; n < size; n++) {
        db[n] = (newdata[n] & newmask[n]) | (db[n] & ~newmask[n]);
    }
    return 0;
}

/* These two static functions destroy the cdt that is
 * passed as *cdt to _cdt_destroy.  _cdt_member_destroy
 * is a static function to free the member list */
static void
_cdt_member_destroy(cdt_member *member) {
    if(member->next != NULL) _cdt_member_destroy(member->next);
    if(member->name != NULL) xfree(member->name);
    xfree(member);
}

static inline void
_cdt_destroy(datatype *cdt) {
    if(cdt->members != NULL) _cdt_member_destroy(cdt->members);
    if(cdt->name != NULL ) xfree(cdt->name);
}

/* Creates a compound datatype using the definition string in *str.
 * Returns the positive index of the newly created datatype if
 * sucessful or 0 on failure. If *error is not NULL any error codes
 * will be placed there otherwise zero will assigned to error. */
tag_type
cdt_create(char *str, int *error) {
    int result;
    char *name, *member, *last;
    datatype cdt;
    datatype *new_datatype;
    
    /* This messes with *str.  It puts a '\0' everywhere there is a ':'
     * and that is okay because the calling function doesn't need it anymore
     * except for printing the name so this works out okay. */
    name = strtok_r(str, ":", &last);
    /* Check that the name is okay */
    if( (result = _validate_name(name)) ) {
        if(error != NULL) *error = result;
        return 0;
    }
    if(cdt_get_type(name)) {
        if(error != NULL) *error = ERR_DUPL;
        return 0;
    }
    
    /* Initialize the new datatype */
    cdt.name = strdup(name);
    if(cdt.name == NULL) {
        if(error != NULL) *error = ERR_ALLOC;
        return 0;
    }
    cdt.members = NULL;
    
    while((member = strtok_r(NULL, ":", &last))) {
        result = cdt_append(&cdt, member);
        if(result) {
            _cdt_destroy(&cdt);
            if(error != NULL) *error = result;
            return 0;
        }
    }
    
    /* Do we have space in the array */
    if(_datatype_index == _datatype_size) {
        /* Allocate more space for the array */
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
    _datatypes[_datatype_index].name = cdt.name;
    _datatypes[_datatype_index].members = cdt.members;
    _datatypes[_datatype_index].refcount = 0;
    _datatypes[_datatype_index].flags = 0;
    _datatype_index++;

    if(error) *error = 0;
    return CDT_TO_TYPE((_datatype_index - 1));
}

/* Recieves a definition string in the form of "Name,Type,Count" and 
 * appends that member to the compound datatype passed as *cdt.  Returns
 * 0 on success and dax error code on failure */
int
cdt_append(datatype *cdt, char *str)
{
    int retval, count;
    cdt_member *this, *new;
    char *name, *typestr, *countstr, *last;
    tag_type type;
    
    //--printf("  NEW MEMBER - %s >> ", str);
    /* Parse the description string */
    name = strtok_r(str, ",", &last);
    if(name == NULL) return ERR_ARG;
    typestr = strtok_r(NULL, ",", &last);
    if(typestr == NULL) return ERR_ARG;
    countstr = strtok_r(NULL, ",", &last);
    if(countstr == NULL) return ERR_ARG;
    count = strtol(countstr, NULL, 0);
    
    /* Check that the name is okay */
    if( (retval = _validate_name(name)) ) {
        return retval;
    }

    /* Check that the type is valid */
    if( (type = cdt_get_type(typestr)) == 0 ) {
        return ERR_ARG;
    }

    /* Check for duplicates */
    this = cdt->members;
    while(this != NULL) {
        if( !strcasecmp(name, this->name) ) {
            return ERR_DUPL;
        }
        this = this->next;
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
    if(cdt->members == NULL) {
        cdt->members = new;
    } else {
        this = cdt->members;

        while(this->next != NULL) {
            this = this->next;
        }
        this->next = new;
    }
    
    return 0;
    
//    int retval;
//    datatype *new_datatype;
//
//    if((retval = _validate_name(name)))
//        return retval;
//
//    /* Check for duplicates */
//    if(cdt_get_type(name) != 0) {
//        xerror("Duplicate name '%s' given to cdt_create()");
//        if(error) *error = ERR_DUPL;
//        return 0;
//    }
//
//    /* Do we have space in the array */
//    if(_datatype_index == _datatype_size) {
//        /* Allocate more space for the array */
//        new_datatype = xrealloc(_datatypes, (_datatype_size + DAX_DATATYPE_SIZE) * sizeof(datatype));
//
//        if(new_datatype != NULL) {
//            _datatypes = new_datatype;
//            _datatype_size += DAX_DATATYPE_SIZE;
//        } else {
//            if(error) *error = ERR_ALLOC;
//            return 0;
//        }
//    }
//
//    /* Add the datatype */
//    _datatypes[_datatype_index].name = strdup(name);
//    if(_datatypes[_datatype_index].name == NULL) {
//        if(error) *error = ERR_ALLOC;
//        return 0;
//    }
//    _datatypes[_datatype_index].members = NULL;
//    _datatypes[_datatype_index].refcount = 0;
//    _datatype_index++;
//
//    if(error) *error = 0;
//    return CDT_TO_TYPE((_datatype_index - 1));
}

/* Returns the type of the datatype with given name
 * If the datatype isn't found it returns 0 */
tag_type
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
cdt_get_name(tag_type type)
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
/* TODO: Now that I have made CDT creation atomic I don't think that it's
 * possible to have recursive type??? Probably should delete all this */
//static int
//_cdt_does_contain(unsigned int needle, unsigned int haystack)
//{
//    cdt_member *this;
//    int result;
//    
//    if(haystack == needle) return -1;
//    
//    this = _datatypes[CDT_TO_INDEX(haystack)].members;
//    while(this != NULL) {
//        if(IS_CUSTOM(this->type)) {
//            if(this->type == needle) {
//                return -1;
//            } else {
//                result = _cdt_does_contain(needle, this->type);
//                if(result) return result;
//            }
//        }
//        this = this->next;
//    }
//    return 0;
//}

/* Adds a member to the datatype referenced by it's array index 'cdt_index'. 
 * Returns 0 on success, nonzero error code on failure. */
int
cdt_add_member(datatype *cdt, char *name, tag_type type, unsigned int count)
{
    int retval;
    cdt_member *this, *new;
    
    /* Check that there aren't any references and that the FINAL flag is not set */
//    if(cdt->refcount > 0 || (cdt->flags & CDT_FLAGS_FINAL) ) {
//        xlog(LOG_MINOR, "CDT in Use");
//        return ERR_INUSE;
//    }
    /* Check that the name is okay */
    if( (retval = _validate_name(name)) ) {
        return retval;
    }
    /* if type is 0 then set the FINAL flag and return success */
//    if( type == 0) {
//        xlog(LOG_MINOR, "Finalizing %s", _datatypes[cdt_index].name);
//        _datatypes[cdt_index].flags |= CDT_FLAGS_FINAL;
//        return 0;
//    }
    /* Check that the type is valid */
    if( _checktype(type) ) {
        return ERR_ARG;
    }

    /* Check for duplicates */
    this = cdt->members;
    while(this != NULL) {
        if( !strcasecmp(name, this->name) ) {
            return ERR_DUPL;
        }
        this = this->next;
    }
    
    /* This checks for recursion in the members.  Bad things
     * will happen if we include a member to ourselves. */
//    if( IS_CUSTOM(type) && _cdt_does_contain(CDT_TO_TYPE(cdt_index), type)) {
//        return ERR_ILLEGAL;
//    }
    
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
    if(cdt->members == NULL) {
        cdt->members = new;
    } else {
        this = cdt->members;

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
int
serialize_datatype(tag_type type, char **str)
{
    int size;
    char test[DAX_TAGNAME_SIZE + 1];
    cdt_member *this;
    int cdt_index;
    
    cdt_index = CDT_TO_INDEX(type);

    if(cdt_index < 0 || cdt_index >= _datatype_index) {
        return ERR_ARG;
    }
    /* Check that the type has been finalized */
    if( !(_datatypes[cdt_index].flags & CDT_FLAGS_FINAL)) {
        *str = NULL;
        return ERR_INUSE;
    }
    /* The first thing we do is figure out how big it
     * will all be. */
    size = strlen(_datatypes[cdt_index].name);
    this = _datatypes[cdt_index].members;

    while(this != NULL) {
        size += strlen(this->name);
        size += strlen(cdt_get_name(this->type));
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%d", this->count);
        size += strlen(test);
        size += 3; /* This is for the ':' and the two commas */
        this = this->next;
    }
    size += 1; /* For Trailing NULL */
    
    *str = xmalloc(size);
    if(*str == NULL)
        return ERR_ALLOC;
    /* Now build the string */
    strncat(*str, _datatypes[cdt_index].name, size -1);
    this = _datatypes[cdt_index].members;

    while(this != NULL) {
        strncat(*str, ":", size - 1);
        strncat(*str, this->name, size - 1);
        strncat(*str, ",", size);
        strncat(*str, cdt_get_name(this->type), size - 1);
        strncat(*str, ",", size - 1);
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%d", this->count);
        strncat(*str, test, size - 1);

        this = this->next;
    }
    return size;
}

/* Figures out how large the datatype is and returns
 * that size in bytes.  This function is recursive */
int
type_size(tag_type type)
{
    int size = 0, result;
    unsigned int pos = 0; /* Bit position within the data area */
    cdt_member *this;

    if( (result = _checktype(type)) ) {
        return result;
    }
    
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
                    result = type_size(this->type);
                    assert(result > 0); /* The types within other types should be okay */
                    pos += (result * this->count) * 8;
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
#endif /* DAX_DIAG */
