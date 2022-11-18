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
#include <sqlite3.h>

extern _dax_tag_db *_db;

#ifdef HAVE_SQLITE
static sqlite3 *_sql;
static sqlite3_stmt *update_stmt;
    
#else
static int _fd;
static uint16_t _version;
static uint32_t _last_tag_pointer;
#endif


#ifdef HAVE_SQLITE
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
        
        tag_index = tag_add(name, cdt_get_type(type), count, 0);
        
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
#else

/* read through the file and create the tags in the database */
static int
_create_tags(void) {
    char buff[256];
    uint8_t name_size, flags;
    uint32_t tag_pointer, tag_size, tag_type, tag_count;
    tag_index tag_index;

    lseek(_fd, 12, SEEK_SET); /* Location of the first tag record pointer */
    read(_fd, &tag_pointer, 4);
    if(tag_pointer == 0) return 0; /* No tags to create, bail out */
    while(1) {
        lseek(_fd, tag_pointer, SEEK_SET);
        read(_fd, &tag_pointer, 4); /* Pointer to the next tag */
        read(_fd, &tag_size, 4);
        read(_fd, &name_size, 1);
        read(_fd, &flags, 1);
        read(_fd, &tag_type, 4);
        read(_fd, &tag_count, 4);
        read(_fd, buff, name_size);
        buff[name_size] = 0x00;
        /* We don't actually delete records in the file.  We just mark the tag as deleted
         * and then ignore it here. */
        if((flags & RET_FLAG_DELETED) == 0x00) {
            /* Create the tag as we found it, without the retention attribute
             * If the rest of the system doesn't add this tag with the retention
             * attribute then it'll be removed this go around */
            tag_index = tag_add(buff, tag_type, tag_count, 0);
            if(tag_index < 0) return tag_index;
            read(_fd, _db[tag_index].data, tag_size);
        }
        if(tag_pointer == 0) return 0; /* We're done */
    } /* End While */
}


/* Retention File Format
 *
 * HEADER
 * Byte,  Size,  Type,   Description
 * 0      6      ASCII   "DAXRET", File Signature
 * 6      2      UINT16   Version
 * 8      4      UINT32   First Data Type Record Pointer
 * 12     4      UINT32   First Tag Record Pointer
 * 16     ~               Data Records
 *
 * TAG RECORD (Bytes are offsets from record start)
 * Byte,  Size,  Type,   Description
 * 0      4      UINT32  Pointer to the next tag record (essentially a linked list)
 * 4      4      UINT32  The Tags Data Size
 * 8      1      UINT8   Size of the Tag Name in the record
 * 9      1      BYTE    Flags Byte
 * 10     4      UINT32  Data Type (same as OpenDAX Type)
 * 14     4      UINT32  Tag Item Count (for arrays)
 * 18     (8)    CHAR    Tag Name (Size in Byte 8)
 * 18+(8) (4)    VOID    Tag Data
 *
 * TAG FLAG BYTE
 * Bit,   Description
 * 0      RET_FLAG_DELETED, Tag has been deleted after initialization
 *
 * DATA TYPE RECORD (Currently Not Implemented) (Bytes are offsets from record start)
 * Byte,   Size,  Type,   Description
 * 0       4      UINT32  Pointer to the next data type record (essentially a linked list)
 * 4       2      UINT16  Data Type description size
 * 6       4      UINT32  Data Type ID (same as OpenDAX)
 * 10      1      UINT8   Name Size
 * 11      (10)   CHAR    Data Type Name
 * 11+(10) (4)    CHAR    Data Type Description (string from serialize_datatype() function)
 */

/* There is a major bug in here where if a tag grows after it has been added there
 * will be some corruption of the file */

