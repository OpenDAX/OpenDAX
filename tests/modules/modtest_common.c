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
 *  This contains common code for the compiled C Library tests
 */

#include "modtest_common.h"

static int tid;

pid_t
run_server(void) {
    int status = 0;
    int result;
    pid_t pid;

    pid = fork();

    if(pid == 0) { // Child
        execl("../../../src/server/tagserver", "../../../src/server/tagserver", NULL);
        printf("Failed to launch tagserver\n");
        exit(-1);
    } else if(pid < 0) {
        printf("Forking problem");
        exit(-1);
    }
    usleep(100000);
    return pid;
}

pid_t
run_module(const char *modpath, const char *modconf) {
    int status = 0;
    int result;
    pid_t pid;

    pid = fork();

    if(pid == 0) { // Child
        execl(modpath, modpath, "-C", modconf, NULL);
        printf("Failed to launch module\n");
        exit(-1);
    } else if(pid < 0) {
        printf("Forking problem");
        exit(-1);
    }
    usleep(100000);
    return pid;
}

int
write_single_coil(int sock, u_int16_t addr, u_int8_t val) {
    u_int8_t buff[1024];
    int result, size;

    size = 2;
    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = (size + 4) >> 8;
    buff[5] = (size + 4);
    buff[6] = 1;
    buff[7] = 5;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = val ? 0xFF : 0x00;
    buff[11] = 0;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 0x05) {
        return buff[8];
    }
    return 0;
}


int
write_multiple_coils(int sock, u_int16_t addr, u_int16_t count, u_int8_t *sbuff) {
    u_int8_t buff[1024];
    int result, size;

    size = (count -1)/8 +1;
    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = (size + 4) >> 8;
    buff[5] = (size + 4);
    buff[6] = 1;
    buff[7] = 15;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    buff[12] = size;
    memcpy(&buff[13], sbuff, size);
    result = send(sock, buff, 13 + size, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 15) {
        return buff[8];
    }
    return 0;
}
