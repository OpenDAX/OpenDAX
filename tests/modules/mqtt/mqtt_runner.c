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

#include <common.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../../modtest_common.h"

static pid_t _server_pid;
static pid_t _mod_pid;


int
main(int argc, char *argv[]) {
    int s;
    uint8_t buff[1024];
    char conf_str[64];

    int status;
    int result;

    /* Run the tag server and the module */
    _server_pid = run_server();

    sprintf(conf_str, "conf/%s.conf", TEST_NAME);
    _mod_pid = run_module(TEST_NAME, conf_str);
    DF("Running MQTT Test for %s", TEST_NAME);
    /* We just wait on the module to die and then kill the server */
    if( waitpid(_mod_pid, &result, 0) != _mod_pid )
        fprintf(stderr, "Error killing test module\n");
    kill(_server_pid, SIGINT);
    if( waitpid(_server_pid, &status, 0) != _server_pid )
        fprintf(stderr, "Error killing tag server\n");
    DF("Exiting with %d", WEXITSTATUS(result));
    exit(WEXITSTATUS(result));
}