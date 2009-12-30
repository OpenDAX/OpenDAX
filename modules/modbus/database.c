/* database.c - Modbus module for OpenDAX
 * Copyright (C) 2006 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <database.h>
#include <pthread.h>

static pthread_mutex_t dax_mutex;

/* This initializes all the data that we need to deal with for the database */
int
init_database(void) {
    fprintf(stderr, "Initializing Database\n");
    pthread_mutex_init(&dax_mutex, NULL);
    return 0;
}

/* These are the callbacks that get assigned to each command */
static void
_write_data(struct mb_cmd *c, void *userdata, u_int8_t *data, int datasize)
{
    /* It really should be this easy if we have done everything right up to here */
    pthread_mutex_lock(&dax_mutex);
    dax_write_tag(*((Handle *)userdata), data);
    pthread_mutex_unlock(&dax_mutex);
}


static void
_read_data(struct mb_cmd *c, void *userdata, u_int8_t *data, int datasize)
{
    /* It really should be this easy if we have done everything right up to here */
    pthread_mutex_lock(&dax_mutex);
    dax_read_tag(*((Handle *)userdata), data);
    pthread_mutex_unlock(&dax_mutex);
}


/* This function was setup as the pre_send callback to the modbus library.
 * If this function is called it means that the command is about to be sent for
 * the first time.  We add the OpenDAX tags, change the userdata in the command
 * to point to the tag handle of the OpenDAX tags, free the old userdata and
 * set up the permanent callbacks that will actually handle the data */
void
setup_command(mb_cmd *c, void *userdata, u_int8_t *data, int datasize)
{
    int result, count;
    cmd_temp_data *cdata;
    Handle *h;
    char tagname[DAX_TAGNAME_SIZE + 20];
    
    h = malloc(sizeof(Handle));
    if(h == NULL) {
        return;
    }
    
    cdata = (cmd_temp_data *)userdata;
    
    switch(cdata->function) {
        case 1:
        case 2:
        case 15:
            count = cdata->length;
            result = dax_tag_add(h, cdata->tagname, DAX_BOOL, cdata->index + cdata->length);
            break;
        case 5:
            count = 1;
            result = dax_tag_add(h, cdata->tagname, DAX_BOOL, cdata->index + 1);
            break;
        case 3:
        case 4:
        case 16:
            count = cdata->length;
            result = dax_tag_add(h, cdata->tagname, DAX_UINT, cdata->index + cdata->length);
            break;
        case 6:
            count = 1;
            result = dax_tag_add(h, cdata->tagname, DAX_UINT, cdata->index + 1);
            break;
        default:
            return;
    }
    
    snprintf(tagname, DAX_TAGNAME_SIZE + 20, "%s[%d]", cdata->tagname, cdata->index);
    free(userdata); /* This offsets the malloc in _add_command() */
    mb_set_userdata(c, NULL); /* Just so we know */
           
    result = dax_tag_handle(h, tagname, count);

    if(result == 0) {
        /* Since h is allocated with a single malloc call we don't have to free
         * it because it will be freed when the command is destroyed */
        mb_set_userdata(c, h);
    }
    mb_pre_send_callback(c, NULL);
    if(mb_is_write_cmd(c)) {
        mb_pre_send_callback(c, _read_data);
        /* Since this is a pre call we go ahead and call the function here */
        _read_data(c, userdata, data, datasize);
    } else if(mb_is_read_cmd(c)) {
        mb_post_send_callback(c, _write_data);
    }
}
