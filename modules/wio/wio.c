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
 *  Main source code file for the OpenDAX wireless I/O module.
 */

#include <wio.h>


int
main(int argc, char *argv[])
{
    int fd;
    char str[128];
    
    if(argc < 2) {
        printf("ERROR: Need to pass the device as a parameter\n");
        return 1;
    }
    fd = xbee_open_port(argv[1], B9600);
    if(fd <= 0) {
        printf("ERROR: Unable to open port - %s\n", argv[1]);
        return 2;
    }
    sleep(1);
    strcpy(str, "+++");
    write(fd, str, 3);
    sleep(2);
    read(fd, str, 3);
    printf("%s\n", str);
    
    strcpy(str, "ATIO1\r");
    write(fd, str, 6);
    sleep(1);
    read(fd, str, 2);
    printf("%s\n", str);
    
    strcpy(str, "ATIO0\r");
    write(fd, str, 6);
    read(fd, str, 2);
    printf("%s\n", str);
    strcpy(str, "ATCN\r");
    write(fd, str, 5);
    read(fd, str, 2);
    printf("%s\n", str);
    
    return 0;
}