static int
_init(char *filename) {
    int result;
    char buff[8];

    dax_log(LOG_WARN, "File based tag retention is not complete");
    _fd = open(filename, O_RDWR);
    if(_fd < 0) {
        /* If the file does not exist we will try to create it */
        if(errno == ENOENT) {
            _fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if(_fd < 0) {
                dax_log(LOG_ERROR, "Unable to create Retention File, %s - %s", filename, strerror);
                _fd = 0;
                return ERR_GENERIC;
            }
            result = write(_fd, "DAXRET\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
            if(result != 16) {
                dax_log(LOG_ERROR, "Error Writing to Retention File");
                _fd=0;
                return ERR_GENERIC;
            }
            return 0;
        } else { /* Some other error opening the file */
            dax_log(LOG_ERROR, "Error Opening Retention File, %s - %s",filename, strerror(errno));
            return ERR_GENERIC;
        }
    }
    /* If we make it this far we should have the file open */
    result = read(_fd, buff, 8);
    if(result != 8) {
        dax_log(LOG_ERROR, "Error reading file signature");
        _fd = 0;
        return ERR_GENERIC;
    }
    if(strncmp("DAXRET", buff, 6) != 0) {
        dax_log(LOG_ERROR, "File signature doesn't match");
        _fd = 0;
        return ERR_GENERIC;
    }
    memcpy(&_version, &buff[6], 2);

    if(_version != 1) {
        dax_log(LOG_ERROR, "Wrong File Version");
        _fd = 0;
        return ERR_GENERIC;
    }
    /* If we get here then all is good */
    result = _create_tags();
    /* Truncate the file and write the default header */
    result = ftruncate(_fd, 0);
    result = lseek(_fd, 0, SEEK_SET);
    result = write(_fd, "DAXRET\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
    return 0;
}

#endif /* HAVE_SQLITE */

int
ret_init(char *filename) {
    if(filename == NULL) {
        filename = "retentive.db";
    }
    return _init(filename);
}

#ifdef HAVE_SQLITE
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

    result = sqlite3_bind_blob(update_stmt, 1, _db[index].data, type_size(_db[index].type), NULL);
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
static int
_write_tag_def(int index, size_t offset) {
    unsigned char name_size, flags;
    uint32_t data_size, tmp;

    name_size = strlen(_db[index].name);
    flags = 0x00;
    data_size = tag_get_size(index);
    lseek(_fd, offset, SEEK_SET);
    tmp = 0;
    write(_fd, &tmp, 4); /* pointer to next tag, 0 = we are the last one*/
    write(_fd, &data_size, 4); /* size of the data area */
    write(_fd, &name_size, 1); /* size of the tag name */
    write(_fd, &flags, 1); /* flag byte */
    write(_fd, &_db[index].type, 4);
    write(_fd, &_db[index].count, 4);
    write(_fd, _db[index].name, name_size);
    write(_fd, _db[index].data, data_size);
    /* This points to the offset in the file where we'll write this tags data */
    _db[index].ret_file_pointer = offset + 18 + name_size;

    return 0; /* TODO: Handle errors */
}

int
ret_add_tag(int index) {
    uint32_t offset;

    dax_log(LOG_DEBUG, "Adding Retained Tag at index %d", index);
    /* TODO: Implement retaining custom data type tags */
    if(IS_CUSTOM(_db[index].type)) return ERR_NOTIMPLEMENTED;
    if(_fd == 0) return ERR_FILE_CLOSED;
    offset = lseek(_fd, 0, SEEK_END); /* We're writing to the end no matter what */
    if(_last_tag_pointer == 0) { /* This is our first tag */
        lseek(_fd, 12, SEEK_SET); /* first tag pointer */
        write(_fd, &offset, sizeof(offset));
        _write_tag_def(index, offset);
        _last_tag_pointer = offset;
    } else {
        _write_tag_def(index, offset);
        lseek(_fd, _last_tag_pointer, SEEK_SET); /* Seek to the last tag */
        /* Write the offset to the tag we just added to the previous tag */
        write(_fd, &offset, 4);
        _last_tag_pointer = offset;
    }
    return 0;
}


int
ret_del_tag(int index) {
    uint32_t offset;
    uint8_t flags = RET_FLAG_DELETED;
    offset = _db[index].ret_file_pointer - strlen(_db[index].name - 9);
    lseek(_fd, offset, SEEK_SET);
    write(_fd, &flags, 1);
    return 0;
}


int
ret_tag_write(int index) {
    uint32_t offset, data_size;

    data_size = tag_get_size(index);
    offset = _db[index].ret_file_pointer;
    lseek(_fd, offset, SEEK_SET);
    write(_fd, _db[index].data, data_size);
    return 0;
}


int
ret_close(void) {
    if(_fd) close(_fd);
    return 0;
}

#endif /* HAVE_SQLITE */