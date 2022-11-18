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
 *
 *  Main source code file for the OpenDAX Historical Logging SQLite plugin
 */

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sqlite3.h>

#include "dhl_sqlite.h"

static dax_state *ds;
static tag_handle rotate_handle;

static const char *database_filename;
static sqlite3 *log_db;
static char *rotate_tag;
static int rotate_flag;

static double (*_gettime)(void);

static void
_rotate_callback(dax_state *_ds, void *udata) {
    rotate_flag = 1;
}

static int
_open_file(void) {
    int result;
    result = sqlite3_open(database_filename, &log_db);
    if(result != SQLITE_OK) {
        dax_log(LOG_ERROR, "Unable to open database file %s", database_filename);
    }
    return result;
}

static int
_build_database(sqlite3 *db) {
    char *errorMsg = NULL;
    int result;
    char *sql = "CREATE TABLE Tags (" \
        "id	INTEGER,        " \
        "tagname	TEXT NOT NULL UNIQUE," \
        "description	TEXT," \
        "type INTEGER NOT NULL," \
        "PRIMARY KEY(id)" \
        ");" \
        "CREATE TABLE Data ( " \
	    "id	INTEGER NOT NULL, " \
	    "tagid	INTEGER NOT NULL, " \
	    "timestamp REAL NOT NULL," \
	    "data	NUMERIC, " \
	    "PRIMARY KEY(id), " \
	    "FOREIGN KEY(tagid) REFERENCES Tags(id) " \
        ");" \
        "CREATE TABLE MetaData ( " \
	    "id	INTEGER NOT NULL, " \
	    "key	TEXT NOT NULL UNIQUE, " \
	    "val	TEXT NOT NULL, " \
	    "PRIMARY KEY(id)" \
	    ");"; 
    //DF("%s\n", sql);
    result = sqlite3_exec(db, sql, NULL, 0, &errorMsg);
    if( result != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s - %d\n", errorMsg, result);
        sqlite3_free(errorMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
    result = sqlite3_exec(db, "INSERT INTO MetaData(key, val) VALUES ('VERSION', '1');", NULL, 0, &errorMsg);
    if( result != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s - %d\n", errorMsg, result);
        sqlite3_free(errorMsg);
    } else {
        fprintf(stdout, "Version Added Correctly\n");
    }
    return 0;
}


int
init(dax_state *_ds) {
    //int result;
    lua_State *L;
    const char *s;

    ds = _ds;
    L = dax_get_luastate(ds);
    lua_getglobal(L, "filename");
    s = lua_tostring(L, -1);
    if(s == NULL) {
        database_filename = "logs";
    } else {
        database_filename = strdup(s);
    }
    lua_pop(L, 1);

    lua_getglobal(L, "rotate_tag");
    s = lua_tostring(L, -1);
    if(s == NULL) {
        rotate_tag = "hist_rotate";
    } else {
        rotate_tag = strdup(s);
    }
    lua_pop(L, 1);

    _open_file();
    _build_database(log_db);
    return 0;
}

static int
_add_tag_callback(void *userdata, int count, char** val, char** key) {
    int n;
    uint32_t *tag_index = (uint32_t*)userdata;
    
    *tag_index = atoll(val[0]);
    return 0;
}

tag_object *
add_tag(const char *tagname, uint32_t type, const char *attributes) {
    tag_object *tag;
    char *errorMsg = NULL;
    int result;
    char sql[256];

    tag = malloc(sizeof(tag_object));
    if(tag == NULL) return NULL;
    tag->name = tagname;
    tag->type = type;

    snprintf(sql, 256, "INSERT INTO Tags('tagname', 'type') VALUES ('%s', 2)", tagname);
    result = sqlite3_exec(log_db, sql, NULL, 0, &errorMsg);
    if(result == 19) {
        dax_log(LOG_DEBUG, "Tag %s already exists in the database", tagname); // TODO: Check that the type is the same and error out otherwise
        sqlite3_free(errorMsg);
    } else if( result != SQLITE_OK){
        dax_log(LOG_ERROR, "SQL: %s - %d\n", errorMsg, result);
        sqlite3_free(errorMsg);
    }
    snprintf(sql, 256, "SELECT id FROM Tags WHERE tagname = '%s';", tagname); 
    result = sqlite3_exec(log_db, sql, _add_tag_callback, &tag->tag_index, &errorMsg);
    if( result != SQLITE_OK ){
        dax_log(LOG_ERROR, "SQL: %s - %d\n", errorMsg, result);
        sqlite3_free(errorMsg);
    }
    dax_log(LOG_DEBUG, "Added tag %s type = %d, index = %d", tag->name, tag->type, tag->tag_index);
    return tag;
}


int
free_tag(tag_object *tag) {
    free((void *)tag->name);
    free(tag);
    return 0;
}


int
write_data(tag_object *tag, void *value, double timestamp) {
    char val_string[256];
    char *errorMsg = NULL;
    int result;
    char sql[256];

    if(value == NULL) {
        snprintf(sql, 256, "INSERT INTO Data ('tagid', 'timestamp') VALUES (%d, %f);", tag->tag_index, timestamp); 
   } else {
        dax_val_to_string(val_string, 256, tag->type, value, 0);
        snprintf(sql, 256, "INSERT INTO Data ('tagid', 'timestamp','data') VALUES (%d, %f, %s);", tag->tag_index, timestamp, val_string); 
    }
    result = sqlite3_exec(log_db, sql, _add_tag_callback, &tag->tag_index, &errorMsg);
    if( result != SQLITE_OK ){
        dax_log(LOG_ERROR, "SQL: %s - %d\n", errorMsg, result);
        sqlite3_free(errorMsg);
    }
    return 0;
}

static void
_add_tags(void) {
    int result;

    result = dax_tag_add(ds, &rotate_handle, rotate_tag, DAX_BOOL, 1, 0);
    if(result) {
        dax_log(LOG_ERROR, "Unable to add file rotation tag %s", rotate_tag);
    } else {
        result=  dax_event_add(ds, &rotate_handle, EVENT_SET, NULL, NULL, _rotate_callback, NULL, NULL);
    }
}

int
flush_data(void) {
    static int firstrun = 1;

    if(firstrun) {
        _add_tags();
        firstrun = 0;
    }
    if(rotate_flag) {
        DF("Log Rotate");
        dax_sint val = 0;
        dax_write_tag(ds, rotate_handle, &val); /* reset the command */
        rotate_flag = 0;
    }
    return 0;
}

void
set_timefunc(double (*f)(void)) {
    _gettime=f;
}
