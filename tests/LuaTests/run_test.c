/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2022 Phil Birkelbach
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

/* This is a generic test that runs the given Lua script and returns the exit code */

#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "../modtest_common.h"


int
main(int argc, char *argv[]) {
    int lua_status, server_status;
    pid_t server_pid;
    char commandline[256];

    server_pid = run_server();
    if(server_pid < 0) exit(-1);
    assert(argc >= 2);
    snprintf(commandline, 256, "%s %s", "lua ", argv[1]);
    lua_status = system(commandline);

    kill(server_pid, SIGINT);
    if(waitpid(server_pid, &server_status, 0) != server_pid)  {
        printf("Error killing server\n");
    }
    unlink("retentive.db");
    if(lua_status) exit(-1);
    exit(0);
}


