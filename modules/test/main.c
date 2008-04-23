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
 *  Main source code file for the OpenDAX test module
 */

#include <common.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <opendax.h>
#include <string.h>
#include <tags.h>

#define TEST 1000

/* TODO: Rewrite this testing module to use a Lua script
   to decide what gets tested */

int main(int argc,char *argv[]) {
    int tests_failed = 0;
    int n;
    handle_t handle;
    //char tagname[DAX_TAGNAME_SIZE +1];
    u_int16_t dummy[20], test[20];
    
    //openlog("test", LOG_NDELAY, LOG_DAEMON);
    dax_log("Starting module test");
    dax_set_verbosity(1);
    dax_set_debug_topic(0xFFFF); /* This should get them all out there */
    
    if(dax_mod_register("test"))
        dax_fatal("Unable to register with the server");
    
    if(check_tag_addition()) {
        dax_log("Tagname addition test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tagname addition test - PASSED");
    }
    
    if(check_tag_retrieve()) {
        dax_log("Tagname retrieving test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tagname retrieving test - PASSED");
    }
    
    //add_random_tags(100);
    /* TEST TO DO
        Tag Read / Write Test
    */
    
#ifdef DKFOOEIT74YEJKIOUWERHLVKSJDHIUHFEW
    if(check_tag_events()) {
        dax_log("Tag Event Test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tag Event Test - PASSED");
    }
    
    dummy[0] = 0x0505;
    dummy[1] = 0x8888;
    dummy[2] = 0x3333;
    dummy[3] = 0x4444;
    dummy[4] = 0x7777;
    
    handle = dax_tag_add("BitTest", DAX_WORD, 10);
    printf("Handle = %d\n",handle);
    dax_tag_write_bits(handle + 1, &dummy, 77);
    dax_tag_read_bits(handle + 1, &test, 77);
    for(n=0;n<5;n++) {
        printf("Before write 0x%X: After Read 0x%X\n",dummy[n], test[n]);
    }
    
    
    /*
    for(n = 0; n<126; n++) {
        if(n % 5) {
            sprintf(tagname,"BOOL%d",n);
            handle = dax_tag_add(tagname,DAX_BOOL,1);
        } else {
            sprintf(tagname,"BYTE%d",n);
            handle = dax_tag_add(tagname,DAX_BYTE,1);
        }
    }
    
    handle = dax_tag_add("Byte1", DAX_BYTE, 1);
    handle = dax_tag_add("Dint2", DAX_DINT, 10);
    handle = dax_tag_add("Byte3", DAX_BYTE, 1);
    handle = dax_tag_add("Dint4", DAX_DINT, 100);
    handle = dax_tag_add("Bool5", DAX_BOOL, 1);
    handle = dax_tag_add("Dint6", DAX_DINT, 10);
    handle = dax_tag_add("Dint7", DAX_DINT, 100);
    handle = dax_tag_add("Bool8", DAX_BOOL, 5);
    handle = dax_tag_add("Bool9", DAX_BOOL, 1);
    handle = dax_tag_add("Real10", DAX_REAL, 5);
    handle = dax_tag_add("Byte11", DAX_BYTE, 1);
    handle = dax_tag_add("Real12", DAX_REAL, 5);
    handle = dax_tag_add("Real13", DAX_REAL, 5);
    handle = dax_tag_add("Real14", DAX_REAL, 5);
    handle = dax_tag_add("Real15", DAX_REAL, 5);
    handle = dax_tag_add("Real16", DAX_REAL, 5);
    handle = dax_tag_add("Real17", DAX_REAL, 5);
    handle = dax_tag_add("Real18", DAX_REAL, 5);
    handle = dax_tag_add("Real19", DAX_REAL, 38);
    handle = dax_tag_add("Real20", DAX_REAL, 1);
    handle = dax_tag_add("Bool21", DAX_BOOL, 1);
    handle = dax_tag_add("Bool22", DAX_BOOL, 1);
    handle = dax_tag_add("Bool23", DAX_BOOL, 1);
    handle = dax_tag_add("Bool24", DAX_BOOL, 1);
    
    //handle = dax_tag_add("test",DAX_UINT,100);
    //printf("Test received handle 0x%X\n",handle);
    */
     
    /* Verify the integrity of the tag database */
    if( check_tagbase() ) {
        dax_log("Tagbase verification test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tagbase verification test - PASSED");
    }
#endif    
    dax_debug(1, "OpenDAX Test Finished, %d tests failed", tests_failed);
    sleep(5);
    dax_mod_unregister();
    
    return 0;
}
