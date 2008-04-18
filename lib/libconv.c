/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
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
 
 * This file contains libdax data conversion functions.  These functions handle
 * the required conversions of data to be sent and received from the server.
 */

#include <libdax.h>
#include <dax/libcommon.h>

/* TODO: All of these functions need to be written.  Right now we just
   assume that all is good and that we don't need any of this. */
/* TODO: Might also be nice to have some that will handle a whole buffer
   of data instead of just a single point */

/* 16 Bit Conversion Functions */
int16_t mtos_int(int16_t x) {
    return x;
}

u_int16_t mtos_uint(u_int16_t x) {
    return x;
}

int16_t stom_int(int16_t x) {
    return x;
}

u_int16_t stom_uint(u_int16_t x) {
    return x;
}

/* 32 Bit conversion Functions */
int32_t mtos_dint(int32_t x) {
    return x;
}

u_int32_t mtos_udint(u_int32_t x) {
    return x;
}

float mtos_real(float x) {
    return x;
}

int32_t stom_dint(int32_t x) {
    return x;
}

u_int32_t stom_udint(u_int32_t x) {
    return x;
}

float stom_real(float x) {
    return x;
}
