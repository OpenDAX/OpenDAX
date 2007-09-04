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
    //handle_t handle;
    int n;
    //u_int16_t indata[TEST],outdata[TEST],maskdata[TEST];
    dax_tag modbus_tag;
    
    openlog("test",LOG_NDELAY,LOG_DAEMON);
    xnotice("Starting module test");
    setverbosity(10);
    dax_mod_register("test");
    
    n = -1;
    while(n < 0) {
        n = dax_tag_get_name("modbus", &modbus_tag);
        xlog(10,"dax_tag_get_name returned %d",n);
        if(n < 0) {
            xlog(10,"OOPS Trying again");
            sleep(1);
        }
    }
    xlog(10,"Tag handle for modbus is 0x%X",modbus_tag.handle);

    while(1) {
    /*    dax_tag_read_bytes(0,&indata,50);
        for(n=0;n<25;n++) {
            printf("data[%d] = %d   ",n,indata[n]);
            if((n % 2) == 1) printf("\n");
        }
        printf("----------\n");
    */
        sleep(5);
    }

    dax_mod_unregister();
    //sleep(3);
    return 0;
}
