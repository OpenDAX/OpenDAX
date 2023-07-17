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
 *  This test makes sure that we get bad address exceptions.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "modbus_common.h"

/* sock: socket on which to send data
 * outbuff: pointer to the bytes that we want to send
 * outlen:  how many bytes to send
 * resbuff: pointer to the
 * reslen: the length of the response for comparison
 */
int
_check_response(int sock, char *outbuff, int outlen, char *resbuff, int reslen) {
    uint8_t buff[1024];
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
        if(((uint8_t *)resbuff)[n] != buff[n]) {
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
    mod_pid = run_module("../../../src/modules/modbus/daxmodbus", "conf/mb_server.conf");
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

    /* FC 1 */
    /* Test an address that is to big */
    result += _check_response(s, "\x00\x01\x00\x00\x00\x06\x01\x01\x00\x40\x00\x01", 12,
                                 "\x00\x01\x00\x00\x00\x03\x01\x81\x02", 9);
    /* Test a good address but too many bits */
    result += _check_response(s, "\x00\x02\x00\x00\x00\x06\x01\x01\x00\x3F\x00\x02", 12,
                                 "\x00\x02\x00\x00\x00\x03\x01\x81\x02", 9);
    /* FC 2 */
    /* Test an address that is to big */
    result += _check_response(s, "\x00\x03\x00\x00\x00\x06\x01\x02\x00\x40\x00\x01", 12,
                                 "\x00\x03\x00\x00\x00\x03\x01\x82\x02", 9);
    /* Test a good address but too many bits */
    result += _check_response(s, "\x00\x04\x00\x00\x00\x06\x01\x02\x00\x3F\x00\x02", 12,
                                 "\x00\x04\x00\x00\x00\x03\x01\x82\x02", 9);
    /* FC 3 */
    result += _check_response(s, "\x00\x05\x00\x00\x00\x06\x01\x03\x00\x20\x00\x01", 12,
                                 "\x00\x05\x00\x00\x00\x03\x01\x83\x02", 9);
    result += _check_response(s, "\x00\x05\x00\x00\x00\x06\x01\x03\x00\x1F\x00\x02", 12,
                                 "\x00\x05\x00\x00\x00\x03\x01\x83\x02", 9);
    /* FC 4 */
    result += _check_response(s, "\x00\x06\x00\x00\x00\x06\x01\x04\x00\x20\x00\x01", 12,
                                 "\x00\x06\x00\x00\x00\x03\x01\x84\x02", 9);
    result += _check_response(s, "\x00\x06\x00\x00\x00\x06\x01\x04\x00\x1F\x00\x02", 12,
                                 "\x00\x06\x00\x00\x00\x03\x01\x84\x02", 9);
    /* FC 5 */
    result += _check_response(s, "\x00\x07\x00\x00\x00\x06\x01\x05\x00\x40\x00\x00", 12,
                                 "\x00\x07\x00\x00\x00\x03\x01\x85\x02", 9);
    /* FC 6 */
    result += _check_response(s, "\x00\x08\x00\x00\x00\x06\x01\x06\x00\x20\x00\x00", 12,
                                 "\x00\x08\x00\x00\x00\x03\x01\x86\x02", 9);
    /* FC 15 */
    result += _check_response(s, "\x00\x09\x00\x00\x00\x07\x01\x0F\x00\x40\x00\x01\x01", 13,
                                 "\x00\x09\x00\x00\x00\x03\x01\x8F\x02", 9);
    result += _check_response(s, "\x00\x0A\x00\x00\x00\x07\x01\x0F\x00\x3F\x00\x02\x01", 13,
                                 "\x00\x0A\x00\x00\x00\x03\x01\x8F\x02", 9);
    /* FC 16 */
    result += _check_response(s, "\x00\x0B\x00\x00\x00\x08\x01\x10\x00\x20\x00\x01\x06\x11", 14,
                                 "\x00\x0B\x00\x00\x00\x03\x01\x90\x02", 9);
    /* FC 16 */
    result += _check_response(s, "\x00\x0C\x00\x00\x00\x0A\x01\x10\x00\x1F\x00\x02\x06\x11\x01\x02", 16,
                                 "\x00\x0C\x00\x00\x00\x03\x01\x90\x02", 9);


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
