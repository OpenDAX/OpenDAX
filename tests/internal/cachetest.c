/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2020 Phil Birkelbach
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
 *  Main source code file for the OpenDAX Bad Module
 */

/* This test program loads up the caching functions from the library and
 * tests that the functions that deal with the cache behave correctly
 */

#include <libcommon.h>
#include "libdax.h"


static void
print_cache(dax_state *ds)
{
    int n;
    tag_cnode *this;
    this = ds->cache_head;
    if(this == NULL) {
        printf("Empty Cache\n");
        return;
    }
    printf("cache_head->idx = {%d}\n", ds->cache_head->idx);
    for(n=0; n<ds->cache_count; n++) {
        printf("{%d:%p} - %s\t", this->idx, this, this->name);
        printf("this->prev = {%d:%p}", this->prev->idx, this->prev);
        printf(" : this->next = {%d:%p}\n", this->next->idx, this->next);
        this = this->next;
    }
    printf("{ count = %d, head = %p }\n\n", ds->cache_count, ds->cache_head);
    //printf("{ head->handle = %d, tail->handle = %d head->prev = %p, tail->next = %p}\n", 
    //       _cache_head->handle, _cache_tail->handle, _cache_head->prev, _cache_tail->next);
}

static int
check_cache_hit(dax_state *ds, dax_tag tag)
{
    dax_tag check_tag;
    int result;
    
    result = check_cache_index(ds, tag.idx, &check_tag);
    if(result) {
        printf("Tag with index %d was not in the cache\n", tag.idx);
        exit(result);
    }
    if(strcmp(tag.name, check_tag.name)) {
        printf("The tag we got back was not the correct one\n");
        exit(-1);
    }
    result = check_cache_name(ds, tag.name, &check_tag);
    if(result) {
        printf("Tag with name %s was not in the cache\n", tag.name);
        exit(result);
    }
    if(tag.idx != check_tag.idx) {
        printf("The tag we got back was not the correct one\n");
        exit(-1);
    }
    return 0;
}

static int
check_cache_miss(dax_state *ds, dax_tag tag)
{
    dax_tag check_tag;
    int result;
    
    result = check_cache_index(ds, tag.idx, &check_tag);
    if(result != ERR_NOTFOUND) {
        printf("Tag with index %d was found in the cache\n", tag.idx);
        exit(-1);
    }
    result = check_cache_name(ds, tag.name, &check_tag);
    if(result != ERR_NOTFOUND) {
        printf("Tag with name %s was found in the cache\n", tag.name);
        exit(-1);
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    dax_state *ds;
    tag_index head;
    dax_tag tags[100];
    char tagname[DAX_TAGNAME_SIZE + 1];
    int n, result;
    
    ds = dax_init("cachetest");
    dax_init_config(ds, "cachetest");
    dax_configure(ds, argc, argv, CFG_CMDLINE);
    printf("cachesize = %s\n", dax_get_attr(ds, "cachesize"));
    init_tag_cache(ds);
    for(n=0;n<100;n++) {
        sprintf(tagname, "TestTag%d", n);
        strcpy(tags[n].name, tagname);
        tags[n].count = 1;
        tags[n].idx = n;
        tags[n].type = DAX_INT;
    }
    for(n=0;n<8;n++) {
        cache_tag_add(ds, &tags[n]);
        print_cache(ds);
    }

    /* Delete one that we know we don't have */
    cache_tag_del(ds, 15);
    /* Make sure they are all still in there */
    for(n=0;n<8;n++) {
        check_cache_hit(ds, tags[n]);
    }
    /* Delete one that we know is in there */
    printf("Delete tag from cache\n");
    cache_tag_del(ds, 3);
    print_cache(ds);
    /* Make sure it's not in there */
    check_cache_miss(ds, tags[3]);
    /* Make sure that the rest are still there */
    for(n=0;n<8;n++) {
        if(n != 3) {
            check_cache_hit(ds, tags[n]);
        }
    }
    /* Put it back to make stuff simpler */
    printf("Add 3 back\n");
    cache_tag_add(ds, &tags[3]);
    print_cache(ds);
    head = ds->cache_head->idx;
    printf("head index = %d\n", head);
    /* Delete the head and make sure it all still works */
    printf("Delete head\n");
    cache_tag_del(ds, head);
    print_cache(ds); 
    check_cache_miss(ds, tags[head]);
    for(n=0;n<8;n++) {
        if(n != head) {
            check_cache_hit(ds, tags[n]);
        }
    }            
    print_cache(ds);
    for(n=0;n<8;n++) {
        cache_tag_del(ds, tags[n].idx);
        print_cache(ds);
    }
    for(n=0;n<8;n++) {
        check_cache_miss(ds, tags[n]);
    }
    printf("Free cache\n");
    for(n=0;n<8;n++) {
        cache_tag_add(ds, &tags[n]);
        print_cache(ds);
    }
    free_tag_cache(ds);
    print_cache(ds);
    dax_free_config(ds);
    dax_free(ds);
    return 0;
}
