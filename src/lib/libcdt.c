/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2009 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * This is the source file for Custom Data Type handling code in the library
 */

#include <libdax.h>
#include <libcommon.h>
#include <ctype.h>

/* This defines the starting size of the datatype array.
 * It is also the amount that the datatype array will
 * grow when necessary */
#ifndef DAX_DATATYPE_SIZE
# define DAX_DATATYPE_SIZE 8
#endif

/* Inserts the given datatype into the array */
static int
_insert_type(dax_state *ds, int index, char *typename) {
    /* Right now all we are doing is checking that the name isn't
     * too big.  We are assuming that the server hasn't sent us a
     * name that is malformed. */
    if(strlen(typename) <= DAX_TAGNAME_SIZE) {
        ds->datatypes[index].name = strdup(typename);
    }
    if(ds->datatypes[index].name == NULL) {
        return ERR_ALLOC;
    }
    return 0;
}

/* Adds the member that is described by the string desc.  This string
 * would look like the one generated by the serialize_datatype() function
 * in the server.  The index is the array index in _datatypes[] */
static int
_add_member_to_cache(dax_state *ds, int index, char *desc) {
    char *str, *last;
    char *name, *type;
    int count;
    cdt_member *new, *this;

    name = type = NULL;
    count = 0;

    new = malloc(sizeof(cdt_member));
    if(new == NULL) {
        return ERR_ALLOC;
    }

    //--printf("Member string = '%s'\n", desc);
    name = strtok_r(desc, ",", &last);
    if(name) {
        type = strtok_r(NULL, ",", &last);
        if(type) {
            str = strtok_r(NULL, ",", &last);
            if(str) {
                count = (int)strtol(str, (char **)NULL, 10);
            }
        }
    }

    /* TODO: Need to do some error checking here */
    if(name && type && (count > 0)) {
        new->name = strdup(name);
        new->type = dax_string_to_type(ds, type);
        new->count = count;
        new->next = NULL;
        //--printf("_add_member() - name = %s type = 0x%X count = %ld\n", new->name, new->type, new->count);
        /* Add the new member to the end of the linked list */
        this = ds->datatypes[index].members;
        if(this == NULL) {
            ds->datatypes[index].members = new;
        } else {
            while(this->next != NULL) this = this->next;
            this->next = new;
        }
    } else {
        free(new);
        return ERR_ARG;
    }

    return 0;
}

/*!
 * Calculate the size (in bytes) of the datatype.  This function
 * can be used on all datatypes including compound data types.
 *
 * @param ds The pointer to the dax state object
 * @param type The type whose size we are trying to get
 *
 * @returns the size of the given datatype
 */
