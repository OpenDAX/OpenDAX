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
 *  Test function code 6 (read holding registers) for modbus server
 */
/* TODO: Convert this to use the functions in modbus_common.c */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../modtest_common.h"

struct mod_frame {
    u_int16_t tid;
    u_int8_t  uid;
    u_int8_t  fc;
    u_int16_t  addr;
    u_int8_t *buff;
    u_int16_t size; /* Size of buffer */
};

int
_send_frame(int sock, struct mod_frame frame) {
    u_int8_t buff[2048];
    int result;

    buff[0] = frame.tid>>8;
    buff[1] = frame.tid;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = (frame.size + 4) >> 8;
    buff[5] = (frame.size + 4);
    buff[6] = frame.uid;
    buff[7] = frame.fc;
    buff[8] = frame.addr>>8;
    buff[9] = frame.addr;
    memcpy(&buff[10], frame.buff, frame.size);
    result = send(sock, buff, 12, 0);

    return result;
}

int
_recv_frame(int sock, struct mod_frame *frame) {
    u_int8_t buff[2048];
    int result;

    result = recv(sock, buff, 1024, 0);
    frame->tid = (u_int16_t)buff[0]<<8 | buff[1];
    frame->size = ((u_int16_t)buff[4]<<8 | buff[5])-4;
    frame->uid = buff[6];
    frame->fc = buff[7];
    frame->addr = (u_int16_t)buff[8]<<8 | buff[9];
    memcpy(frame->buff, &buff[10], frame->size);
    return result;
}

int
main(int argc, char *argv[])
{
    int s, exit_status = 0;
    dax_state *ds;
    tag_handle h;
    dax_uint test;
    u_int8_t buff[1024], rbuff[8], bit;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n, i;
    int result;
    pid_t server_pid, mod_pid;
    struct mod_frame sframe, rframe;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/modbus", "conf/mb_server.conf");
    /* Connect to the tag server */
    ds = dax_init("test");
    if(ds == NULL) {
        dax_fatal(ds, "Unable to Allocate DaxState Object\n");
    }
    dax_init_config(ds, "test");
    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) return result;
    result =  dax_tag_handle(ds, &h, "mb_hreg", 0);
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
    /* Loop through all the registers setting them */
    for(n=0; n<h.count; n++) {
        printf("Testing Register %d of %d\n", n, h.count);
        /* Set it every register in OpenDAX to zero */
        bzero(buff, h.size);
        result = dax_write_tag(ds, h, buff);
        if(result) fprintf(stderr, "Unable to write tag\n");
        /* Set the next bit */
        buff[0] = n/256;
        buff[1] = n%256;
        sframe.tid = n;
        sframe.uid = 1;
        sframe.fc = 6;
        sframe.addr = n;
        sframe.size = 2;
        sframe.buff = buff;
        result = _send_frame(s, sframe);
        assert(result == 12);
        rframe.buff = rbuff;
        result = _recv_frame(s, &rframe);
        assert(result == 12);
        /* Read them back and verify that they are correct */
        result = dax_read_tag(ds, h, buff);
        if(result) fprintf(stderr, "Unable to read tag\n");
        for(i=0; i<h.size; i+=2) {
            test = buff[i+1] * 256 + buff[i];
            if(i/2==n) {
                if(test != n) exit_status++;
            } else {
                if(test != 0x0000) exit_status++;
            }
        }
    }


    close(s);
    dax_disconnect(ds);

    kill(mod_pid, SIGINT);
    kill(server_pid, SIGINT);
    if( waitpid(mod_pid, &status, 0) != mod_pid )
        fprintf(stderr, "Error killing modbus module\n");
    if( waitpid(server_pid, &status, 0) != server_pid )
        fprintf(stderr, "Error killing tag server\n");

    exit(exit_status);
}

/* TODO: Make sure that sending some function other than 0-xFF00 will not affect the register */
