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
#include "func.h"
#include "tagbase.h"
#include <sys/stat.h>
#include <fcntl.h>

extern _dax_tag_db *_db;

static int _fd;
static u_int16_t _version;
static u_int32_t _last_type_pointer;
static u_int32_t _last_tag_pointer;
//static u_int32_t _next_offset;


int
ret_init(char *filename) {
    int result;
    char buff[8];

    xlog(LOG_MINOR, "Setting up Tag Retention");
    if(filename == NULL) {
        filename = "retentive.db";
    }
    _fd = open(filename, O_RDWR);
    if(_fd < 0) {
        /* If the file does not exist we will try to create it */
        if(errno == ENOENT) {
            _fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if(_fd < 0) {
                xerror("Unable to create Retention File, %s - %s", filename, strerror);
                _fd = 0;
                return ERR_GENERIC;
            }
            result = write(_fd, "DAXRET\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", 16);
            if(result != 16) {
                xerror("Error Writing to Retention File");
                _fd=0;
                return ERR_GENERIC;
            }
            return 0;
        } else { /* Some other error opening the file */
            xerror("Error Opening Retention File, %s - %s",filename, strerror(errno));
            return ERR_GENERIC;
        }
    }
    /* If we make it this far we should have the file open */
    result = read(_fd, buff, 8);
    if(result != 8) {
        xerror("Error reading file signature");
        _fd = 0;
        return ERR_GENERIC;
    }
    if(strncmp("DAXRET", buff, 6) != 0) {
        xerror("File signature doesn't match");
        _fd = 0;
        return ERR_GENERIC;
    }
    _version = (buff[6]<<8) | buff[7];

    if(_version != 1) {
        xerror("Wrong File Version");
        _fd = 0;
        return ERR_GENERIC;
    }
    /* If we get here then all is good */
    /* TODO: Do the thing you do */
    return 0;
}

static int
_write_tag(int index, size_t offset) {
    unsigned char name_size, flags;
    u_int32_t data_size, tmp;

    name_size = strlen(_db[index].name);
    flags = 0x00;
    data_size = type_size(_db[index].type) * _db[index].count;
    lseek(_fd, offset, SEEK_SET);
    tmp = 0;
    write(_fd, &tmp, 4); /* pointer to next tag, 0 = we are the last one*/
    write(_fd, &data_size, 4); /* size of the data area */
    write(_fd, &name_size, 1); /* size of the tag name */
    write(_fd, &flags, 1); /* flag byte */
    write(_fd, _db[index].name, name_size);
    write(_fd, _db[index].data, data_size);
    /* This points to the offset in the file where we'll write this tags data */
    _db[index].ret_file_pointer = offset + 10 + name_size;

    return 0; /* TODO: Handle errors */
}

int
ret_add_tag(int index) {
    u_int32_t offset;

    xlog(LOG_MINOR, "Adding Retained Tag at index %d", index);
    /* TODO: Implement retaining custom data type tags */
    if(IS_CUSTOM(_db[index].type)) return ERR_NOTIMPLEMENTED;
    if(_fd == 0) return ERR_FILE_CLOSED;
    offset = lseek(_fd, 0, SEEK_END); /* We're writing to the end no matter what */
    if(_last_tag_pointer == 0) { /* This is our first tag */
        lseek(_fd, 12, SEEK_SET); /* first tag pointer */
        write(_fd, &offset, sizeof(offset));
        _write_tag(index, offset);
        _last_tag_pointer = offset;
    } else {
        _write_tag(index, offset);
        lseek(_fd, _last_tag_pointer, SEEK_SET); /* Seek to the last tag */
        /* Write the offset to the tag we just added to the previous tag */
        write(_fd, &offset, 4);
        _last_tag_pointer = offset;
    }
    return 0;
}

int
ret_tag_write(int index) {
    u_int32_t offset, data_size;

    data_size = type_size(_db[index].type) * _db[index].count;
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
