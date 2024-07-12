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

#include <ctype.h>
#include <assert.h>
#include <common.h>
#include "tagbase.h"
#include "retain.h"
#include "func.h"

/* Notes:
 * The tags are stored in the server in two different arrays.  Both
 * of these arrays are allocated at runtime and the size is increased
 * as needed.
 *
 * The first array is the actual tag array. It contains a pointer to
 * the name of the tag, the type of tag and the number of items.  It
 * also contains any status flags as well as the data area itself. The
 * tags are stored in this array in the order that they were created.
 * The index of the tag in this array is used as the identifier for
 * that tag for the duration of the program.
 *
 * The second array is the index.  Each item in the index contains a pointer
 * to the name of the tag and the index where the tag data can be found in the
 * first array.  This array is kept sorted in alphabetical order for quicker
 * searching by name.  The name pointer in both arrays point to the same
 * address so the string is not duplicated.
 */

_dax_tag_db *_db;
static _dax_tag_index *_index;
static tag_index _indexcount = 0;     /* Number of tags in the index */
static tag_index _tagnextindex = 0;   /* The next index in the database */
static tag_index _tagcount = 0;
static tag_index _ovrdinstalled = 0;  /* Installled overrides */
static tag_index _ovrdset = 0;        /* Overrides that are set */
static tag_index _dbsize = 0;         /* Size of the database and the index */
static datatype *_datatypes;
static unsigned int _datatype_index;  /* Next datatype index */
static unsigned int _datatype_size;


/* Private function definitions */

/* These functions are convienience for setting status tags */
void
set_dbsize(tag_index x) {
    assert(tag_write(-1, INDEX_DBSIZE, 0, &x, sizeof(tag_index)) == 0);
}

/* checks whether type is a valid datatype */
static int
_checktype(tag_type type)
{
    int index;
    /* Delete the DAX_QUEUE bit and check the type otherwise */
    type &= ~DAX_QUEUE;

    if(type == DAX_BOOL)
        return 0;
    if(type == DAX_BYTE)
        return 0;
    if(type == DAX_SINT)
        return 0;
    if(type == DAX_CHAR)
        return 0;
    if(type == DAX_WORD)
        return 0;
    if(type == DAX_INT)
        return 0;
    if(type == DAX_UINT)
        return 0;
    if(type == DAX_DWORD)
        return 0;
    if(type == DAX_DINT)
        return 0;
    if(type == DAX_UDINT)
        return 0;
    if(type == DAX_TIME)
        return 0;
    if(type == DAX_REAL)
        return 0;
    if(type == DAX_LWORD)
        return 0;
    if(type == DAX_LINT)
        return 0;
    if(type == DAX_ULINT)
        return 0;
    if(type == DAX_LREAL)
        return 0;
    /* NOTE: This will only work as long as we don't allow CDT's to be deleted */
    if(IS_CUSTOM(type)) {
        index = CDT_TO_INDEX(type);
        if( index >= 0 && index < _datatype_index) {
            return 0;
        }
    }
    return ERR_NOTFOUND;
}

/* Determine the size of the tag in bytes.  It'll
 * be big trouble if the index is out of bounds.
 * This will also return 0 when the tag has been deleted
 * which is designated by zero type and zero count */
int
tag_get_size(tag_index idx)
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
    max = _indexcount - 1;
    while(min <= max) {
        try = min + ((max - min) / 2);

        i = strcmp(name, _index[try].name);
        if(i > 0) {
            min = try + 1;
        } else if(i < 0) {
            max = try - 1;
        } else {
            return _index[try].tag_idx;
        }
    }
    return ERR_NOTFOUND;
}

