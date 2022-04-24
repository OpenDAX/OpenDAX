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
 *  Main source code file for the OpenDAX Historical Logging file plugin
 */

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include <dhl_file.h>

static const char *log_directory;
static FILE *log_file;

int
init(dax_state *ds) {
    int result;
    lua_State *L;
    char filename[256]; /* Should probably allocate this */

    L = dax_get_luastate(ds);
    lua_getglobal(L, "directory");
    log_directory = lua_tostring(L, -1);
    if(log_directory == NULL) {
        log_directory = "logs";
    }
    DIR* dir = opendir(log_directory);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
    } else if (ENOENT == errno) {
        result = mkdir(log_directory, 0777);
        if(result) {
            dax_error(ds, "%s", strerror(errno));
        }
    } else {
        dax_error(ds, "%s", strerror(errno));
    }
    snprintf(filename, 256, "%s/somereallylongdatetimething.daxlog", log_directory);
    log_file = fopen(filename, "a");
    return 0;
}

int
set_config(const char *attr, char *value) {
    fprintf(stderr, "Set attribute %s = %s\n", attr, value);
    return 0;
}
char *
get_config(const char *attr) {
    fprintf(stderr, "Get attribute %s\n", attr);
    return "OK";
}


tag_object *
add_tag(const char *tagname, uint32_t type, const char *attributes) {
    tag_object *tag;

    tag = malloc(sizeof(tag_object));
    if(tag == NULL) return NULL;
    tag->name = tagname;
    tag->type = type;

    fprintf(stderr, "Adding ye olde tag %s type = %d\n", tagname, type);
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

    dax_val_to_string(val_string, 256, tag->type, value, 0);
    fprintf(log_file, "%s,%f,%s\n", tag->name, timestamp, val_string);
    return 0;
}

int
flush_data(void) {
    fflush(log_file);
    // TODO: Should also create new files and delete old files based on configuration here
    return 0;
}
