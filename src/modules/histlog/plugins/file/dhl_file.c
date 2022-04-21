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

#include <dhl_file.h>

int
init(const char *file) {
    fprintf(stderr, "Initializing the thing with the thing %s\n", file);
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

    fprintf(stderr, "Adding ye olde tag %s\n", tagname);
    return NULL;
}

int
free_tag(tag_object *tag) {
    free(tag->name);
    free(tag);
    return 0;
}

int
write_data(uint32_t index, void *value, double timestamp) {
    fprintf(stderr, "Writing data for tag at index %d\n", index);
    return 0;
}
