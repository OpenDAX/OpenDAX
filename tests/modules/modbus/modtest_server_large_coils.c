/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2021 Phil Birkelbach
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
 */

/*
 *  Test for fairly large modbus register sets
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "modbus_common.h"


int
main(int argc, char *argv[])
{
    int s, exit_status = 0;
    dax_state *ds;
    tag_handle h;
    uint8_t buff[1024], rbuff[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n, i;
    int result;
    pid_t server_pid, mod_pid;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/daxmodbus", "conf/mb_server_large.conf");
    /* Connect to the tag server */
    ds = dax_init("test");
    if(ds == NULL) {
        dax_log(DAX_LOG_FATAL, "Unable to Allocate DaxState Object\n");
        kill(getpid(), SIGQUIT);
    }
    dax_init_config(ds, "test");
    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) return result;
    result =  dax_tag_handle(ds, &h, "mb_creg", 0);
    if(result) return result;

    /* Open a socket to do the modbus stuff */
    s = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5502);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    /*---- Connect the socket to the server using the address struct ----*/
    addr_size = sizeof serverAddr;
    result = connect(s, (struct sockaddr *) &serverAddr, addr_size);
    if(result) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(result);
    }
    /* Test Reading in a couple of places */
    bzero(buff, h.size);
    buff[0]=0xA5;
    result =  dax_tag_handle(ds, &h, "mb_creg[3000]", 8);
    if(result) return result;
    dax_write_tag(ds, h, buff); /* Write it to OpenDAX */
    exit_status += read_coils(s, 3000, 8, rbuff);
    if(rbuff[0] != 0xA5) exit_status++;

    buff[0]=0x01; buff[1]=0x23; buff[2]=0x45; buff[3]=0x67;
    buff[4]=0x89; buff[5]=0xAB; buff[6]=0xCD; buff[7]=0xEF;
    result =  dax_tag_handle(ds, &h, "mb_creg[3936]", 64);
    if(result) return result;
    dax_write_tag(ds, h, buff); /* Write it to OpenDAX */
    exit_status += read_coils(s, 3936, 64, rbuff);
    for(i=0;i<8;i++) if(buff[i] != rbuff[i]) exit_status++;

    buff[0]=0x89; buff[1]=0xAB; buff[2]=0xCD; buff[3]=0xEF;
    buff[4]=0x01; buff[5]=0x23; buff[6]=0x45; buff[7]=0xF6;
    exit_status += write_multiple_coils(s, 3936, 64, buff);
    dax_read_tag(ds, h, rbuff);
    for(i=0;i<8;i++) if(buff[i] != rbuff[i]) exit_status++;

    /* Test clearing that last bit with FC 5 */
    exit_status += write_single_coil(s, 3999, 0);
    dax_read_tag(ds, h, rbuff);
    if(rbuff[7] != 0x76) exit_status++;

    close(s);
    dax_disconnect(ds);

    kill(mod_pid, SIGINT);
    kill(server_pid, SIGINT);
    if( waitpid(mod_pid, &status, 0) != mod_pid )
        fprintf(stderr, "Error killing modbus module\n");
    if( waitpid(server_pid, &status, 0) != server_pid )
        fprintf(stderr, "Error killing tag server\n");
    if(exit_status == 0)
        fprintf(stderr, "TEST PASSED\n");
    else
        fprintf(stderr, "***TEST FAILED***\n");
    exit(exit_status);
}

/* TODO: Make sure that sending some function other than 0-xFF00 will not affect the register */
