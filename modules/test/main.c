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
#include <syslog.h>
#include <opendax.h>
#include <dax/message.h>
#include <dax/func.h>
#include <string.h>
#include <tags.h>

#define TEST 1000

int main(int argc,char *argv[]) {
    int tests_failed = 0;
    int n;
    handle_t handle;
    char tagname[DAX_TAGNAME_SIZE +1];
    u_int16_t dummy[20], test[20];
    
    openlog("test", LOG_NDELAY, LOG_DAEMON);
    xnotice("Starting module test");
    setverbosity(1);
    dax_mod_register("test");
    
    if(check_tag_addition()) {
        xnotice("Tagname addition test - FAILED");
        tests_failed++;
    } else {
        xnotice("Tagname addition test - PASSED");
    }
    
    if(check_tag_retrieve()) {
        xnotice("Tagname retrieving test - FAILED");
        tests_failed++;
    } else {
        xnotice("Tagname retrieving test - PASSED");
    }
    
    /* TEST TO DO
        Tag Read / Write Test
    */
    
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
    add_random_tags(1000);
    
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
        xnotice("Tagbase verification test - FAILED");
        tests_failed++;
    } else {
        xnotice("Tagbase verification test - PASSED");
    }
    
    dax_mod_unregister();
    
    xlog(1,"OpenDAX Test Finished, %d tests failed", tests_failed);
    return 0;
}
