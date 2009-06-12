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

#define TEST 1000

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
    index = dax_tag_add("TestReadTagInt", DAX_INT, 32);
    
    dax_tag_handle(&handle, "TestReadTagInt", 32);
    
    write_data[0] = 0x5555;
    dax_write_tag(handle, write_data);
    dax_read_tag(handle, read_data);
    
    printf("read_data[0] = 0x%X\n", read_data[0]);
    
    type = dax_cdt_create("TestCDT", NULL);
    dax_cdt_add(type, "TheDint", DAX_DINT, 1);
    dax_cdt_add(type, "TheInt", DAX_INT, 2);
    dax_cdt_finalize(type);
    
    dax_tag_add("TestCDTRead", type, 1);
    dax_tag_handle(&handle, "TestCDTRead.TheInt[1]", 1);
    write_data[1] = 444;
    dax_write_tag(handle, &write_data[1]);
    dax_read_tag(handle, &read_data[1]);
    printf("read_data[1] = %d\n", read_data[1]);
    
    
    return 0;
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
    
/*    
    if(check_tag_retrieve()) {
        dax_log("Tagname retrieving test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tagname retrieving test - PASSED");
    }

    if( check_tagbase() ) {
        dax_log("Tagbase verification test - FAILED");
        tests_failed++;
    } else {
        dax_log("Tagbase verification test - PASSED");
    }
     
    
    handle[0] = dax_tag_add("WriteDINTTest", DAX_DINT, 10);
    for(n = 0; n < 10; n++) {
        dummy[n] = n*2;
    }
    dax_write_tag(handle[0], 0, dummy, 10, DAX_DINT);
    
    result = dax_read_tag(handle[0], 0, dummy, 10, DAX_DINT);
    
    for(n = 0; n < 10; n++) {
        //dax_read_tag(handle[0], n, dummy, 1, DAX_DINT);
        printf("WriteDINTTest[%d] = %d\n", n, dummy[n]);
    }
    
    type = dax_cdt_create("test", NULL);
    printf("dax_cdt_create(test) returned 0x%X\n", type);
    if(type) {
        result = dax_cdt_add(type, "TestBOOL", DAX_BOOL, 4);
        result = dax_cdt_add(type, "TestINT", DAX_INT, 2);
        //result = dax_cdt_add(type, "TestDINT", DAX_DINT, 8);
        //result = dax_cdt_add(type, "TestREAL", DAX_REAL, 1);
        result = dax_cdt_finalize(type);
        if(result) printf("Finalize failed\n");
    }
    
    dax_tag_add("TestTag", type, 1);
    
    printf("Get Type 'test' by name = 0x%X\n", dax_string_to_type("test"));
    printf("Get Type 0x%X by type = %s\n",type, dax_type_to_string(type));
    
    type = dax_cdt_create("combined", NULL);
    printf("dax_cdt_create(combined) returned 0x%X\n", type);
    if(type) {
        result = dax_cdt_add(type, "Int1", DAX_BYTE, 4);
        result = dax_cdt_add(type, "Bit1", DAX_BOOL, 5);
        result = dax_cdt_add(type, "Test1", dax_string_to_type("test"), 1);
        result = dax_cdt_add(type, "Dint1", DAX_DINT, 2);
        result = dax_cdt_finalize(type);
        if(result) printf("Finalize failed\n");
    }
    
    //printf("Get Type 'combined' by name = 0x%X\n", dax_string_to_type("combined"));
    printf("Get Type 0x%X by type = %s\n",type, dax_type_to_string(type));
    
    dax_tag_add("CombinedTag", type, 3);
    
    _check_cdt_handles();
    
    //--result = dax_cdt_get(0, NULL);
    //--result = dax_cdt_get(0, "test");
    //--result = dax_cdt_get(type, NULL);
    
    
    //handle[1] = dax_tag_add("WriteBOOLTest", DAX_BOOL, 32);
    //buff[0] = 0x51; buff[1] = 0xAB;
    //dax_write_tag(handle[1], 0, buff, 16, DAX_BOOL);
    //dax_write_tag(handle[1], 2, buff, 1, DAX_BOOL);
    //dax_write_tag(handle[1], 4, buff, 1, DAX_BOOL);
    //dax_write_tag(handle[1], 8, buff, 1, DAX_BOOL);
    
    for(n = 0; n < 16; n++) {
        dax_read_tag(handle[1], n, buff, 1, DAX_BOOL);
        printf("WriteBOOLTest[%d] = ", n);
        
        if(buff[0]) {
            printf("1\n");
        } else {
            printf("0\n");
        }
    }*/
       
    
    /* TODO: Check the corner conditions of the tag reading and writing.
     * Offset, size vs. tag size etc. */
    tag_read_write_test();

    
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
    
    dummy[0] = 0x0505;
    dummy[1] = 0x8888;
    dummy[2] = 0x3333;
    dummy[3] = 0x4444;
    dummy[4] = 0x7777;
    
    idx = dax_tag_add("BitTest", DAX_WORD, 10);
    printf("Handle = %d\n",idx);
    dax_tag_write_bits(idx + 1, &dummy, 77);
    dax_tag_read_bits(idx + 1, &test, 77);
    for(n=0;n<5;n++) {
        printf("Before write 0x%X: After Read 0x%X\n",dummy[n], test[n]);
    }
    
    
    /*
    for(n = 0; n<126; n++) {
        if(n % 5) {
            sprintf(tagname,"BOOL%d",n);
            handle = dax_tag_add(tagname,DAX_BOOL,1);
        } else {
            sprintf(tagname,"BYTE%d",n);
            handle = dax_tag_add(tagname,DAX_BYTE,1);
        }
    }
    
    handle = dax_tag_add("Byte1", DAX_BYTE, 1);
    handle = dax_tag_add("Dint2", DAX_DINT, 10);
    handle = dax_tag_add("Byte3", DAX_BYTE, 1);
    handle = dax_tag_add("Dint4", DAX_DINT, 100);
    handle = dax_tag_add("Bool5", DAX_BOOL, 1);
    handle = dax_tag_add("Dint6", DAX_DINT, 10);
    handle = dax_tag_add("Dint7", DAX_DINT, 100);
    handle = dax_tag_add("Bool8", DAX_BOOL, 5);
    handle = dax_tag_add("Bool9", DAX_BOOL, 1);
    handle = dax_tag_add("Real10", DAX_REAL, 5);
    handle = dax_tag_add("Byte11", DAX_BYTE, 1);
    handle = dax_tag_add("Real12", DAX_REAL, 5);
    handle = dax_tag_add("Real13", DAX_REAL, 5);
    handle = dax_tag_add("Real14", DAX_REAL, 5);
    handle = dax_tag_add("Real15", DAX_REAL, 5);
    handle = dax_tag_add("Real16", DAX_REAL, 5);
    handle = dax_tag_add("Real17", DAX_REAL, 5);
    handle = dax_tag_add("Real18", DAX_REAL, 5);
    handle = dax_tag_add("Real19", DAX_REAL, 38);
    handle = dax_tag_add("Real20", DAX_REAL, 1);
    handle = dax_tag_add("Bool21", DAX_BOOL, 1);
    handle = dax_tag_add("Bool22", DAX_BOOL, 1);
    handle = dax_tag_add("Bool23", DAX_BOOL, 1);
    handle = dax_tag_add("Bool24", DAX_BOOL, 1);
    
    //handle = dax_tag_add("test",DAX_UINT,100);
    //printf("Test received handle 0x%X\n",handle);
    */
     
    /* Verify the integrity of the tag database */
#endif    
    dax_debug(LOG_MAJOR, "OpenDAX Test Finished, %d tests run, %d tests failed",
              tests_run(), tests_failed());
    //sleep(5);
    dax_mod_unregister();
    
    return 0;
}
