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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libtest_common.h"

int
run_test(int (testfunc(int argc, char **argv)), int argc, char **argv, int opts) {
    int status = 0;
    int result;
    pid_t pid;
    dax_state *ds;

    pid = fork();

    if(pid == 0) { // Child
        execl("../../src/server/tagserver", "../../src/server/tagserver", "-v", NULL);
        printf("Failed to launch tagserver\n");
        exit(-1);
    } else if(pid < 0) {
        exit(-1);
    } else {
         /* Try to connect to the server multiple times until then exit */
        ds = dax_init("test_loader");
        dax_configure(ds, 1, (char **)"dummy", 0);
        for(int n=0;n<10;n++) {
            if(dax_connect(ds) == 0) {
                break;
            }
            usleep(20000);
        }
        dax_disconnect(ds);

        usleep(50000);
        result=testfunc(argc, argv);
        kill(pid, SIGINT);
        if( waitpid(pid, &status, 0) != pid ) {
            if(! opts & NO_UNLINK_RETAIN)
                unlink("retentive.db");
            return status;
        }
    }
    if(! opts & NO_UNLINK_RETAIN)
        unlink("retentive.db");
    return result;
}
