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
 *  Test function code 1 for the modbus server module
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
#include "modbus_common.h"

struct mod_frame {
    uint16_t tid;
    uint8_t  uid;
    uint8_t  fc;
    uint16_t  addr;
    uint8_t bytes;
    uint8_t *buff;
    uint16_t size; /* Size of buffer */
};

int
_send_frame(int sock, struct mod_frame frame) {
    uint8_t buff[2048];
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
    uint8_t buff[2048];
    int result;

    result = recv(sock, buff, 1024, 0);
    frame->tid = (uint16_t)buff[0]<<8 | buff[1];
    frame->size = ((uint16_t)buff[4]<<8 | buff[5])-4;
    frame->uid = buff[6];
    frame->fc = buff[7];
    frame->bytes = buff[8];
    memcpy(frame->buff, &buff[9], frame->bytes);
    return result;
}

int
_test_request(int sock, uint16_t addr, uint16_t count, uint8_t *cmp_buff) {
    static uint16_t tid;
    int result;
    struct mod_frame sframe, rframe;
    uint8_t buff[2];
    uint8_t rbuff[256];

    buff[0] = count >> 8;
    buff[1] = count % 256;
    sframe.tid = tid++;
    sframe.uid = 1;
    sframe.fc = 1;
    sframe.addr = addr;
    sframe.size = 2;
    sframe.buff = buff;
    result = _send_frame(sock, sframe);
    rframe.buff = rbuff;
    result = _recv_frame(sock, &rframe);
    printf("Checking %d bytes\n",(count-1)/8 + 1);
    for(int i=0; i<((count-1)/8 + 1); i++) {
        printf("rbuff[%d] = 0x%X, cmp_buff[%d] = 0x%X\n", i, rbuff[i], i, cmp_buff[i]);
        if( rbuff[i] != cmp_buff[i] ) return 1;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    int s, exit_status = 0;
    dax_state *ds;
    tag_handle h;
    uint8_t buff[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n, i;
    int result;
    pid_t server_pid, mod_pid;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/daxmodbus", "conf/mb_server.conf");
    /* Connect to the tag server */
    ds = dax_init("test");
    if(ds == NULL) {
        dax_fatal(ds, "Unable to Allocate DaxState Object\n");
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
    bzero(buff, h.size); /* Zero out the buffer */
    buff[0]=0x01;        /* set the first bit to 1*/
    dax_write_tag(ds, h, buff); /* Write it to OpenDAX */
    exit_status += _test_request(s, 0, 1, buff); /* Check it */
    /* chekc a whole byte */
    buff[0]=0xAA; buff[1]=0x55;
    dax_write_tag(ds, h, buff);
    exit_status += _test_request(s, 0, 8, buff);
    /* Check a byte and a half */
    buff[1] = 0x05;
    exit_status += _test_request(s, 0, 12, buff);
    /* Check a byte and a half starting with the second byte */
    buff[0] = 0x12; buff[1] = 0x34; buff[2] = 0x56;
    dax_write_tag(ds, h, buff);
    buff[2] = 0x6;
    exit_status += _test_request(s, 8, 12, &buff[1]);

    buff[0] = 0x12; buff[1] = 0x34; buff[2] = 0x56;
    dax_write_tag(ds, h, buff);
    /* basically shifting four bits right, and removing four */
    buff[0] = 0x41; buff[1] = 0x03;
    exit_status += _test_request(s, 4, 12, buff);
    /* read the whole thing */
    for(i=0;i<h.size;i++) {
        buff[i] = i;
    }
    dax_write_tag(ds, h, buff);
    exit_status += _test_request(s, 0, h.count, buff);

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
