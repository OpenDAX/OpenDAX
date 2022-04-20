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
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "../modtest_common.h"

int main() {
    int result;
    int status;
    pid_t mpid, spid;
    int fin, fout, ferr;

    spid = run_server();
    mpid = run_module2("../../../src/modules/daxc/daxc", &fin, &fout, &ferr, NULL);
    assert(expect(fout, "dax>", 100)==0);
    write(fin, "exit\n", 5);
    if( waitpid(mpid, &status, 0) != mpid ) {
        fprintf(stderr, "Error exiting daxc module\n");
        return -1;
    }

    kill(spid, SIGINT);
    if( waitpid(spid, &status, 0) != spid ) {
        fprintf(stderr, "Error killing tag server\n");
        return -1;
    }
    return 0;
}
