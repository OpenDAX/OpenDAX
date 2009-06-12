/* modcmds.c - Modbus (tm) Communications Library
 * Copyright (C) 2009 Phil Birkelbach
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
 *
 * Source file for mb_cmd handling functions
 */

#include <modbus.h>
#include <modlib.h>

/* Sets the command values to some defaults */
static void
initcmd(struct mb_cmd *c)
{
    c->method = 0;
    c->node = 0;
    c->function = 0;
    c->m_register = 0;
    c->length = 0;
    c->data = NULL;
    c->datasize = 0;
    c->interval = 0;

    c->icount = 0;
    c->requests = 0;
    c->responses = 0;
    c->timeouts = 0;
    c->crcerrors = 0;
    c->exceptions = 0;
    c->lasterror = 0;
    c->lastcrc = 0;
    c->firstrun = 0;
    c->userdata = NULL;
    c->userdata_free = NULL;
    c->next = NULL;
};

/********************/
/* Public Functions */
/********************/


/* Allocates and initializes a master modbus command.  Returns the pointer
 * to the new command or NULL on failure. */
mb_cmd *
mb_new_cmd(mb_port *port)
{
    int result;
    
    mb_cmd *mcmd;
    mcmd = (mb_cmd *)malloc(sizeof(mb_cmd));
    if(mcmd != NULL) {
        initcmd(mcmd);
        result = add_cmd(port, mcmd);
        if(result) {
            free(mcmd);
            return NULL;
        }
    }
    return mcmd;    
}

void
mb_disable_cmd(mb_cmd *cmd) {
    cmd->enable = 0;
}

void
mb_enable_cmd(mb_cmd *cmd) {
    cmd->enable = 1;
}

/*********************/
/* Utility Functions */
/*********************/

void
destroy_cmd(mb_cmd *cmd) {
    
/* This frees the userdata if it is set.  If there is
 * a userdata_free callback assigned it will call that
 * otherwise it just calls free */    
    if(cmd->userdata != NULL) {
        if(cmd->userdata_free != NULL) {
            userdata_free(cmd);
        } else {
            free(cmd);
        }
    }
    free(cmd);
}

 