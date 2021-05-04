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
#include <common.h>
#include <opendax.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int read_coils(int sock, u_int16_t addr, u_int16_t count, u_int8_t *rbuff);
int read_discretes(int sock, u_int16_t addr, u_int16_t count, u_int8_t *rbuff);
int read_holding_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *rbuff);
int read_input_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *rbuff);
int write_single_coil(int sock, u_int16_t addr, u_int8_t val);
int write_single_register(int sock, u_int16_t addr, u_int16_t val);
int write_multiple_coils(int sock, u_int16_t addr, u_int16_t count, u_int8_t *sbuff);
int write_multiple_registers(int sock, u_int16_t addr, u_int16_t count, u_int16_t *sbuff);

