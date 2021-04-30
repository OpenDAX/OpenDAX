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
