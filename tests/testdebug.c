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

// void quit_signal(int sig);
// static void getout(int exitstatus);

dax_state *ds;
// static int _quitsignal;

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    // dax_byte buff[8];
    // dax_byte mask[8];
    Handle src, dest;
    dax_id id;

    ds = dax_init("test");
    dax_init_config(ds, "test");

    dax_configure(ds, argc, argv, CFG_CMDLINE);
    dax_connect(ds);

    dax_tag_add(ds, &src, "DummyIn", DAX_INT, 1);
    dax_tag_add(ds, &dest, "DummyOut", DAX_INT, 1);

    dax_map_add(ds, &src, &dest, &id);

}
