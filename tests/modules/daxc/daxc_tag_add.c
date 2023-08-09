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

/* Tests adding tags of different types and sizes */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "../../modtest_common.h"

int fin, fout, ferr;
dax_state *ds;

char *_badnames[] =  {"_Tag", "1Tag", "-Tag", "Tag-name", "Tag&name", "Tag+name", "tag/name",
                    "tag*name", "TAG?NAME", "TagNameIsWayTooLong12345678912345", NULL};

char *_goodnames[] = {"Tag1", "tAg_name", "t1Ag_name", "TagNameIsBarelyLongEnoughToFit12", NULL};


static int
_add_tag(char *name, char *type, unsigned int count) {
    char s[256];
    if(count != 0) {
        sprintf(s, "add tag %s %s %d\n", name, type, count);
    } else {
        sprintf(s, "add tag %s %s\n", name, type);
    }
    write(fin, s, strlen(s));
    if(expect(fout, "dax>", 100)==NULL) {
        DF("FAIL tagname %s", name);
        return 1;
    }
    return 0;
}

static int
_check_tag(char *name, tag_type type, unsigned int count) {
    int result;
    dax_tag tag;

    result = dax_tag_byname(ds, &tag, name);
    if(result) {
        DF("dax_tag_byname returned %d", result);
        return result;
    }

    if(tag.type == type && tag.count == count) {
        return 0;
    }
    DF("tag doesn't match");
    return 1;
}


static int
_run_test(void) {
    /* This tests that tags with bad names return an error */
    int n=0;
    while(_badnames[n] != NULL) {
        _add_tag(_badnames[n], "int", 1);
        if(expect(ferr, "ERROR:", 100) == NULL) {
            DF("Didn't get error - %s", _badnames[n]);
            return 1;
        }
        n++;
    }

    /* This tests that tags with good names do not return an error */
    n=0;
    while(_goodnames[n] != NULL) {
        _add_tag(_goodnames[n], "int", 1);
        if(expect(ferr, "ERROR:", 100) != NULL) {
            DF("Should not have gotten error - %s", _goodnames[n]);
            return 1;
        }
        n++;
    }

    if(_add_tag("dummy_bool", "bool", 0)) return 1;
    if(_check_tag("dummy_bool", DAX_BOOL, 1)) return 1;

    if(_add_tag("dummy_bool_1", "bool", 1)) return 1;
    if(_check_tag("dummy_bool_1", DAX_BOOL, 1)) return 1;

    if(_add_tag("dummy_bool_100", "bool", 100)) return 1;
    if(_check_tag("dummy_bool_100", DAX_BOOL, 100)) return 1;

    if(_add_tag("dummy_int", "int", 0)) return 1;
    if(_check_tag("dummy_int", DAX_INT, 1)) return 1;

    if(_add_tag("dummy_int_1", "int", 1)) return 1;
    if(_check_tag("dummy_int_1", DAX_INT, 1)) return 1;

    if(_add_tag("dummy_int_10", "int", 10)) return 1;
    if(_check_tag("dummy_int_10", DAX_INT, 10)) return 1;

    if(_add_tag("dummy_real", "real", 0)) return 1;
    if(_check_tag("dummy_real", DAX_REAL, 1)) return 1;

    if(_add_tag("dummy_real_1", "real", 1)) return 1;
    if(_check_tag("dummy_real_1", DAX_REAL, 1)) return 1;

    if(_add_tag("dummy_real_10", "real", 10)) return 1;
    if(_check_tag("dummy_real_10", DAX_REAL, 10)) return 1;

    return 0;
}

static int
_server_connect(int argc, char *argv[]) {
    int result;

    ds = dax_init("test");
    if(ds == NULL) {
        dax_log(LOG_FATAL, "Unable to Allocate DaxState Object\n");
        kill(getpid(), SIGQUIT);
    }
    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    return result;
}

int
main(int argc, char *argv[]) {
    int result;
    int status;
    pid_t mpid, spid;

    spid = run_server();
    mpid = run_module2("../../../src/modules/daxc/daxc", &fin, &fout, &ferr, NULL);

    /* Connect to the server */
    result = _server_connect(argc, argv);
    if(result == 0) {
        if(expect(fout, "dax>", 100)==NULL) {
            DF("daxc didn't start");
            return 1;
        }
        result = _run_test();
        dax_disconnect(ds);
    }


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
    return result;
}
