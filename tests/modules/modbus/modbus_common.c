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
 *
 *
 *  This file contains common modbus function */

#define _XOPEN_SOURCE
#include <unistd.h>
#include "modbus_common.h"

static int tid;


int
read_coils(int sock, u_int16_t addr, u_int16_t count, u_int8_t *rbuff) {
    u_int8_t buff[256];
    int result, size;

    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = 0;
    buff[5] = 6;
    buff[6] = 1;
    buff[7] = 1;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 1) {
        return buff[8];
    }
    size = buff[8];
    memcpy(rbuff, &buff[9], size);
    return 0;
}

int
read_discretes(int sock, u_int16_t addr, u_int16_t count, u_int8_t *rbuff) {
    u_int8_t buff[256];
    int result, size;

    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = 0;
    buff[5] = 6;
    buff[6] = 1;
    buff[7] = 2;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 2) {
        return buff[8];
    }
    size = buff[8];
    memcpy(rbuff, &buff[9], size);
    return 0;
}

int
read_holding_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *rbuff) {
    u_int8_t buff[256];
    int result, size;

    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = 0;
    buff[5] = 6;
    buff[6] = 1;
    buff[7] = 3;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 3) {
        return buff[8];
    }
    size = buff[8];
    swab(&buff[9], rbuff, size);
    return 0;
}

int
read_input_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *rbuff) {
    u_int8_t buff[256];
    int result, size;

    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = 0;
    buff[5] = 6;
    buff[6] = 1;
    buff[7] = 4;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 4) {
        return buff[8];
    }
    size = buff[8];
    swab(&buff[9], rbuff, size);
    return 0;
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
    if(buff[7] != 5) {
        return buff[8];
    }
    return 0;
}

int
write_single_register(int sock, u_int16_t addr, u_int16_t val) {
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
    buff[7] = 6;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = val >> 8;
    buff[11] = val;
    result = send(sock, buff, 12, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 6) {
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

int
write_multiple_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *sbuff) {
    u_int8_t buff[1024];
    int result, size;

    size = count * 2;
    buff[0] = tid>>8;
    buff[1] = tid;
    tid++;
    buff[2] = 0;
    buff[3] = 0;
    buff[4] = (size + 4) >> 8;
    buff[5] = (size + 4);
    buff[6] = 1;
    buff[7] = 16;
    buff[8] = addr>>8;
    buff[9] = addr;
    buff[10] = count>>8;
    buff[11] = count;
    buff[12] = size;
    swab(sbuff, &buff[13], size);
    result = send(sock, buff, 13 + size, 0);
    result = recv(sock, buff, 1024, 0);
    if(buff[7] != 16) {
        return buff[8];
    }
    return 0;
}
