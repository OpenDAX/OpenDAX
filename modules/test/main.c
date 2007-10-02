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
    handle_t handle;
    int n,toggle;
    u_int16_t indata[TEST],outdata[TEST],maskdata[TEST];
    dax_tag modbus_tag;
    char tagname[30];
    
    openlog("test", LOG_NDELAY, LOG_DAEMON);
    xnotice("Starting module test");
    setverbosity(1);
    dax_mod_register("test");
    
    //add_random_tags(72700);
    
    /* These tags should fail */
    if(tagtofail("1Tag")) return -1;
    if(tagtofail("-Tag")) return -1;
    if(tagtofail("Tag-name")) return -1;
    if(tagtofail("Tag&name")) return -1;
    /* These tags should pass */
    if(tagtopass("_Tag")) return -1;
    if(tagtopass("Tag1")) return -1;
    if(tagtopass("tAg_name")) return -1;
    if(tagtopass("t1Ag_name")) return -1;
    
    
    
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
    */
    //handle = dax_tag_add("test",DAX_UINT,100);
    //printf("Test received handle 0x%X\n",handle);
    if( checktagbase() ) {
        xlog(1,"Tagbase failed test");
    } else {
        xlog(1,"Tagbase passed test");
    }
    
    dax_mod_unregister();
    return 0;
}