int
dax_get_typesize(dax_state *ds, tag_type type)
{
    int size = 0;
    unsigned int pos = 0; /* Bit position within the data area */
    cdt_member *this;

    type &= ~DAX_QUEUE; /* Delete the Queue bit from the type */
    if( dax_type_to_string(ds, type) == NULL )
        return ERR_ARG;

    if(IS_CUSTOM(type)) {
        this = ds->datatypes[CDT_TO_INDEX(type)].members;
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
                    pos += (dax_get_typesize(ds, this->type) * this->count) * 8;
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

/* Retrieves a pointer to the datatype array element identified by type
 * returns NULL on failure and if the error pointer is not NULL the error
 * code will be placed there. */
datatype *
get_cdt_pointer(dax_state *ds, tag_type type, int *error)
{
    int index;
    int result;

    if(error != NULL) *error = 0;
    if(IS_CUSTOM(type)) {
        index = CDT_TO_INDEX(type);
        if(index < ds->datatype_size && ds->datatypes[index].name != NULL) {
            return &ds->datatypes[index];
        } else {
            result = dax_cdt_get(ds, type, NULL);
            if(result == 0) {
                return &ds->datatypes[index];
            } else {
                if(error != NULL) *error = result;
            }
            return NULL;
        }
    } else {
        return NULL;
    }
}

/* Adds the given type to the array cache.  'type' is the type id
 * and typedesc is the type description string that would be generated
 * by a call to serialize_datatype() in the server. */
int
add_cdt_to_cache(dax_state *ds, tag_type type, char *typedesc)
{
    int index, result, n;
    char *str, *last;

    index = CDT_TO_INDEX(type);

    if(ds->datatype_size == 0) { /* Make sure we have an array */
        /* Allocate the datatype array and set the initial counters */
        ds->datatypes = malloc(sizeof(datatype) * DAX_DATATYPE_SIZE);
        if(ds->datatypes == NULL) {
            return ERR_ALLOC;
        }
        /* Set both pointers to NULL */
        for(n = 0; n < DAX_DATATYPE_SIZE; n++) {
            ds->datatypes[n].name = NULL;
            ds->datatypes[n].members = NULL;
        }
        ds->datatype_size = DAX_DATATYPE_SIZE;
    }

    /* Do we need to grow the array */
    while(index >= ds->datatype_size) {
        /* Allocate more space for the array */
        datatype *new_datatype;

        new_datatype = realloc(ds->datatypes, (ds->datatype_size + DAX_DATATYPE_SIZE) * sizeof(datatype));

        if(new_datatype != NULL) {
            ds->datatypes = new_datatype;
            /* Set both pointers to NULL */
            for(n = ds->datatype_size; n < ds->datatype_size + DAX_DATATYPE_SIZE; n++) {
                ds->datatypes[n].name = NULL;
                ds->datatypes[n].members = NULL;
            }
            ds->datatype_size += DAX_DATATYPE_SIZE;
        } else {
            return ERR_ALLOC;
        }
    }

    /* At this point we should have the spot for the datatype */
    str = strtok_r(typedesc, ":", &last);
    if(str == NULL) {
        dax_log(LOG_ERROR, "add_cdt_to_cache(): Something is seriously wrong with the string");
        return ERR_ARG;
    }
    result = _insert_type(ds, index, str);

    while( (str = strtok_r(NULL, ":", &last)) ) {
        result = _add_member_to_cache(ds, index, str);
        if(result) return result;
    }
    //--printf("add_cdt_to_cache() type = 0x%X, name = %s\n", type, typedesc);
    return 0;
}

/*! 
 * Creates an empty Custom Datatype
 *
 * @param name The name that will be given to the new type
 * @param errors a pointer to an integer that will contain
 *               any errors that we encounter
 * @returns NULL on error or a pointer to the new
 *          datatype if successful.
 */
dax_cdt *
dax_cdt_new(char *name, int *error)
{
    int result = 0;
    dax_cdt *new = NULL;

    if(strlen(name) > DAX_TAGNAME_SIZE) {
        result = ERR_2BIG;
    } else {
        new = malloc(sizeof(dax_cdt *));
        if(new == NULL) {
            result = ERR_ALLOC;
        } else {
            new->members = NULL;
            new->name = strdup(name);
            if(new->name == NULL) {
                free(new);
                new = NULL;
                result = ERR_ALLOC;
            }
        }
    }
    if(result != 0 && error != NULL) {
        *error = result;
    }

    return new;
}


/*!
 * Adds a member to the Custom Datatype.
 *
 * @param ds The pointer to the dax state object
 * @param cdt The pointer to the cdt that was returned by dax_cdt_new
 * @param name The name of the member that we are adding
 * @param type The data type of the member that we are adding.
 *             This can be another custom data type.  Nesting
 *             data types is allowed.
 * @param count The number of items if this member is an array.
 *
 * @returns Zero on success and an error code otherwise
 */
int
dax_cdt_member(dax_state *ds, dax_cdt *cdt, char *name, tag_type type, unsigned int count)
{
    cdt_member *new = NULL, *this, *last;

    /* Duplicate Check */
    this = cdt->members;
    last = this;
    while(this != NULL) {
        last = this;
        if(strcmp(name, this->name) == 0) return ERR_DUPL;
        this = this->next;
    }
    /* Valid type check */
    if(dax_type_to_string(ds, type) == NULL) return ERR_ARG;
    /* Size check */
    if(strlen(name) > DAX_TAGNAME_SIZE) return ERR_2BIG;

    /* Allocate the new member */
    new = malloc(sizeof(cdt_member));
    if(new == NULL) return ERR_ALLOC;

    /* Assign values */
    new->name = strdup(name);
    if(new->name == NULL) return ERR_ALLOC;
    new->type = type;
    new->count = count;
    new->next = NULL;

    /* Put it in the linked list */
    if(cdt->members == NULL) cdt->members = new;
    else last->next = new;

    return 0;
}

static void
_cdt_member_free(cdt_member *member) {
    if(member->next != NULL) _cdt_member_free(member->next);
    if(member->name != NULL) free(member->name);
    free(member);
}

/*! 
 * Free the compound data type.  This function will only be
 * used by the module if the dax_cdt_create() function is never
 * called for some reason, because the cdt_create() function
 * frees the CDT once it has been created in the server.
 *
 * @param cdt The custom data type that was returned by dax_cdt_new()
 */
void
dax_cdt_free(dax_cdt *cdt) {
    if(cdt->members != NULL) _cdt_member_free(cdt->members);
    if(cdt->name != NULL ) free(cdt->name);
    free(cdt);
}


/*!
 * Find the data type of the given name and return it's numeric ID.
 * both base data types and compound data types can be found with
 * this function.
 * 
 * @param ds Pointer to the dax state object
 * @param type A string that represents the name of the data type
 * @returns 0 on error
 */
tag_type
dax_string_to_type(dax_state *ds, char *type)
{
    int result, n;

    if(type == NULL) return 0;
    if(!strcasecmp(type, "BOOL"))  return DAX_BOOL;
    if(!strcasecmp(type, "BYTE"))  return DAX_BYTE;
    if(!strcasecmp(type, "SINT"))  return DAX_SINT;
    if(!strcasecmp(type, "CHAR"))  return DAX_CHAR;
    if(!strcasecmp(type, "WORD"))  return DAX_WORD;
    if(!strcasecmp(type, "INT"))   return DAX_INT;
    if(!strcasecmp(type, "UINT"))  return DAX_UINT;
    if(!strcasecmp(type, "DWORD")) return DAX_DWORD;
    if(!strcasecmp(type, "DINT"))  return DAX_DINT;
    if(!strcasecmp(type, "UDINT")) return DAX_UDINT;
    if(!strcasecmp(type, "TIME"))  return DAX_TIME;
    if(!strcasecmp(type, "REAL"))  return DAX_REAL;
    if(!strcasecmp(type, "LWORD")) return DAX_LWORD;
    if(!strcasecmp(type, "LINT"))  return DAX_LINT;
    if(!strcasecmp(type, "ULINT")) return DAX_ULINT;
    if(!strcasecmp(type, "LREAL")) return DAX_LREAL;

    /* See if we already have it in the cache */
    for(n = 0; n<ds->datatype_size; n++) {
        if(ds->datatypes[n].name != NULL && !strcasecmp(type, ds->datatypes[n].name)) {
            return CDT_TO_TYPE(n);
        }
    }

    /* If not go to the server for it */
    result = dax_cdt_get(ds, 0, type);

    if(result) {
        return 0;
    } else {
        /* Search Again - It'll be there this time */
        for(n = 0; n < ds->datatype_size; n++) {
            if(ds->datatypes[n].name != NULL && !strcasecmp(type, ds->datatypes[n].name)) {
                return CDT_TO_TYPE(n);
            }
        }
    }
    /* We should have found it by now.  If not return error */
    return 0;
}

/*!
 * Returns a pointer to a string that is the name of the data type
 * 
 * @param ds Pointer to the dax state object
 * @param type The numeric type identifier
 *
 * @returns A pointer to the name of the data type or NULL on error
 */
const char *
dax_type_to_string(dax_state *ds, tag_type type)
{
    int index;

    /* TODO: Could replace this with a call to get_cdt_pointer() */
    if(IS_CUSTOM(type)) {
        index = CDT_TO_INDEX(type);
        if(index < ds->datatype_size && ds->datatypes[index].name != NULL) {
            return ds->datatypes[index].name;
        } else {
            if(!dax_cdt_get(ds, type, NULL)) {
                return ds->datatypes[index].name;
            }
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
        }
    }
    return NULL;
}

/* This function finds the next '.' in "name" and replaces it with '\0'
 * If there is no '.' it returns NULL */
static char *
_split_tagname(char *name)
{
    char *nextname = NULL;
    int n = 0;

    while(name[n] != '.' && name[n] != '\0') {
        n++;
    }

    if(name[n] == '.') { /* If we have more members in the string */
        name[n] = '\0';
        nextname = name + n + 1; /* This points to the next character after the '.' */
    }
    return nextname;
}

/* Looks for a number within [ ] and returns that as an index
 * if it finds problems within the [ ] it returns ERR_ARG if
 * there are no [ ] then it returns ERR_NOTFOUND which is not
 * really an error in this case but we need to know anyway.
 * It if it finds a '[' it replaces it with a '\0' so that
 * the calling function will just see the string */
static inline int
_get_index(char *str)
{
    int i, k, len, index;

    len = strlen(str);
    for(i = 0; i < len && str[i] != '['; i++);
    if(i < len) {
        str[i] = '\0'; /* To end the string */
        for(k = i+1; k < len && str[k] != ']'; k++) {
            if(str[k] < '0' || str[k] > '9') return ERR_ARG;
        }
        index = atoi(str + i + 1);
    } else {
        index = ERR_NOTFOUND;
    }
    return index;
}

/* called when a bit index is found in the tag string.  i.e. tagname.3.  The
 * function determines if all is okay and if so it finishes the handle definition
 * and returns 0 otherwise it returns an error */
static int
_parse_bit_index(dax_state *ds, tag_type type, tag_handle *h, char *digit, int index, int count)
{
    char *endptr;
    int size;

    if(type == DAX_BOOL || type == DAX_TIME ||
       type == DAX_REAL || type == DAX_LREAL ||
       IS_CUSTOM(type)) {
           return ERR_BADTYPE;
       }
    if(index < 0) index = 0;
    h->bit = strtol(digit, &endptr, 10);
    /* This indicates that there is extra text past the number */
    if(endptr[0] != '\0') {
        return ERR_ARG;
    }
    size = dax_get_typesize(ds, type);
    if(h->bit > size*8) {
        return ERR_2BIG;
    }
    if(count == 0) count = 1;
    if((h->bit + count) > size*8) {
        return ERR_2BIG;
    }
    h->byte += size * index + (h->bit / 8);
    h->bit = h->bit % 8;
    h->type = DAX_BOOL;
    h->size = (h->bit + count - 1) / 8 - (h->bit / 8) + 1;
    h->count = count;
    return 0;
}

static int
_parse_next_member(dax_state *ds, tag_type lasttype, tag_handle *h, char *str, int count)
{
    int index, result, size;
    char *name = str;
    char *nextname;
    cdt_member *this;

    if(name == NULL) return ERR_ARG;

    nextname = _split_tagname(str);

    index = _get_index(name);
    if(index < 0 && index != ERR_NOTFOUND) {
        return index;
    }

    if(dax_type_to_string(ds, lasttype) == NULL) {
        return ERR_NOTFOUND; /* This is a serious problem here */
    }

    this = ds->datatypes[CDT_TO_INDEX(lasttype)].members;

    while(this != NULL) {
        /* Start by adding all the bytes of the members before the
         * one we are looking for */

        if(strcmp(name, this->name)) {
            if(this->type == DAX_BOOL) {
                h->bit += this->count;
                if(h->bit > 7) {
                    h->byte += h->bit / 8;
                    h->bit %= 8;
                }
            } else {
                /* Gotta step up one if the last member was a bool */
                if(h->type == DAX_BOOL) h->byte++;
                h->byte += dax_get_typesize(ds, this->type) * this->count;
                h->bit = 0;
            }
            h->type = this->type; /* We need to keep track of the last type */

            this = this->next;
        } else { /* Here we have found it */
            break;
        }

    }
    if(this == NULL) return ERR_NOTFOUND;

    if(nextname) { /* Not the last item */
        if(isdigit(nextname[0])) {
            if(this->count > 1 && index <0) {
                return ERR_ARG;
            }
            return _parse_bit_index(ds, this->type, h, nextname, index, count);
        }
        result = _parse_next_member(ds, this->type, h, nextname, count);
        if(result) return result;
        if(index != ERR_NOTFOUND) {
            if(count == 0) count = 1;
            if((index + count) > this->count) return ERR_2BIG;
            h->byte += dax_get_typesize(ds, this->type) * index;
        }
    } else { /* We are the last item */
        if(h->type == DAX_BOOL && this->type != DAX_BOOL) {
            h->byte++;
            h->bit = 0;
        }

        h->type = this->type;
        if(index != ERR_NOTFOUND){
            if(count == 0 ) count = 1;
            if((index + count) > this->count ) return ERR_2BIG;
            if(this->type == DAX_BOOL) {
                h->byte += index / 8;
                h->bit += index % 8;
                /* Two bits across the byte boundry require two bytes */
                h->size = (h->bit + count - 1) / 8 - (h->bit / 8) + 1;
                h->count = count;
            } else {
                size = dax_get_typesize(ds, this->type);
                h->size = size * count;
                h->byte += size * index;
                h->bit = 0;
                h->count = count;
            }
        } else { /* This is where no index was given */
            if(count > this->count) return ERR_2BIG;
            if(count == 0) count = this->count;
            if(this->type == DAX_BOOL) {
                h->size = (h->bit + count - 1) / 8 - (h->bit / 8) + 1;
                h->count = count;
            } else {
                size = dax_get_typesize(ds, this->type);
                h->bit = 0;
                h->size = size * count;
                h->count = count;
            }
        }
    }
    return 0;
}

/* Wrapping the function to take advantage of the variable length array
 * feature of C for the tagname so we don't have to worry about malloc */
static int
_dax_tag_handle(dax_state *ds, tag_handle *h, char *str, int strlen, int count)
{
    int result, index, size;
    dax_tag tag;
    char tagname[strlen]; /* Hack alert */
    char *membername;

    strcpy(tagname, str);
    membername = _split_tagname(tagname);
    index = _get_index(tagname); /* Get array index inside [] */
    if(index < 0 && index != ERR_NOTFOUND) {
        return index;
    }

    result = dax_tag_byname(ds, &tag, tagname);
    if(result) return ERR_NOTFOUND;
    h->index = tag.idx; /* This index is the database index */

    if(membername) {
        /* This is the case where the .x is a number referring to a bit */
        if(isdigit(membername[0])) {
            if(tag.count > 1 && index < 0) {
                return ERR_ARG;
            }
            return _parse_bit_index(ds, tag.type, h, membername, index, count);
        }
        else if(!IS_CUSTOM(tag.type)) {
            dax_log(LOG_ERROR, "Tag %s, has no member %s", tagname, membername);
            return ERR_ARG;
        }
        if(tag.count > 1 && index == ERR_NOTFOUND) {
            dax_log(LOG_ERROR, "Ambiguous reference in tag %s", tagname);
            return ERR_ARBITRARY;
        }
        result = _parse_next_member(ds, tag.type, h, membername, count);
        if(result) return result;
        if(index != ERR_NOTFOUND) {
            if(count == 0) count = 1;
            if((index + count) > tag.count) return ERR_2BIG;
            h->byte += dax_get_typesize(ds, tag.type) * index;
        }
    } else {
        h->type = tag.type;
        if(index != ERR_NOTFOUND){
            if(count == 0 ) count = 1;
            if((index + count) > tag.count ) return ERR_2BIG;
            if(tag.type == DAX_BOOL) {
                h->byte = index / 8;
                h->bit = index % 8;
                h->size = (h->bit + count - 1) / 8 - (h->bit / 8) + 1;
                h->count = count;
            } else {
                size = dax_get_typesize(ds, tag.type);
                h->size = size * count;
                h->byte = size * index;
                h->bit = 0;
                h->count = count;
            }
        } else { /* This is where no index [] was given */
            if(count > tag.count) return ERR_2BIG;
            if(count == 0) count = tag.count;
            if(tag.type == DAX_BOOL) {
                h->byte = 0;
                h->bit = 0;
                h->size = (h->bit + count - 1) / 8 - (h->bit / 8) + 1;
                h->count = count;
            } else {
                size = dax_get_typesize(ds, tag.type);
                h->size = size * count;
                h->byte = 0;
                h->bit = 0;
                h->count = count;
            }
        }
    }
    return 0;
}

/*!
 * This function retrieves a handle to the tag or tag fragment that is
 * represented by 'str'.  The handle is a complete representation of where
 * the data is located in the server.  It is passed to the reading/writing
 * functions to retrieve the data. If count is 0 then a handle to the whole tag
 * or tag member is returned.
 * 
 * @param ds Pointer to the dax state object
 * @param h Pointer to the handle that we wish to be filled in by the function
 * @param str The string that represents the tag or tag fragment that we wish
 *            to get a handle to.
 * @param count The number of items of the tag that we wish to identify.  If set
 *              to zero the entire tag will be assumed.
 * @returns Zero on success or an error code otherwise
 */
int
dax_tag_handle(dax_state *ds, tag_handle *h, char *str, int count)
{
    int result;
    if(h == NULL || ds == NULL || str == NULL) {
        return ERR_ARG;
    }
    bzero(h, sizeof(tag_handle)); /* Initialize h */
    result = _dax_tag_handle(ds, h, str, strlen(str) + 1, count);
    if(result) {
        bzero(h, sizeof(tag_handle)); /* Reset h in case of error */
    }
    return result;
}


/*!
 * This is the compound data type iterator.  If type is a CDT then this
 * function iterates over each member of the data type and calls 'callback'
 * with the cdt_iter structure and passes back the udata pointer as well.
 * If type is 0 then it iterates over the list of data types.  In this case
 * the name and type fields in cdt_iter are the only things that are relevant.
 * 
 * @param ds Pointer to dax state object
 * @param type The data type that we wish to iterate over.  If set to zero
 *             This will iterate over the list of CDTs in the server
 * @param udata Pointer to user data that will be passed to the callback
 * @param callback Function that will be called for each member in the CDT
 * @returns Zero on success or an error code otherwise
 */
int
dax_cdt_iter(dax_state *ds, tag_type type, void *udata, void (*callback)(cdt_iter member, void *udata))
{
    datatype *dt;
    cdt_member *this;
    cdt_iter iter;
    int result;
    int byte = 0;
    int bit = 0;
    int index = 0;

    if(type == 0) { /* iterate through the custom types */
        dt = get_cdt_pointer(ds, CDT_TO_TYPE(index), &result);

        while(result == 0 || result == ERR_INUSE) {
            if(result == 0) {
                iter.name = dt->name;
                iter.type = CDT_TO_TYPE(index);
                iter.count = iter.byte = iter.bit = 0;
                callback(iter, udata);
            }
            index++;
            dt = get_cdt_pointer(ds, CDT_TO_TYPE(index), &result);
        }
    } else { /* iterate over the members */
        dt = get_cdt_pointer(ds, type, &result);
        if(dt == NULL) {
            return result;
        }

        this = dt->members;
        while(this != NULL) {
            iter.name = this->name;
            iter.type = this->type;
            iter.count = this->count;
            iter.bit = bit;
            iter.byte = byte;
            callback(iter, udata); /* Call the callback */
            /* No sense in calculating the offsets if we're done */
            if(this->next != NULL) {
                if(this->type == DAX_BOOL) {
                    bit += this->count % 8;
                    byte += this->count / 8;
                    if(bit > 7) { /* adjust for the overflow */
                        bit %= 8;
                        byte ++;
                    }
                    /* if the next one is not a BOOL then we should align */
                    if(this->next->type != DAX_BOOL && bit != 0) {
                        bit = 0;
                        byte++;
                    }
                } else { /* Not this->type == DAX_BOOL */
                    bit = 0;
                    byte += dax_get_typesize(ds, this->type) * this->count;
                }
            }
            this = this->next;
        }
    }
    return 0;
}
