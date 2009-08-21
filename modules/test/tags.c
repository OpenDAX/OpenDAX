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

#include <daxtest.h>

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

int
add_random_tags(int tagcount, char *name)
{
    static int index = 0;
    int n, count, data_type;
    tag_index result;
    char tagname[32];
    
    srand(12); /* Random but not so random */
    for(n = 0; n < tagcount; n++) {
        /* TODO: Perhaps we should randomize the name too */
        sprintf(tagname,"%s%d_%d", name, rand()%100, index++);
        if(rand()%4 == 0) { /* One of four tags will be an array */
            count = rand() % 100 + 1;
        } else {
            count = 1; /* the rest are just tags */
        }
        data_type = get_random_type();
        result = dax_tag_add(NULL, tagname, data_type, count);
        if(result < 0) {
            dax_debug(LOG_MINOR, "Failed to add Tag %s %s[%d]", tagname, dax_type_to_string(data_type), count );
            return -1;
        } else {
            dax_debug(LOG_MINOR, "%s added at index 0x%08X", tagname, result);
        }
    }
    return 0;
}

/* This function expects that the passed tagname will be allowed to be added
 * by the server.  Returns error otherwise and the test will fail */
int
tagtopass(const char *name)
{
    tag_index index;
    
    index = dax_tag_add(NULL, (char *)name, DAX_BOOL, 1);
    if(index < 0) {
        dax_debug(LOG_MINOR, "Test Failed - %s was not allowed", name);
        return -1;
    }
    return 0;
}

/* Same as above excpet that this tag is expected to fail. */
int
tagtofail(const char *name)
{
    tag_index index;
    
    index = dax_tag_add(NULL, (char *)name, DAX_BOOL, 1);
    if(index >= 0) {
        dax_debug(LOG_MINOR, "Test Failed - %s was allowed", name);
        return -1;
    }
    return 0;
}


/******************************
 * TODO: Stuff below hasn't been added to Lua Yet

 * This test runs the tagname retrival process through it's paces */
/* TODO: add some kind of mechanism to make sure that we get the right index or bit offset */
//static int
//getshouldpass(char *name, tag_idx_t h, int index, int bit, unsigned int type, unsigned int count)
//{
//    dax_tag test_tag;
//    dax_debug(2,"Testing tagname \"%s\" to find.", name);
//    if(dax_tag_byname(&test_tag, name)) {
//        dax_debug(1,"Test Failed - \"%s\" Should have been found", name);
//        return -1;
//    } else if(test_tag.idx != h) {
//        dax_debug(1,"Test Failed - Handle doesn't match for \"%s\"", name);
//        return -1;
//    } else if(test_tag.type != type) {
//        dax_debug(1, "Test Failed - Returned type doesn't match for \"%s\"", name);
//        return -1;
//    } else if(test_tag.count != count) {
//        dax_debug(1, "Test Failed - Returned count doesn't match for \"%s\"", name);
//        return -1;
//    } else {
//        return 0;
//    }
//}
//
///* This test runs the tagname retrival process through it's paces */
//static int
//getshouldfail(char *name)
//{
//    dax_tag test_tag;
//    dax_debug(2, "Testing tagname \"%s\" to not find.", name);
//    if(! dax_tag_byname(&test_tag, name)) {
//        dax_debug(1,"Test Failed - \"%s\" Should not have been found", name);
//        return -1;
//    } else {
//        return 0;
//    }
//}


/* Test's the different aspects of the tag database */
int
check_tagbase(void)
{
    tag_index n = 0;
    dax_tag last_tag, this_tag;
    long int lastbit;
 
    if( dax_tag_byindex(&last_tag, n++) ) {
        dax_debug(10, "CheckTagBase() - Unable to get first tag");
        return -1;
    }
    
    while( !dax_tag_byindex(&this_tag, n) ) {
        /* Check the sort order */
        if(this_tag.idx <= last_tag.idx) {
            dax_debug(10, "CheckTagBase() - Tag array not properly sorted at index %d", n);
            return -2;
        }
        printf("Tag being checked = %s\n", last_tag.name);
        /* check for overlap - Don't use TYPESIZE() macro.  We're testing it too */
        lastbit = last_tag.idx + ( 0x0001 << (last_tag.type & 0x0F)) * last_tag.count;
        //if( lastbit > this_tag.handle) {
        //    dax_debug(10, "CheckTagBase() - Overlap in tag array at index %d", n);
        //    return -3;
        //}
        memcpy(&last_tag, &this_tag, sizeof(dax_tag)); /* copy old to new */
        n++;
    }
    return 0;
}


int
check_tag_events(void)
{
    tag_index handle;
    int id;
    
    handle = dax_tag_add(NULL, "EventTest", DAX_DINT, 10);
    if(handle < 0) {
        dax_debug(1, "Unable to add EventTest Tag");
        return -1;
    }
    id = dax_event_add("EventTest[2]", 1);
    if(id < 0) {
        if(id == ERR_MSG_SEND) {
            dax_debug(1, "Unable to send message to Add Event for EventTest[2]");
        } else if(id == ERR_MSG_RECV) {
            dax_debug(1, "No response to Add Event for EventTest[2]");
        } else {
            dax_debug(1, "Error from dax_event_add() is %d", id);
        }
        return -1;
    }
    return 0;
}
