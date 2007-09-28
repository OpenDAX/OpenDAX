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
    
    openlog("test", LOG_NDELAY, LOG_DAEMON);
    xnotice("Starting module test");
    setverbosity(10);
    dax_mod_register("test");
    
    add_random_tags(500);
    
    
    handle = dax_tag_add("Byte1", DAX_BYTE, 1);
    handle = dax_tag_add("Dint1", DAX_DINT, 10);
    handle = dax_tag_add("Byte2", DAX_BYTE, 1);
    handle = dax_tag_add("Bool1", DAX_BOOL, 1);
    handle = dax_tag_add("Bool2", DAX_BOOL, 5);
    handle = dax_tag_add("Bool3", DAX_BOOL, 1);
    handle = dax_tag_add("Byte3", DAX_BYTE, 1);
    
    //handle = dax_tag_add("test",DAX_UINT,100);
    //printf("Test received handle 0x%X\n",handle);
    
    
    dax_mod_unregister();
    return 0;
}
