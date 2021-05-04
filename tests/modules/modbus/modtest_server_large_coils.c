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
    u_int16_t  count;
    u_int8_t bytes;
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
    frame->bytes = buff[8];
    memcpy(frame->buff, &buff[9], frame->bytes);
    return result;
}

int
_test_read(int sock, u_int16_t addr, u_int16_t count, u_int8_t *cmp_buff) {
    static u_int16_t tid;
    int result;
    struct mod_frame sframe, rframe;
    u_int8_t buff[2];
    u_int8_t rbuff[256];

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
_check_response(int sock, u_int8_t *outbuff, int outlen, u_int8_t *resbuff, int reslen) {
    u_int8_t buff[1024];
    int sresult, rresult;
    int n, csize;

    sresult = send(sock, outbuff, outlen, 0);
    bzero(buff, 1024);
    rresult = recv(sock, buff, 1024, 0);
    csize = (buff[4] * 256 + buff[5]) + 6; /* how big should the result be */
    if(csize != rresult) {
        return 1;
    }
    for(n=0;n<reslen;n++) {
        if(resbuff[n] != buff[n]) {
            return 1;
        }
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    int s, exit_status = 0;
    dax_state *ds;
    tag_handle h;
    u_int8_t buff[1024], rbuff[1024];
    struct mod_frame sframe, rframe;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n, i;
    int result;
    pid_t server_pid, mod_pid;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/modbus", "conf/mb_server_large.conf");
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
    /* Test Reading in a couple of places */
    bzero(buff, h.size);
    buff[0]=0xA5;
    result =  dax_tag_handle(ds, &h, "mb_creg[3000]", 8);
    if(result) return result;
    dax_write_tag(ds, h, buff); /* Write it to OpenDAX */
    exit_status += _test_read(s, 3000, 8, buff); /* Check it */

    buff[0]=0x01; buff[1]=0x23; buff[2]=0x45; buff[3]=0x67;
    buff[4]=0x89; buff[5]=0xAB; buff[6]=0xCD; buff[7]=0xEF;
    result =  dax_tag_handle(ds, &h, "mb_creg[3936]", 64);
    if(result) return result;
    dax_write_tag(ds, h, buff); /* Write it to OpenDAX */
    exit_status += _test_read(s, 3936, 64, buff); /* Check it */

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

    exit(exit_status);
}

/* TODO: Make sure that sending some function other than 0-xFF00 will not affect the register */
