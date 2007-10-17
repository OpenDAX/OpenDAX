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
 */

#include <tags.h>

/* Returns a random datatype to the caller */
int get_random_type(void) {
    switch (rand()%11) {
    case 0:
        return DAX_BOOL;        
    case 1:
        return DAX_BYTE;        
    case 2:
        return DAX_SINT;        
    case 3:
        return DAX_WORD;        
    case 4:
        return DAX_INT;        
    case 5:
        return DAX_UINT;        
    case 6:
        return DAX_DWORD;        
    case 7:
        return DAX_DINT;        
    case 8:
        return DAX_UDINT;        
    case 9:
        return DAX_TIME;        
    case 10:
        return DAX_REAL;        
    case 11:
        return DAX_LWORD;        
    case 12:
        return DAX_LINT;        
    case 13:
        return DAX_ULINT;        
    case 14:
        return DAX_LREAL;        
    default:
        return 0;
    }
}

void add_random_tags(int tagcount) {
    static int index = 0;
    int n, count, data_type;
    handle_t result;
    char tagname[32];
    
    srand(12); /* Random but not so random */
    for(n = 0; n < tagcount; n++) {
        sprintf(tagname,"randtag%d",index++);
        if(rand()%4 == 0) { /* One of four tags will be an array */
            count = rand() % 100 + 1;
        } else {
            count = 1; /* the rest are just tags */
        }
        data_type = get_random_type();
        result = dax_tag_add(tagname, data_type, count);
        if(result < 0) {
            xlog(10, "Failed to add Tag %s %s[%d]", tagname, dax_get_type(data_type), count );
        } else {
          //  xlog(10, "%s added at handle 0x%8X", tagname, result);
        }
    }
}

int check_tagbase(void) {
    dax_tag last_tag, this_tag;
    int n = 0;
    long int lastbit;
    
    if(n == 0) {
        if( dax_tag_byindex(n++, &last_tag) ) {
            xlog(10,"CheckTagBase() - Unable to get first tag");
            return -1;
        }
    }
    
    while( !dax_tag_byindex(n, &this_tag) ) {
        /* Check the sort order */
        if(this_tag.handle <= last_tag.handle) {
            xlog(10,"CheckTagBase() - Tag array not properly sorted at index %d",n);
            return -2;
        }
        /* check for overlap - Don't use TYPESIZE() macro.  We're testing it too */
        lastbit = last_tag.handle + ( 0x0001 << (last_tag.type & 0x0F)) * last_tag.count;
        if( lastbit > this_tag.handle) {
            xlog(10,"CheckTagBase() - Overlap in tag array at index %d",n);
            return -3;
        }
        /* TODO: calculate fragmentation, datasize etc. */
        memcpy(&last_tag, &this_tag, sizeof(dax_tag)); /* copy old to new */
        n++;
    }
    return 0;
}


static int tagtopass(char *name) {
    handle_t handle;
    
    handle = dax_tag_add(name, DAX_BOOL, 1);
    if(handle < 0) {
        xlog(1,"Test Failed - %s was not allowed", name);
        return -1;
    }
    return 0;
}

static int tagtofail(char *name) {
    handle_t handle;
    
    handle = dax_tag_add(name, DAX_BOOL, 1);
    if(handle >= 0) {
        xlog(1,"Test Failed - %s was allowed", name);
        return -1;
    }
    return 0;
}

int check_tag_addition(void) {    
    /* These tags should fail */
    if(tagtofail("1Tag")) return -1;
    if(tagtofail("-Tag")) return -1;
    if(tagtofail("Tag-name")) return -1;
    if(tagtofail("Tag&name")) return -1;
    if(tagtofail("TagNameIsWayTooLong12345678912345")) return -1;
    /* These tags should pass */
    if(tagtopass("_Tag")) return -1;
    if(tagtopass("Tag1")) return -1;
    if(tagtopass("tAg_name")) return -1;
    if(tagtopass("t1Ag_name")) return -1;
    if(tagtopass("TagNameIsBarelyLongEnoughToFit12")) return -1;
    return 0;
}

/* This test runs the tagname retrival process through it's paces */
static handle_t getshouldpass(char *name, handle_t h, unsigned int type) {
    dax_tag test_tag;
    xlog(2,"Testing tagname \"%s\" to find.", name);
    if(dax_tag_byname(name, &test_tag)) {
        xlog(1,"Test Failed - \"%s\" Should have been found", name);
        return -1;
    } else if(test_tag.handle != h) {
        xlog(1,"Test Failed - Handle doesn't match for \"%s\"", name);
        return -1;
    } else if(test_tag.type != type) {
        xlog(1, "Test Failed - Returned type doesn't match for \"%s\"", name);
        return -1;
    } else {
        return 0;
    }
}

/* This test runs the tagname retrival process through it's paces */
static handle_t getshouldfail(char *name) {
    dax_tag test_tag;
    xlog(2,"Testing tagname \"%s\" to not find.", name);
    if(! dax_tag_byname(name, &test_tag)) {
        xlog(1,"Test Failed - \"%s\" Should not have been found", name);
        return -1;
    } else {
        return 0;
    }
}

int check_tag_retrieve(void) {
    handle_t handle;
    
    handle = dax_tag_add("TestTag", DAX_INT, 10);
    if(handle < 0) {
        xlog(1,"OOPs TestTag should have been allowed to be added");
        return -1;
    }
    if(getshouldpass("TestTag", handle, DAX_INT)) return -1;
    if(getshouldpass("TestTag[0]", handle, DAX_INT)) return -1;
    if(getshouldpass("TestTag.0", handle, DAX_BOOL)) return -1;
    if(getshouldpass("TestTag.1", handle + 1, DAX_BOOL)) return -1;
    if(getshouldpass("TestTag.15", handle + 15, DAX_BOOL)) return -1;
    if(getshouldpass("TestTag[0].0", handle, DAX_BOOL)) return -1;
    if(getshouldpass("TestTag[1]", handle + 16, DAX_INT)) return -1;
    if(getshouldpass("TestTag[9]", handle + 16*9, DAX_INT)) return -1;
    if(getshouldfail("TestTag[10]")) return -1;
    if(getshouldpass("TestTag.159", handle + 159, DAX_BOOL)) return -1;
    if(getshouldfail("TestTag.160")) return -1;
    if(getshouldpass("TestTag[9].15", handle + 159, DAX_BOOL)) return -1;
    if(getshouldfail("TestTag[9].16")) return -1;
    if(getshouldfail("TestTag[8].16")) return -1;
    if(getshouldfail("TestTag[5].16")) return -1;
    
    return 0;
}