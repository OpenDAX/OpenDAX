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

#include "modbus.h"
#include <sys/stat.h>
#include <sys/select.h>

extern dax_state *ds;

static int
_mb_read(mb_port *port, int fd)
{
    int result;
    unsigned char buff[MB_BUFF_SIZE];
    int buffindex;
    uint16_t checksum;
    struct stat staterr;
    fd_set readfs, errfs;
    struct timeval timeout;
    int state=0; /* 0 = waiting */

    buffindex = 0;

    /* Read a frame from the serial port */
    while(1) {
        if(state == 0) { /* Waiting */
            timeout.tv_usec = (port->timeout % 1000) * 1000;
            timeout.tv_sec = port->timeout / 1000;
            FD_ZERO(&readfs);
            FD_SET(fd, &readfs);
            FD_ZERO(&errfs);
            FD_SET(fd, &errfs);
            result = select(fd+1, &readfs, NULL, &errfs, &timeout);
            if(FD_ISSET(fd, &readfs)) {
                state = 1; /* We have data to read */
            }
        } else if(state == 1) { /* reading frame */
            result = read(fd, &buff[buffindex], MB_BUFF_SIZE-buffindex);
            if(result < 0) {
                dax_log(LOG_ERROR, "Error reading Serial port on fd = %d", fd);
                return MB_ERR_RECV_FAIL;
            } else if(result > 0) {
                buffindex += result;
                if(buffindex > MB_BUFF_SIZE) {
                    return MB_ERR_OVERFLOW;
                }
            } else {
                result = stat(port->device, &staterr);
                if(result) {
                    close(fd);
                    port->fd = 0;
                    return MB_ERR_PORTFAIL;
                }
                break; /* One way or another we are at the end of our frame */
            }
            // TODO: Need to calculate this based on serial parameters.
            usleep(port->frame); /* Inter byte timeout */
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
                dax_log(LOG_ERROR, "Error reading serial port data %d\n", result);
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
            dax_log(LOG_ERROR, "Exiting slave loop because of error %d\n",  result);
            return result;
        }
    }
}
