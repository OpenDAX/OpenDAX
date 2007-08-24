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

#define TEST 1000

int main(int argc,char *argv[]) {
    handle_t handle;
    int n;
    int indata[TEST],outdata[TEST],maskdata[TEST];
    
    openlog("test",LOG_NDELAY,LOG_DAEMON);
    xnotice("starting module test");
    dax_mod_register("test");
    //sleep(1);
    handle=dax_tag_add("modbus",DAX_DINT,TEST);
    xlog(0,"modbus handle = 0x%X",handle);
    //sleep(3);
    
    for(n=0;n<TEST;n++) {
        outdata[n]=n+6000;
		maskdata[n]=0xFFFFFFFE;
    }
    
    //dax_tag_write_bytes(handle,outdata,sizeof(int)*TEST);
    dax_tag_mask_write(handle,outdata,maskdata,sizeof(int)*TEST);
	dax_tag_read_bytes(handle,indata,sizeof(int)*TEST);
    sleep(2);
	for(n=0;n<TEST;n++) {
        printf("outdata[%d] = %d : indata[%d] = %d\n",n,outdata[n],n,indata[n]);
    }
    
    sleep(10);
    dax_mod_unregister();
    sleep(3);
    return 0;
}
