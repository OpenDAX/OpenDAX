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
 *  This contains common code for the compiled C module tests
 */

#include <string.h>
#include <poll.h>
#include "modtest_common.h"

#define BUFF_LEN 512

pid_t
run_server(void) {
    pid_t pid;

    pid = fork();

    if(pid == 0) { // Child
        execl("../../../src/server/tagserver", "../../../src/server/tagserver", "-v", NULL);
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


/* Runs the module and redirects the standard i/o streams and return those in the given pointers */
pid_t
run_module2(const char *modpath, int *fd_stdin, int *fd_stdout, int *fd_stderr, const char *modconf) {
    pid_t pid;

    int infd[2];
    int outfd[2];
    int errfd[2];
    pipe(infd);
    pipe(outfd);
    pipe(errfd);

    pid = fork();
    if(pid == 0) {
        dup2(infd[0], STDIN_FILENO);
        dup2(outfd[1], STDOUT_FILENO);
        dup2(errfd[1], STDERR_FILENO);
        close(infd[1]);
        close(outfd[0]);
        close(errfd[0]);
        if(modconf != NULL) {
            execl(modpath, modpath, "-C", modconf, NULL);
        } else {
            execl(modpath, modpath, NULL);
        }
        printf("Failed to launch module\n");
        exit(-1);
    } else if(pid < 0) {
        printf("Forking problem");
        exit(-1);
    }
    if(fd_stdin != NULL) *fd_stdin = infd[1];
    close(infd[0]);
    if(fd_stdout != NULL) *fd_stdout = outfd[0];
    close(outfd[1]);
    if(fd_stderr != NULL) *fd_stderr = errfd[0];
    close(errfd[1]);
    usleep(100000);

    return pid;
}


int
expect(int fd, char *str, int timeout) {
    struct pollfd fds;
    int result;
    char buffer[BUFF_LEN];

    bzero(buffer, BUFF_LEN);
    fds.fd = fd;
    fds.events = POLLIN;

    result = poll(&fds, 1, timeout);
    if(result == 1) {
        result = read(fd, buffer, BUFF_LEN);
        return strncmp(buffer, str, strlen(str));
    } else {
        return -1;
    }
    return -1;
}
