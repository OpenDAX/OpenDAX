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

int main(int argc,char *argv[]) {
    handle_t handle;
    int n;
    u_int8_t buffer[3000];
    
    openlog("test",LOG_NDELAY,LOG_DAEMON);
    xnotice("starting module test");
    dax_mod_register("test");
    sleep(1);
    handle=dax_tag_add("modbus",DAX_BOOL,200);
    xlog(0,"modbus handle = %d",handle);
    sleep(3);
    
    for(n=0;n<3000;n++) {
        buffer[n]=n+6000;
    }
    
    dax_tag_write_bytes(0,buffer,2050);
    dax_tag_read_bytes(0,buffer,2050);
    
    sleep(10);
    dax_mod_unregister();
    sleep(3);
    return 0;
}
