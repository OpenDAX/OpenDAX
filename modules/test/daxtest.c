/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2007 Phil Birkelbach
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
 *
 *  Main source code file for the OpenDAX API regression test module
 */

#include <daxtest.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/* These are to keep score */
static int _tests_run = 0;
static int _tests_failed = 0;

void
test_start(void) {
    _tests_run++;
}

void
test_fail(void) {
    _tests_failed++;
}

int
tests_run(void)
{
    return _tests_run;
}

int
tests_failed(void)
{
    return _tests_failed;
}


/* TODO: Tests that need to be added....
 * Check that duplicate tags fail
 */
int static
tag_read_write_test(void)
{
    tag_index index;
    Handle handle;
    tag_type type;
    dax_int write_data[32], read_data[32];
    index = dax_tag_add(&handle, "TestReadTagInt", DAX_INT, 32);
    
    dax_tag_handle(&handle, "TestReadTagInt", 32);
    
    write_data[0] = 0x5555;
    dax_write_tag(handle, write_data);
    dax_read_tag(handle, read_data);
    
    printf("read_data[0] = 0x%X\n", read_data[0]);
    
    //--type = dax_cdt_new("TestCDT", NULL);
    //--dax_cdt_member(type, "TheDint", DAX_DINT, 1);
    //--dax_cdt_member(type, "TheInt", DAX_INT, 2);
    //dax_cdt_finalize(type);
    assert(0); //--Assertion because of the above commented function
    
    dax_tag_add(&handle, "TestCDTRead", type, 1);
    dax_tag_handle(&handle, "TestCDTRead.TheInt[1]", 1);
    write_data[1] = 444;
    dax_write_tag(handle, &write_data[1]);
    dax_read_tag(handle, &read_data[1]);
    printf("read_data[1] = %d\n", read_data[1]);
    
    
    return 0;
}

static int
get_status_tag_test(void)
{
    Handle handle;
    int result;
    
    result = dax_tag_handle(&handle, "_status", 0);
    if(result != 0) {
        printf("Get _status handle test failed\n");
    }
    return result;
}

int
main(int argc,char *argv[])
{
    int result=0, flags;
    char *script;
    lua_State *L;
    
    
    dax_log("Starting module test");
    dax_set_debug_topic(0xFFFF); /* This should get them all out there */
        
    dax_init_config("daxtest");
    flags = CFG_CMDLINE | CFG_DAXCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute("exitonfail","exitonfail", 'x', flags, "0");
    result += dax_add_attribute("testscript","testscript", 't', flags, "daxtest.lua");
    
    dax_configure(argc, argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);
    
    if(dax_mod_register("daxtest"))
        dax_fatal("Unable to register with the server");
    
    script = dax_get_attr("testscript");

    /* Free the configuration memory once we are done with it */
    dax_free_config();

    L = lua_open();
    /* This adds all of the Lua functions to the lua_State */
    add_test_functions(L);
    
    /* load and run the configuration file */
    if(luaL_loadfile(L, script)  || lua_pcall(L, 0, 0, 0)) {
        dax_error("Problem executing configuration file - %s", lua_tostring(L, -1));
        return ERR_GENERIC;
    }
    
    /* That will do it for the real LUA tests.  The following code is just
     * temporary code for testing along with new development */
           
    get_status_tag_test();
    /* TODO: Check the corner conditions of the tag reading and writing.
     * Offset, size vs. tag size etc. */
    //tag_read_write_test();

    printf("Starting Temporary CDT Test\n");
    dax_cdt *test_cdt;
    test_cdt = dax_cdt_new("TestCDT", NULL);
    result = dax_cdt_member(test_cdt, "Member1", DAX_DINT, 1);
    if(result) printf("Problem Adding Member1\n");
    result = dax_cdt_member(test_cdt, "Member1", DAX_DINT, 1);
    if(result) printf("This was supposed to fail\n");
    result = dax_cdt_member(test_cdt, "Member2", DAX_DINT, 1);
    if(result) printf("Problem Adding Member2\n");
    result = dax_cdt_member(test_cdt, "Member3", DAX_DINT, 1);
    if(result) printf("Problem Adding Member3\n");
    dax_cdt_create(test_cdt);
    
    /* TEST TO DO
        Tag Read / Write Test
    */
    
#ifdef DKFOOEIT74YEJKIOUWERHLVKSJDHIUHFEW
    if(check_tag_events()) {
        dax_log("Tag Event Test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tag Event Test - PASSED");
    }
    
#endif    
    dax_debug(LOG_MAJOR, "OpenDAX Test Finished, %d tests run, %d tests failed",
              tests_run(), tests_failed());
    //sleep(5);
    dax_mod_unregister();
    
    return 0;
}