/* This function incrememnts the reference counter for the
 * compound data type.  It assumes that the type is valid, if
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

    new_index = xrealloc(_index, (_dbsize *2) * sizeof(_dax_tag_index));
    new_db = xrealloc(_db, (_dbsize *2) * sizeof(_dax_tag_db));

    if(new_index != NULL && new_db != NULL) {
        _index = new_index;
        _db = new_db;
        _dbsize *= 2;
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
_add_index(char *name, tag_index index)
{
    int n;
    char *temp;
    int i, min, max, try;
    /* Let's allocate the memory for the string first in case it fails */
    temp = strdup(name);
    if(temp == NULL)
        return ERR_ALLOC;

    if(_indexcount == 0) {
        n = 0;
    } else {
        min = 0;
        max = _indexcount - 1;

        while(min <= max) {
            try = min + ((max - min) / 2);
            i = strcmp(name, _index[try].name);
            if(i > 0) {
                min = try + 1;
            } else if(i < 0) {
                max = try - 1;
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
    _indexcount++;
    return 0;
}

/* Delete the entry from the index for the given tag.
 * The tag name must exist or this is an infinite loop */
int
_del_index(char *name) {
    int i, min, max, try, n=-1;
    min = 0;
    max = _indexcount - 1;

    while(min <= max) {
        try = min + ((max - min) / 2);
        i = strcmp(name, _index[try].name);
        if(i > 0) {
            min = try + 1;
        } else if(i < 0) {
            max = try - 1;
        } else {
            n = try;
            break;
        }
    }
    memmove(&_index[n], &_index[n+1], (_dbsize - n - 1) * sizeof(_dax_tag_index));
    _indexcount--;
    return 0;
}

static int
_queue_add(int idx, tag_type type, unsigned int count) {
    virt_functions vf;
    tag_queue *q;
    int size;

    q = (tag_queue *)malloc(sizeof(tag_queue));
    if(q == NULL) {
        return ERR_ALLOC;
    }
    size = type_size(type) * count;
    q->size = size;
    q->count = count;
    q->type = type;
    q->qcount = 0;
    q->qread = 0;
    q->queue = malloc(START_QUEUE_SIZE * sizeof(void *));
    if(q->queue == NULL) {
        free(q);
        return ERR_ALLOC;
    }
    /* Set the pointers to zero so we know that they are not allocated */
    bzero(q->queue, START_QUEUE_SIZE * sizeof(void *));
    q->qsize = START_QUEUE_SIZE;
    vf.rf = read_queue;
    vf.wf = write_queue;
    vf.userdata = (uint8_t *)q;
    _db[idx].data = malloc(sizeof(virt_functions));
    if(_db[idx].data == NULL) {
        free(q);
        free(q->queue);
        return ERR_ALLOC;
    }
    memcpy(_db[idx].data, &vf, sizeof(virt_functions));
    _db[idx].attr |= TAG_ATTR_VIRTUAL;
    dax_log(DAX_LOG_MINOR, "Add Queue for tag at index %d", idx);
    return 0;
}

/* This function adds a virtual tag to the system.  When this tag is read, the data will
 * come from the function given by rf. And when written the data will be passed to wf.  If
 * rf == NULL then the tag will be write only and reads will return an error and likewise if
 * wf == NULL then the tag will be read only and writes will return an error.
 */
tag_index
virtual_tag_add(char *name, tag_type type, unsigned int count, vfunction *rf, vfunction *wf)
{
    tag_index idx;
    virt_functions vf;

    idx = tag_add(-1, name, type, count, 0);
    /* We just allocated this data but that was just for convenience */
    free(_db[idx].data);
    vf.rf = rf;
    vf.wf = wf;
    _db[idx].data = xmalloc(sizeof(virt_functions));
    if(_db[idx].data == NULL) return ERR_ALLOC;
    memcpy(_db[idx].data, &vf, sizeof(virt_functions));
    _db[idx].attr |= TAG_ATTR_VIRTUAL;
    if(wf == NULL) _db[idx].attr |= TAG_ATTR_READONLY;
    return 0;
}


/* Allocates the symbol table and the database array.  There's no return
 value because failure of this function is fatal */
void
initialize_tagbase(void)
{
    tag_type type, tag_type;
    uint64_t starttime;

    _db = xmalloc(sizeof(_dax_tag_db) * DAX_TAGLIST_SIZE);
    if(!_db) {
        dax_log(DAX_LOG_FATAL, "Unable to allocate the database");
        kill(getpid(), SIGQUIT);
    }
    _dbsize = DAX_TAGLIST_SIZE;
    /* Allocate the primary database */
    _index = (_dax_tag_index *)xmalloc(sizeof(_dax_tag_index)
            * DAX_TAGLIST_SIZE);
    if(!_db) {
        dax_log(DAX_LOG_FATAL, "Unable to allocate the database");
        kill(getpid(), SIGQUIT);
    }

    dax_log(DAX_LOG_MINOR, "Database created with size = %d", _dbsize);
    /* Creates the system tags */

    /* Allocate the datatype array and set the initial counters */
    _datatypes = xmalloc(sizeof(datatype) * DAX_DATATYPE_SIZE);
    if(!_datatypes) {
        dax_log(DAX_LOG_FATAL, "Unable to allocate array for datatypes");
        kill(getpid(), SIGQUIT);
    }
    _datatype_index = 0;
    _datatype_size = DAX_DATATYPE_SIZE;

    /*  Create the default datatypes */
    char *_mod_cdt = "_module:starttime,TIME,1:id,DINT,1:running,BOOL,1:faulted,BOOL,1:status,CHAR,64:stop,BOOL,1:run,BOOL,1:reload,BOOL,1:kill,BOOL,1";
    type = cdt_create(_mod_cdt, NULL);
    if(type == 0) {
        dax_log(DAX_LOG_FATAL, "Unable to create default datatypes");
        kill(getpid(), SIGQUIT);
    }
    special_set_module_type(type);

    char tag_type_cdt[256];
    snprintf(tag_type_cdt, 256, "_tag_desc:index,DINT,1:type,UDINT,1:count,UDINT,1:attributes,UINT,1:name,CHAR,%d", DAX_TAGNAME_SIZE + 1);
    tag_type = cdt_create(tag_type_cdt, NULL);
    if(tag_type == 0) {
        dax_log(DAX_LOG_FATAL, "Unable to create default datatypes");
        kill(getpid(), SIGQUIT);
    }

    /* TODO: instead of using all these constants we should probably just add
     * the tags and store the indexes for the ones that we need. */
    assert(tag_add(-1, "_tagcount", DAX_DINT, 1, 0) == INDEX_TAGCOUNT);
    _db[INDEX_TAGCOUNT].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_lastindex", DAX_DINT, 1, 0) == INDEX_LASTINDEX);
    _db[INDEX_LASTINDEX].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_tag_added", tag_type, 1, 0) == INDEX_ADDED_TAG);
    _db[INDEX_ADDED_TAG].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_tag_deleted", tag_type, 1, 0) == INDEX_DELETED_TAG);
    _db[INDEX_DELETED_TAG].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_dbsize", DAX_DINT, 1, 0) == INDEX_DBSIZE);
    _db[INDEX_DBSIZE].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_starttime", DAX_TIME, 1, 0) == INDEX_STARTED);
    _db[INDEX_STARTED].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_lastmodule", DAX_CHAR, DAX_TAGNAME_SIZE +1, 0) == INDEX_LASTMODULE);
    _db[INDEX_LASTMODULE].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_overrides_installed", DAX_DINT, 1, 0) == INDEX_OVRD_INSTALLED);
    _db[INDEX_OVRD_INSTALLED].attr = TAG_ATTR_READONLY;
    assert(tag_add(-1, "_overrides_set", DAX_DINT, 1, 0) == INDEX_OVRD_SET);
    _db[INDEX_OVRD_SET].attr = TAG_ATTR_READONLY;
    virtual_tag_add("_time", DAX_TIME, 1, server_time, NULL);
    virtual_tag_add("_my_tagname", DAX_CHAR, DAX_TAGNAME_SIZE +1, get_module_tag_name, NULL);
    starttime = xtime();
    tag_write(-1, INDEX_STARTED,0,&starttime,sizeof(uint64_t));
    set_dbsize(_dbsize);
}

static void
_set_attribute(tag_index idx, uint32_t attr) {
    /* We only let the Tag Retention attribute to be set at this point */
    if(attr & TAG_ATTR_RETAIN) {
        /* TODO: add tag retention */;
        ret_add_tag(idx);
        _db[idx].attr |= TAG_ATTR_RETAIN;
    } else {
        if(attr & TAG_ATTR_OWNED && attr & TAG_ATTR_READONLY) {
            _db[idx].attr |= (TAG_ATTR_OWNED | TAG_ATTR_READONLY);
        } else if(attr & TAG_ATTR_OWNED) {
            _db[idx].attr |= TAG_ATTR_OWNED;
        }
    }
}

/* This adds a tag to the database. It takes a string for the name
 * of the tag, the type (see opendax.h for types) and a count.  If
 * count is greater than 1 an array is created.  It returns the index
 * of the newly created tag or an error. */
tag_index
tag_add(int fd, char *name, tag_type type, uint32_t count, uint32_t attr)
{
    int n;
    void *newdata;
    unsigned int size;
    int result;
    uint8_t tag_desc[47];

    if(count == 0) {
        dax_log(DAX_LOG_ERROR, "tag_add() called with count = 0");
        return ERR_ARG;
    }

    if(_tagnextindex >= _dbsize) {
        if(_database_grow()) {
            dax_log(DAX_LOG_ERROR, "Failure to increase database size");
            return ERR_ALLOC;
        } else {
            dax_log(DAX_LOG_MINOR, "Database increased to %d items", _dbsize);
        }
    }
    result = _checktype(type);
    if( result ) {
        dax_log(DAX_LOG_MSGERR, "tag_add() passed an unknown datatype %x", type);
        return ERR_BADTYPE; /* is the datatype valid */
    }
    if(_validate_name(name)) {
        dax_log(DAX_LOG_MSGERR, "%s is not a valid tag name", name);
        return ERR_TAG_BAD;
    }

    /* Figure the size in bytes */
    if(type == DAX_BOOL) {
        size = count / 8 + 1;
    } else {
        size = type_size(type) * count;
    }

    /* Check for an existing tagname in the database */
    if( (n = _get_by_name(name)) >= 0) { /* The tag already exists */
        if((_db[n].attr & TAG_ATTR_OWNED) && _db[n].fd != fd) {
            return ERR_TAG_DUPL;
        }
        /* If the tag is identical or bigger then just return the handle */
        if(_db[n].type == type && _db[n].count >= count) {
            _set_attribute(n, attr);
            return n;
        } else if(_db[n].type == type && _db[n].count < count) {
            /* If the new count is greater than the existing count then lets
             try to increase the size of the tags data */
            newdata = xrealloc(_db[n].data, size);
            if(newdata) {
                _db[n].data = newdata;
                _db[n].count = count;
                _set_attribute(n, attr);
                /* Since it changed we update this tag so the write event will trigger */
                tag_write(-1, INDEX_ADDED_TAG, 0, &n, sizeof(tag_index));
                return n;
            } else {
                dax_log(DAX_LOG_ERROR, "Unable to allocate memory to grow the size of tag %s", name);
                return ERR_ALLOC;
            }
        } else {
            dax_log(DAX_LOG_MSGERR, "Duplicate tag name %s", name);
            return ERR_TAG_DUPL;
        }
    } else {
        n = _tagnextindex;
    }

    /* Assign everything to the new tag, copy the string and git */
    _db[n].count = count;
    _db[n].type = type;

    /* If the type is a queue the data area will be allocated in the
     * queue_add() function instead of here */
    if(IS_QUEUE(type)) {
        _queue_add(n, type, count);
    } else {
        /* Allocate the data area */
        if((_db[n].data = xmalloc(size)) == NULL){
            dax_log(DAX_LOG_ERROR, "Unable to allocate memory for tag %s", name);
            return ERR_ALLOC;
        } else {
            bzero(_db[n].data, size);
        }
    }
    _db[n].nextevent = 1;
    _db[n].nextmap = 1;
    _db[n].fd = fd;
    _db[n].events = NULL;
    _db[n].omask = NULL;
    _db[n].odata = NULL;

    if(_add_index(name, n)) {
        /* free up our previous allocation if we can't put this in the __index */
        free(_db[n].data);
        dax_log(DAX_LOG_ERROR, "Unable to allocate data for the tag database index");
        return ERR_ALLOC;
    }
    /* Only if everything works will we increment the count */
    if(IS_CUSTOM(type)) {
        _cdt_inc_refcount(type);
    }
    if(_db[INDEX_LASTINDEX].data != NULL) {
        tag_write(-1, INDEX_LASTINDEX, 0, &_tagnextindex, sizeof(tag_index));
    }
    _tagnextindex++;
    _tagcount++;
    if(_db[INDEX_TAGCOUNT].data != NULL) {
        tag_write(-1, INDEX_TAGCOUNT, 0, &_tagcount, sizeof(tag_index));
    }
    _set_attribute(n, attr);

    /* Update the '_tag_added' system tag */
    memset(&tag_desc, 0, sizeof(dax_tag));
    *((tag_index *)tag_desc) = n;
    *((tag_type *)&tag_desc[4]) = type;
    *((dax_udint *)&tag_desc[8]) = count;
    *((uint16_t *)&tag_desc[12]) = attr;
    memcpy(&tag_desc[14], name, DAX_TAGNAME_SIZE + 1);
    tag_write(-1, INDEX_ADDED_TAG, 0, tag_desc, 47);

    dax_log(DAX_LOG_DEBUG, "Tag added with name = %s, type = 0x%X, count = %d", name, type, count);

    return n;
}

int
tag_set_attribute(tag_index index, uint32_t attr) {
    _db[index].attr |= attr;
    return 0;
}

int
tag_clr_attribute(tag_index index, uint32_t attr) {
    _db[index].attr &= ~attr;
    return 0;
}


/* Deletes the tag given my index.  The tags position in the _db array is not
 * moved.  The name and data fields are freed and set to NULL.  The events
 * and the mappings are also freed and removed.  The item in the index is also
 * removed.  After a tag is deleted the index will always be smaller than the
 * database.  We cannot remove the items from the database and reuse them
 * because some modules may have stored an index to that tag and may try to
 * access it in the future.  If the tag is now different then bad things could
 * happen.
 */
int
tag_del(tag_index idx)
{
    uint8_t tag_desc[47];

    if(idx >= _tagnextindex || idx < 0) {
        dax_log(DAX_LOG_ERROR, "tag_del() pass index out of range %d", idx);
        return ERR_ARG;
    }
    if(_db[idx].data == NULL) { /* We've already deleted this one */
        dax_log(DAX_LOG_ERROR, "tag_del() trying to delete already deleted tag %d", idx);
        return ERR_DELETED;
    }
    event_del_check(idx); /* Check to see if we have a deleted event for this tag */
    if(IS_CUSTOM(_db[idx].type)) {
        _cdt_dec_refcount(_db[idx].type);
    }
    if(_db[idx].attr & TAG_ATTR_RETAIN) {
        ret_del_tag(idx);
    }
    events_del_all(_db[idx].events);
    map_del_all(_db[idx].mappings);
    _del_index(_db[idx].name);
    dax_log(DAX_LOG_DEBUG, "Tag deleted with name = %s", _db[idx].name);

    /* Update the '_tag_deleted' system tag */
    memset(&tag_desc, 0, sizeof(dax_tag));
    *((tag_index *)tag_desc) = idx;
    *((tag_type *)&tag_desc[4]) = _db[idx].type;
    *((dax_udint *)&tag_desc[8]) = _db[idx].count;
    *((uint16_t *)&tag_desc[12]) = _db[idx].attr;
    memcpy(&tag_desc[14], _db[idx].name, DAX_TAGNAME_SIZE + 1);
    tag_write(-1, INDEX_DELETED_TAG, 0, tag_desc, 47);

    xfree(_db[idx].name);
    xfree(_db[idx].data);
    _db[idx].name = NULL;
    _db[idx].data = NULL;
    _db[idx].attr = 0;
    _tagcount--;
    if(_db[INDEX_TAGCOUNT].data != NULL) {
        tag_write(-1, INDEX_TAGCOUNT, 0, &_tagcount, sizeof(tag_index));
    }

    //tag_write(-1, INDEX_DELETED_TAG, 0, &idx, sizeof(tag_index));

    return 0;
}

/* Finds a tag based on it's name.  Basically just a wrapper for _get_by_name().
 * Fills in the structure 'tag' and returns zero on success */
int
tag_get_name(char *name, dax_tag *tag)
{
    int i;

    i = _get_by_name(name);
    if(i < 0) {
        return ERR_NOTFOUND;
    }
    if(_db[i].data == NULL) {
        return ERR_DELETED;
    } else {
        if(tag != NULL) { /* Sometimes we just want to know if the tag exists */
            tag->idx = i;
            tag->type = _db[i].type;
            tag->count = _db[i].count;
            tag->attr = _db[i].attr;
            strcpy(tag->name, _db[i].name);
        }
        return 0;
    }
}

/* Finds a tag based on it's index in the array.  Fills in the structure
 * that is passed in as *tag. */
int
tag_get_index(int index, dax_tag *tag)
{
    if(index < 0 || index >= _tagnextindex) {
        dax_log(DAX_LOG_ERROR, "tag_get_index() called with an index that is out of range");
        return ERR_ARG;
    }
    if(_db[index].data == NULL) {
        return ERR_DELETED;
    } else {
        tag->idx = index;
        tag->type = _db[index].type;
        tag->count = _db[index].count;
        tag->attr = _db[index].attr;
        strcpy(tag->name, _db[index].name);
        return 0;
    }
}

/* Just returns the next tag index */
tag_index
get_tagindex(void) {
    return _tagnextindex;
}

/* Returns true if the tag at the given index is read only */
int
is_tag_readonly(tag_index idx) {
    return (_db[idx].attr & TAG_ATTR_READONLY );
}

/* Returns true if the tag at the given index is a virtual tag */
int
is_tag_virtual(tag_index idx) {
    return (_db[idx].attr & TAG_ATTR_VIRTUAL );
}

/* Returns true if tag at the given index is a queue */
int
is_tag_queue(tag_index idx) {
    return IS_QUEUE(_db[idx].type);
}

int
is_tag_owned(int fd, tag_index idx) {
    return (_db[idx].attr & TAG_ATTR_OWNED && _db[idx].fd == fd);
}


/* These are the low level tag reading / writing interface to the
 * database.
 *
 * This function reads the data from the tag given by handle at
 * the byte offset.  It will write size bytes into data and will
 * return 0 on success and some negative number on failure.
 *
 * The writing functions bypass the read only attribute of the tag
 */
int
tag_read(int fd, tag_index idx, int offset, void *data, int size)
{
    virt_functions *vf;
    int n, result;
    uint8_t x, y;

    /* Bounds check handle */
    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
     /* determine if it's a virtual tag */
    if(_db[idx].attr & TAG_ATTR_VIRTUAL) {
        /* virtual tags read their data from a function that should be
         * pointed to by the *data pointer */
        vf = (virt_functions *)_db[idx].data;
        if(vf->rf == NULL) return ERR_WRITEONLY;
        return vf->rf(fd, idx, offset, data, size, vf->userdata);
    } else {
        /* Bounds check size */
        if( (offset + size) > tag_get_size(idx)) {
            return ERR_2BIG;
        }
        if(_db[idx].data == NULL) {
            return ERR_DELETED;
        }
        /* Copy the data into the right place. */
        memcpy(data, &(_db[idx].data[offset]), size);
        /* If the tag is special then call the hook */
        /* NOTE: Not sure this should be before the override */
        if(_db[idx].attr & TAG_ATTR_SPECIAL) {
            result = special_tag_read(fd, idx, offset, data, size);
            if(result) return result;
        }
        if(_db[idx].attr & TAG_ATTR_OVR_SET) {
            for(n=0; n<size; n++) {
                 x = _db[idx].odata[n+offset] & _db[idx].omask[n+offset];
                 y = _db[idx].data[n+offset] & ~_db[idx].omask[n+offset];
                 ((uint8_t *)data)[n] = x | y;
            }
        }
    }

    return 0;
}

/* This function writes data to the _db just like the above function reads it */
int
tag_write(int fd, tag_index idx, int offset, void *data, int size)
{
    virt_functions *vf;
    int result;

    /* Bounds check handle */
    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    /* determine if it's a virtual tag */
    if(_db[idx].attr & TAG_ATTR_VIRTUAL) {
       /* virtual tags read their data from a function that should be
        * pointed to by the *data pointer */
       vf = (virt_functions *)_db[idx].data;
       if(vf->wf == NULL) return ERR_READONLY;
       return vf->wf(fd, idx, offset, data, size, vf->userdata);
    } else {
        /* Bounds check size */
        if( (offset + size) > tag_get_size(idx)) {
            return ERR_2BIG;
        }
        if(_db[idx].data == NULL) {
            return ERR_DELETED;
        }
        if(_db[idx].attr & TAG_ATTR_SPECIAL) {
            result = special_tag_write(fd, idx, offset, data, size);
            if(result) return result;
        }
        /* Copy the data into the right place. */
        memcpy(&(_db[idx].data[offset]), data, size);
        event_check(idx, offset, size);
    }

    if(_db[idx].attr & TAG_ATTR_RETAIN) {
        ret_tag_write(idx);
    }

    return 0;
}

/* Writes the data to the tagbase but only if the corresponding mask bit is set */
int
tag_mask_write(int fd, tag_index idx, int offset, void *data, void *mask, int size)
{
    uint8_t *db, *newdata, *newmask;
    int n, result;

    /* We don't allow masked writes to virtual tags.  This would be
     * too ambiguous */
    if(_db[idx].attr & TAG_ATTR_VIRTUAL) return ERR_ILLEGAL;
    /* Bounds check handle */
    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    /* Bounds check size */
    if( (offset + size) > tag_get_size(idx)) {
        return ERR_2BIG;
    }
    if(_db[idx].data == NULL) {
        return ERR_DELETED;
    }
    if(_db[idx].attr & TAG_ATTR_SPECIAL) {
        result = special_tag_mask_write(fd, idx, offset, data, mask, size);
        if(result) return result;
    }

    /* Just to make it easier */
    db = &_db[idx].data[offset];
    newdata = (uint8_t *)data;
    newmask = (uint8_t *)mask;
    for(n = 0; n < size; n++) {
        db[n] = (newdata[n] & newmask[n]) | (db[n] & ~newmask[n]);
    }
    event_check(idx, offset, size);

    if(_db[idx].attr & TAG_ATTR_RETAIN) {
        ret_tag_write(idx);
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

/* Receives a definition string in the form of "Name,Type,Count" and
 * appends that member to the compound datatype passed as *cdt.  Returns
 * 0 on success and dax error code on failure */
static int
cdt_append(datatype *cdt, char *str)
{
    int retval, count;
    cdt_member *this, *new;
    char *name, *typestr, *countstr, *last;
    tag_type type;

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
            dax_log(DAX_LOG_ERROR, "cdt_append() Duplicate name given");
            return ERR_DUPL;
        }
        this = this->next;
    }

    /* Allocate the new member */
    new = xmalloc(sizeof(cdt_member));
    if(new == NULL) {
        dax_log(DAX_LOG_ERROR, "Unable to allocate memory for new CDT member");
        return ERR_ALLOC;
    }
    /* Assign everything to the new datatype member */
    new->name = strdup(name);
    if(new->name == NULL) {
        free(new);
        dax_log(DAX_LOG_ERROR, "Unable to allocate memory for the name of the new CDT member");
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


/* Creates a compound datatype using the definition string in *str.
 * Returns the positive index of the newly created datatype if
 * sucessful or 0 on failure. If *error is not NULL any error codes
 * will be placed there otherwise zero will assigned to error. This
 * function uses strtok_r so the passed string can't be constant. */
tag_type
cdt_create(char *str, int *error) {
    int result;
    char *name, *member, *last, *tmp, *serial;
    datatype cdt;
    datatype *new_datatype;
    tag_type type;

    /* We need a duplicate of the string to help check if this exact
     * CDT has been added before.  In that case we don't return error */
    tmp = strdup(str);
    if(tmp == NULL) {
        if(error != NULL) *error = ERR_ALLOC;
        return 0;
    }
    /* This messes with *str.  It puts a '\0' everywhere there is a ':'
     * and that is okay because the calling function doesn't need it anymore
     * except for printing the name so this works out okay. */
    name = strtok_r(tmp, ":", &last);
    /* Check that the name is okay */
    if( (result = _validate_name(name)) ) {
        if(error != NULL) *error = result;
        free(tmp);
        return 0;
    }
    if((type = cdt_get_type(name))) {
        serialize_datatype(type, &serial);
        if(strcmp(serial, str)) { /* This means the two CDT's are not equal */
            if(error != NULL) *error = ERR_DUPL;
            free(tmp);
            return 0;
        } else { /* If it's the same then just return it */
            free(tmp);
            return type;
        }
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
            free(tmp);
            return 0;
        }
    }
    free(tmp);

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
    //--printf("create_cdt() - Created datatype %s\n", cdt.name);
    return CDT_TO_TYPE((_datatype_index - 1));
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
    if(!strcasecmp(name, "CHAR"))
        return DAX_CHAR;
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
            case DAX_CHAR:
                return "CHAR";
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

/* Returns a pointer to the CDT given by 'type' */
datatype *
cdt_get_entry(tag_type type) {
    int index;

    index = CDT_TO_INDEX(type);
    if(IS_CUSTOM(type) && index > 0 && index < _datatype_size) {
        return &_datatypes[index];
    } else {
        return NULL;
    }
}


/* Adds a member to the datatype referenced by it's array index 'cdt_index'.
 * Returns 0 on success, nonzero error code on failure. */
int
cdt_add_member(datatype *cdt, char *name, tag_type type, unsigned int count)
{
    int retval;
    cdt_member *this, *new;

    /* Check that the name is okay */
    if( (retval = _validate_name(name)) ) {
        return retval;
    }
    /* Check that the type is valid */
    if( _checktype(type) ) {
        dax_log(DAX_LOG_ERROR, "cdt_add_member() - datatype 0x%X does not exist");
        return ERR_ARG;
    }

    /* Check for duplicates */
    this = cdt->members;
    while(this != NULL) {
        if( !strcasecmp(name, this->name) ) {
            dax_log(DAX_LOG_ERROR, "cdt_add_member() - name %s already exists", name);
            return ERR_DUPL;
        }
        this = this->next;
    }

    /* Allocate the new member */
    new = xmalloc(sizeof(cdt_member));
    if(new == NULL) {
        dax_log(DAX_LOG_ERROR, "cdt_add_member() - Unable to allocate memory for new datatype member %s", name);
        return ERR_ALLOC;
    }
    /* Assign everything to the new datatype member */
    new->name = strdup(name);
    if(new->name == NULL) {
        free(new);
        dax_log(DAX_LOG_ERROR, "cdt_add_member() - Unable to allocate memory for member name %s", name);
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

int
override_add(tag_index idx, int offset, void *data, void *mask, int size) {
    int tag_size;

    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    if(_db[idx].data == NULL) {
        return ERR_DELETED;
    }
    tag_size = _db[idx].count * type_size(_db[idx].type);

    if(_db[idx].odata == NULL) {
        _db[idx].odata = malloc(tag_size);
        if(_db[idx].odata == NULL) return ERR_ALLOC;
        _db[idx].omask = malloc(tag_size);
        if(_db[idx].omask == NULL) {
            xfree(_db[idx].odata);
            _db[idx].odata = NULL;

            return ERR_ALLOC;
        }
        bzero(_db[idx].odata, size);
        bzero(_db[idx].omask, size);
    }
    if((offset + size) > tag_size) return ERR_2BIG;
    memcpy(&_db[idx].odata[offset], data, size);
    memcpy(&_db[idx].omask[offset], mask, size);

    /* If the flag for this tag is not already set then increment the counter */
    if(! (_db[idx].attr & TAG_ATTR_OVERRIDE )) {
        _ovrdinstalled++;
        if(_db[INDEX_OVRD_INSTALLED].data != NULL) {
            tag_write(-1, INDEX_OVRD_INSTALLED, 0, &_ovrdinstalled, sizeof(tag_index));
        }
    }

    _db[idx].attr |= TAG_ATTR_OVERRIDE; /* Indicate that we have an override installed */

    return 0;
}

int
override_del(tag_index idx, int offset, void *mask, int size) {
    int tag_size;
    int n;

    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    if(_db[idx].data == NULL) {
        return ERR_DELETED;
    }
    tag_size = _db[idx].count * type_size(_db[idx].type);
    if(_db[idx].odata == NULL) {
        return ERR_GENERIC;
    }
    for(n=0;n<size;n++) {
        _db[idx].omask[n+offset] &= ~((uint8_t *)mask)[n];
        _db[idx].odata[n+offset] &= ~((uint8_t *)mask)[n];
    }
    tag_size = _db[idx].count * type_size(_db[idx].type);
    for(n=0;n<tag_size;n++) {
        if(_db[idx].omask[n])  return 0;
    }
    /* Remove both of the flags that indicate we have an override installed or set */
    _db[idx].attr &= ~(TAG_ATTR_OVERRIDE | TAG_ATTR_OVR_SET);
    /* If we get here then we've deleted the last of the overrides for this
     * tag so we'll free the memory */
    _ovrdinstalled--;
    if(_db[INDEX_OVRD_INSTALLED].data != NULL) {
        tag_write(-1, INDEX_OVRD_INSTALLED, 0, &_ovrdinstalled, sizeof(tag_index));
    }
    xfree(_db[idx].odata);
    _db[idx].odata = NULL;
    xfree(_db[idx].omask);
    _db[idx].omask = NULL;

    return 0;
}


int
override_get(tag_index idx, int offset, int size, void *data, void *mask) {
    int tag_size;

    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    if(_db[idx].data == NULL) {
        return ERR_DELETED;
    }
    tag_size = _db[idx].count * type_size(_db[idx].type);
    if((offset + size) > tag_size) return ERR_2BIG;
    memcpy(data, _db[idx].data, size);
    memcpy(mask, _db[idx].omask, size);
    return 0;
}

int
override_set(tag_index idx, uint8_t flag) {
    if(idx < 0 || idx >= _tagnextindex) {
        return ERR_ARG;
    }
    if(_db[idx].data == NULL) {
        return ERR_DELETED;
    }
    if(flag) {
        if(_db[idx].odata == NULL) return ERR_ILLEGAL;
        if( !(_db[idx].attr & TAG_ATTR_OVR_SET)) {
            _ovrdset++;
            if(_db[INDEX_OVRD_SET].data != NULL) {
                tag_write(-1, INDEX_OVRD_SET, 0, &_ovrdset, sizeof(tag_index));
            }
            _db[idx].attr |= TAG_ATTR_OVR_SET;
        }
    } else {
        if(_db[idx].attr & TAG_ATTR_OVR_SET) {
            _ovrdset--;
            if(_db[INDEX_OVRD_SET].data != NULL) {
                tag_write(-1, INDEX_OVRD_SET, 0, &_ovrdset, sizeof(tag_index));
            }
            _db[idx].attr &= ~TAG_ATTR_OVR_SET;
        }
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

#ifdef TESTING
void
diag_list_tags(void)
{
    int n;
    for (n=0; n<_tagnextindex; n++) {
        printf("__db[%d] = %s[%d] type = %d\n", n, _db[n].name, _db[n].count, _db[n].type);
    }
}
#endif /* TESTING */
