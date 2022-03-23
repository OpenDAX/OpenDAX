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
 *  This test makes sure that we get bad function code exceptions.
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

/* sock: socket on which to send data
 * outbuff: pointer to the bytes that we want to send
 * outlen:  how many bytes to send
 * resbuff: pointer to the
 * reslen: the length of the response for comparison
 */
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
    int s;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int status, n;
    int result = 0;
    pid_t server_pid, mod_pid;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/modbus/daxmodbus", "conf/mb_server_no_inputs.conf");
    /* Connect to the tag server */

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

    /* FC 4 */
    result += _check_response(s, "\x00\x01\x00\x00\x00\x06\x01\x04\x00\x00\x00\x01", 12,
                                 "\x00\x01\x00\x00\x00\x03\x01\x84\x01", 9);

    /* Make sure the others work */
    result += _check_response(s, "\x00\x06\x00\x00\x00\x06\x01\x01\x00\x08\x00\x10", 12,
                                 "\x00\x06\x00\x00\x00\x05\x01\x01\x02\x00\x00", 11);
    result += _check_response(s, "\x00\x07\x00\x00\x00\x06\x01\x02\x00\x00\x00\x10", 12,
                                 "\x00\x07\x00\x00\x00\x05\x01\x02\x02\x00\x00", 11);
    result += _check_response(s, "\x00\x0B\x00\x00\x00\x06\x01\x03\x00\x02\x00\x02", 12,
                                 "\x00\x0B\x00\x00\x00\x07\x01\x03\x04\x00\x00\x00\x00", 13);
//    result += _check_response(s, "\x00\x0C\x00\x00\x00\x06\x01\x04\x00\x02\x00\x04", 12,
//                                 "\x00\x0C\x00\x00\x00\x0B\x01\x04\x08\x00\x00\x00\x00\x00\x00\x00\x00", 17);
    close(s);

    kill(mod_pid, SIGINT);
    kill(server_pid, SIGINT);
    if( waitpid(mod_pid, &status, 0) != mod_pid )
        fprintf(stderr, "Error killing modbus module\n");
    if( waitpid(server_pid, &status, 0) != server_pid )
        fprintf(stderr, "Error killing tag server\n");

    exit(result);
}

/* TODO: Make sure that sending some function other than 0-xFF00 will not affect the register */
