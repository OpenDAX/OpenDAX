/* mbserver.c - Modbus (tm) Communications Library
 * Copyright (C) 2022 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Source file for TCP Server functionality
 */

#include <modbus.h>

extern dax_state *ds;

static int
_mb_read(mb_port *port, int fd)
{
    int result;
    unsigned char buff[MB_BUFF_SIZE];
    int buffindex;
    uint16_t checksum;

    buffindex = 0;

    /* Read a frame from the serial port */
    while((result = read(fd, &buff[buffindex], MB_BUFF_SIZE-buffindex))) {
        if(result < 0) {
            dax_error(ds, "Error reading Serial port on fd = %d", fd);
            return MB_ERR_RECV_FAIL;
        }
        if(result>0) {
            buffindex += result;
        }
    }
    if(buffindex > 0) {
        if(port->in_callback) {
            port->in_callback(port, buff, buffindex);
        }

        /* Check the checksum here. */
        result = crc16check(buff, buffindex);
        if(result) {
            result = create_response(port, buff, MB_BUFF_SIZE);
            if(result > 0) { /* We have a response */
                checksum = crc16(buff, result);
                COPYWORD(&(buff[result]), &checksum);
                if(port->out_callback) {
                    port->out_callback(port, buff, result+2);
                }
                write(fd, buff, result+2);
                buffindex = 0;
            } else if(result < 0) {
                dax_error(ds, "Error reading serial port data %d\n", result);
                return result;
            }
        }
    }
    return 0;
}


int
slave_loop(mb_port *port) {
    int result;

   while(1) {
        result = _mb_read(port, port->fd);
        if(result) {
            dax_error(ds, " Exiting slave loop because of error %d\n",  result);
            return result;
        }
    }
}
