/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2019 Phil Birkelbach
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
 *  We use this file for debugging tests.  Most of our tests use Python
 *  ctypes and those can be hard to debug.  This file can be more easily
 *  run with gdb.
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

dax_state *ds;
static int _quitsignal;

int
do_test(int argc, char *argv[])
{
    tag_handle tag;
    int result;
    char buff[32];
    unsigned long x = 18446744073709551615UL;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        return -1;
    } else {
        result = dax_tag_add(ds, &tag, "Dummy", DAX_INT, 1);
        assert(result == 0);
        x = 0x55555555;
        dax_write_tag(ds,  tag, &x);
        dax_tag_del(ds, tag.index);
        result = dax_read_tag(ds, tag, buff);
        printf("result = %d\n", result);
    }
    return 0;

}

/* main inits and then calls run */
int
main(int argc, char *argv[])
{
    int status = 0;
    pid_t pid;

    pid = fork();
    if(pid == 0) { // Child
        execl("../src/server/tagserver", "../src/server/tagserver", NULL);
        printf("Failed to launch tagserver\n");
        exit(0);
    } else if(pid < 0) {
        exit(-1);
    } else {
        usleep(100000);
        if (do_test(argc, argv)) status ++;
        kill(pid, SIGINT);
        if (waitpid (pid, &status, 0) != pid)
            status ++;
    }
    return status;
}
