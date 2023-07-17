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
#include <time.h>
#include "modtest_common.h"

#define BUFF_LEN 1024

static char _before[BUFF_LEN];

double
_gettime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)(ts.tv_nsec/1e9 + ts.tv_sec);
}


pid_t
run_server(void) {
    pid_t pid;
    dax_state *ds;
    char filename[256];

    pid = fork();
    sprintf(filename, "%s/src/server/tagserver", BUILD_DIR);

    if(pid == 0) { // Child
        execl(filename, filename, "-v", NULL);
        printf("Failed to launch tagserver\n");
        exit(-1);
    } else if(pid < 0) {
        printf("Forking problem");
        exit(-1);
    }
    /* Try to connect to the server multiple times until then exit */
    ds = dax_init("test_loader");
    dax_init_config(ds, "test_loader");
    dax_configure(ds, 1, (char **)"dummy", 0);
    for(int n=0;n<10;n++) {
        if(dax_connect(ds) == 0) {
            break;
        }
        usleep(50000);
    }
    dax_disconnect(ds);
    return pid;
}

pid_t
run_module(const char *modpath, const char *modconf) {
    pid_t pid;

    pid = fork();

    if(pid == 0) { // Child
        execl(modpath, modpath, "-T", "ALL", "-C", modconf, NULL);
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


char *
expect(int fd, char *str, int timeout) {
    struct pollfd fds;
    int result, nexttime;
    char *check;
    char buffer[BUFF_LEN];
    double start = _gettime();
    double diff;
    int index = 0;

    bzero(buffer, BUFF_LEN);
    fds.fd = fd;
    fds.events = POLLIN;
    nexttime = timeout;
    while(1) {
        result = poll(&fds, 1, nexttime);
        if(result == 1) {
            result = read(fd, &buffer[index], 1);
            check = strstr(buffer, str);
            if(check == NULL) {
                diff = (_gettime() - start) * 1000;
                if(diff > timeout) {
                    return NULL;
                } else {
                    nexttime = timeout - diff;
                    index += result;
                    assert(index < BUFF_LEN);
                }
            } else {
                bzero(_before, BUFF_LEN);
                int len = index - strlen(str)+1;
                if(len > 0) {
                    strncpy(_before, buffer, len);
                }
                return check;
            }
        } else {
            return NULL;
        }
    }
    return NULL;
}

char *
before(void) {
    return _before;
}


