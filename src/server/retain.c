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

 *  Source code file for tag retention functions
 */

#include <common.h>
#include "retain.h"
#include "func.h"
#include "tagbase.h"
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SQLITE
#include <sqlite3.h>
#endif

extern _dax_tag_db *_db;

#ifdef HAVE_SQLITE
static sqlite3 *_sql;
static sqlite3_stmt *update_stmt;

/* Create the types that are found in the retention database */
static int
_create_types(void) {
    sqlite3_stmt *stmt;
    int result, error;
    char *name;
    char *definition;

    result = sqlite3_prepare_v2(_sql, "SELECT name,definition FROM types ORDER BY id;", -1, &stmt, NULL);
    if(result != SQLITE_OK) {
        /* Most errors are because we have a new file */
        dax_log(LOG_ERROR, "Unable to compile SQL statement to create types");
        sqlite3_finalize(stmt);
        return result;
    }
    while((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        name = (char *)sqlite3_column_text(stmt, 0);
        definition = (char *)sqlite3_column_text(stmt, 1);
        result = cdt_create(definition, &error);
        if(result == 0) {
            dax_log(LOG_ERROR, "Problem creating datatype %s - %d", name, error);
        }
    }
    if(result != SQLITE_DONE) {
        dax_log(LOG_ERROR, "Problem writing tags to database - %d", result);
        sqlite3_finalize(stmt);
        return result;
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* Create the tags that we find in the database */
static int
_create_tags(void) {
    sqlite3_stmt *stmt;
    int result;
    char *name;
    char *type;
    const void *data;
    int count, size;
    tag_index tag_index;


    // CREATE ALL THE DATATYPES FIRST
    result = sqlite3_prepare_v2(_sql, "SELECT name,type,count,data FROM tags;", -1, &stmt, NULL);
    if(result != SQLITE_OK) {
        /* Most errors are because we have a new file */
        dax_log(LOG_ERROR, "Unable to compile SQL statement to create tags");
        sqlite3_finalize(stmt);
        return result;
    }
    while((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        name = (char *)sqlite3_column_text(stmt, 0);
        type = (char *)sqlite3_column_text(stmt, 1);
        count = sqlite3_column_int(stmt, 2);
        data =  sqlite3_column_blob(stmt, 3);
        size =  sqlite3_column_bytes(stmt, 3);

        tag_index = tag_add(-1, name, cdt_get_type(type), count, 0);

        if(tag_index < 0) {
            dax_log(LOG_ERROR, "Retained tag not created properly");
        } else {
            memcpy(_db[tag_index].data, data, MIN(size, tag_get_size(tag_index)));
        }
    }
    if(result != SQLITE_DONE) {
        dax_log(LOG_ERROR, "Problem writing tags to database - %d", result);
        sqlite3_finalize(stmt);
        return result;
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* Create the databae tables */
static int
_create_database_tables(void) {
    char *errormsg;
    int result;
    const char *query = "CREATE TABLE IF NOT EXISTS types (" \
	                    "id     INTEGER PRIMARY KEY,"\
	                    "name   TEXT NOT NULL UNIQUE," \
                        "definition TEXT);" \
                        "CREATE TABLE IF NOT EXISTS tags (" \
	                    "id     INTEGER PRIMARY KEY AUTOINCREMENT," \
	                    "name   TEXT NOT NULL UNIQUE," \
	                    "type   TEXT NOT NULL," \
	                    "count  INTEGER," \
                        "data   BLOB);";
    result = sqlite3_exec(_sql, query, NULL, 0 , &errormsg);
    if(result != SQLITE_OK) {
        dax_log(LOG_ERROR, "Unable to create tag retention database tables - %s", errormsg);
        return result;
    }
    return 0;
}

static int
_init(char *filename) {
    int result;
    char *errmsg;

    dax_log(LOG_DEBUG, "Setting up SQLite Tag Retention - %s", filename);
    result = sqlite3_open(filename, &_sql);
    if(result != SQLITE_OK) {
        dax_log(LOG_ERROR, "Unable to open tag retention database - %s", filename);
        sqlite3_close(_sql);
        _sql=NULL;
        return result;
    }
    /* First we attempt to create data types and tags if they exist in the retention database
       These may fail if the file doesn't exist but that's okay */
    result = _create_types(); /* Creates the types that are stored in the data base */
    result = _create_tags(); /* Creates the tags that are in the data base */
    result = sqlite3_exec(_sql, "DROP TABLE IF EXISTS tags;", NULL, 0, &errmsg);
    if(result != SQLITE_OK) {
        DF("unable to drop table 'tags' - %s", errmsg);
    }
    result = sqlite3_exec(_sql, "DROP TABLE IF EXISTS types;", NULL, 0, &errmsg);
    if(result != SQLITE_OK) {
        DF("unable to drop table 'types' - %s", errmsg);
    }
    result = _create_database_tables();
    result = sqlite3_prepare_v2(_sql, "UPDATE tags SET data = ?1 WHERE id = ?2;", -1, &update_stmt, NULL);
    if(result) {
        DF("problem with prepare %d", result);
    }
    return 0;
}

int
ret_init(char *filename) {
    if(filename == NULL) {
        filename = "retentive.db";
    }
    return _init(filename);
}

int
_add_type(tag_type type) {
    datatype *dt;
    cdt_member *next;
    char *type_str;
    sqlite3_stmt *stmt;
    int index;
    int result;
    char query[256];

    dt = cdt_get_entry(type);
    index = CDT_TO_INDEX(type);

    if(dt == NULL) {
        dax_log(LOG_ERROR, "Bad type passed");
    }
    /* Loop through all the members and recursively add any
       compound data types that we find */
    next = dt->members;
    while(next != NULL) {
        if(IS_CUSTOM(next->type)) {
            _add_type(next->type);
        }
        next = next->next;
    }
    /* Turn the type into a string that we can use to reload it later */
    serialize_datatype(type, &type_str);
    if(_sql == NULL) return ERR_FILE_CLOSED;

    snprintf(query, 256, "INSERT INTO main.types(id,name,definition) VALUES (%d,'%s',?);",index, dt->name);

    result = sqlite3_prepare_v2(_sql, query, -1, &stmt , NULL);
    if(result != SQLITE_OK) {
        dax_log(LOG_ERROR, "Unable to create tag in retention ");
        return result;
    }
    sqlite3_bind_text(stmt, 1, type_str, -1, NULL);
    result = sqlite3_step(stmt);
    if(result != SQLITE_DONE) {
        dax_log(LOG_ERROR, "Problem inserting tag");
        return -1;
    }
    sqlite3_finalize(stmt);

    return 0;
}

int
ret_add_tag(int index) {
    sqlite3_stmt *stmt;
    int id;
    int result;
    char query[256];

    dax_log(LOG_DEBUG, "Adding Retained Tag at index %d", index);
    /* TODO: Implement retaining custom data type tags */
    if(IS_CUSTOM(_db[index].type)) {
        _add_type(_db[index].type);
    }
    if(_sql == NULL) return ERR_FILE_CLOSED;
    snprintf(query, 256, "INSERT INTO main.tags(name,type,count,data) VALUES ('%s','%s',%d,NULL);",
                         _db[index].name, cdt_get_name(_db[index].type), _db[index].count);

    result = sqlite3_prepare_v2(_sql, query, -1, &stmt , NULL);
    if(result != SQLITE_OK) {
        dax_log(LOG_ERROR, "Unable to create tag in retention ");
        return result;
    }
    result = sqlite3_step(stmt);
    if(result == SQLITE_DONE) {
        /* This might be a race condition if we ever multi thread the server */
        id = sqlite3_last_insert_rowid(_sql);
        _db[index].ret_file_pointer = id; /* We'll use this on tag writes */
    } else {
        dax_log(LOG_ERROR, "Problem inserting tag");
        return -1;
    }
    sqlite3_finalize(stmt);
    return 0;
}

int
ret_del_tag(int index) {

    dax_log(LOG_DEBUG, "Deleting Retained Tag at index %d", index);

    return 0;
}

int
ret_tag_write(int index) {
    int result;

    result = sqlite3_bind_blob(update_stmt, 1, _db[index].data, type_size(_db[index].type) * _db[index].count, NULL);
    if(result) {
        DF("Problem with bind_blob %d", result);
    }
    result = sqlite3_bind_int(update_stmt, 2, _db[index].ret_file_pointer);
    if(result) {
        DF("Problem with bind_int %d", result);
    }
    result = sqlite3_step(update_stmt);
    result = sqlite3_reset(update_stmt);
    return 0;
}

int
ret_close(void) {

    dax_log(LOG_DEBUG, "Closing Tag Data Retention");
    return 0;
}

#else

int
ret_init(char *filename) {
   dax_log(LOG_WARN, "No SQLite3 available so tag retention will not work");
   return 0;
}


int
ret_add_tag(int index) {
    return 0;
}


int
ret_del_tag(int index) {
    return 0;
}


int
ret_tag_write(int index) {
    return 0;
}


int
ret_close(void) {
    return 0;
}

#endif /* HAVE_SQLITE */