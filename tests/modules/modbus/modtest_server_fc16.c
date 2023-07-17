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
 *  Test function code 15 for the modbus server module
 */
/* TODO: Convert this to use the functions in modbus_common.c */

#define _XOPEN_SOURCE
#include <unistd.h>
#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "modbus_common.h"

struct mod_frame {
    uint16_t tid;
    uint8_t  uid;
    uint8_t  fc;
    uint16_t addr;
    uint16_t count;
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
    buff[10] = frame.count>>8;
    buff[11] = frame.count;
    buff[12] = frame.size;
    swab(frame.buff, &buff[13], frame.size);
    //memcpy(&buff[13], frame.buff, frame.size);
    result = send(sock, buff, 13+frame.size, 0);

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
    frame->addr = (uint16_t)buff[8]<<8 | buff[9];
    memcpy(frame->buff, &buff[10], frame->size);
    return result;
}

int
_send_request(int sock, uint16_t addr, uint16_t count, uint16_t *snd_buff) {
    static uint16_t tid;
    int result;
    struct mod_frame sframe, rframe;
    uint8_t buff[2];
    uint8_t rbuff[256];

    buff[0] = count >> 8;
    buff[1] = count % 256;
    sframe.tid = tid++;
    sframe.uid = 1;
    sframe.fc = 16;
    sframe.addr = addr;
    sframe.count = count;
    sframe.size = count * 2;
    sframe.buff = (uint8_t *)snd_buff;
    result = _send_frame(sock, sframe);
    rframe.buff = rbuff;
    result = _recv_frame(sock, &rframe);
    if(rframe.fc != sframe.fc) return -1; /* Check for exception */
    return 0;
}

int
main(int argc, char *argv[])
{
    int s, exit_status = 0;
    dax_state *ds;
    tag_handle h;
    uint16_t buff[1024], rbuff[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n, i;
    int result;
    pid_t server_pid, mod_pid;
    struct mod_frame sframe, rframe;

    /* Run the tag server andn the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/daxmodbus", "conf/mb_server.conf");
    /* Connect to the tag server */
    ds = dax_init("test");
    if(ds == NULL) {
        dax_log(LOG_FATAL, "Unable to Allocate DaxState Object\n");
        kill(getpid(), SIGQUIT);
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

    buff[0] = 0x1234; buff[1] = 0x5678;
    exit_status += _send_request(s, 0, 2, buff);
    dax_read_tag(ds, h, rbuff);
    for(i=0;i<2;i++) if(buff[i] != rbuff[i]) exit_status++;

    for(i=0;i<h.count;i++) {
        buff[i] = i;
    }
    exit_status += _send_request(s, 0, h.count, buff);
    dax_read_tag(ds, h, rbuff);
    for(i=0;i<h.count;i++) if(buff[i] != rbuff[i]) exit_status++;

    buff[0] = 0x1234; buff[1] = 0x5678;
    exit_status += _send_request(s, 16, 2, buff);
    dax_read_tag(ds, h, rbuff);
    for(i=0;i<2;i++) if(buff[i] != rbuff[i+16]) exit_status++;

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
