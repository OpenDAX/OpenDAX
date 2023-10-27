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

static dax_state *ds;
static tag_handle rotate_handle;

static const char *log_directory;
static char *rotate_tag;
static FILE *log_file;
static int rotate_flag;

static double (*_gettime)(void);

static void
_rotate_callback(dax_state *_ds, void *udata) {
    rotate_flag = 1;
}

static void
_open_file(void) {
    char filename[256]; /* Should probably allocate this */

    snprintf(filename, 256, "%s/dax_%f.log", log_directory, _gettime());
    log_file = fopen(filename, "a");
}

int
init(dax_state *_ds) {
    int result;
    lua_State *L;
    const char *s;

    ds = _ds;
    L = dax_get_luastate(ds);
    lua_getglobal(L, "directory");
    s = lua_tostring(L, -1);
    if(s == NULL) {
        log_directory = "logs";
    } else {
        log_directory = strdup(s);
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

    DIR* dir = opendir(log_directory);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
    } else if (ENOENT == errno) {
        result = mkdir(log_directory, 0777);
        if(result) {
            dax_log(DAX_LOG_ERROR, "%s", strerror(errno));
        }
    } else {
        dax_log(DAX_LOG_ERROR, "%s", strerror(errno));
    }
    _open_file();
    return 0;
}


tag_object *
add_tag(const char *tagname, uint32_t type, const char *attributes) {
    tag_object *tag;

    tag = malloc(sizeof(tag_object));
    if(tag == NULL) return NULL;
    tag->name = tagname;
    tag->type = type;
    dax_log(DAX_LOG_DEBUG, "Added tag %s type = %d", tag->name, tag->type);

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
    if(value == NULL) {
        fprintf(log_file, "%s,%f,NULL\n", tag->name, timestamp);
    } else {
        dax_val_to_string(val_string, 256, tag->type, value, 0);
        fprintf(log_file, "%s,%f,%s\n", tag->name, timestamp, val_string);
    }
    return 0;
}

static void
_add_tags(void) {
    int result;

    result = dax_tag_add(ds, &rotate_handle, rotate_tag, DAX_BOOL, 1, 0);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to add file rotation tag %s", rotate_tag);
    } else {
        result=  dax_event_add(ds, &rotate_handle, EVENT_SET, NULL, NULL, _rotate_callback, NULL, NULL);
    }
}

int
flush_data(double time) {
    static int firstrun = 1;

    if(firstrun) {
        _add_tags();
        firstrun = 0;
    }
    if(rotate_flag) {
        dax_sint val = 0;
        DF("rotate");
        fclose(log_file);
        _open_file();
        dax_write_tag(ds, rotate_handle, &val); /* reset the command */
        rotate_flag = 0;
    }
    fflush(log_file);
    // TODO: Should also create new files and delete old files based on configuration here
    return 0;
}

void
set_timefunc(double (*f)(void)) {
    _gettime=f;
}
