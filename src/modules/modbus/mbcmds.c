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

#include "modbus.h"


/* Sets the command values to some defaults */
static void
initcmd(struct mb_cmd *c)
{
    c->enable = 1;
    c->mode = 0;
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
    bzero(&c->data_h, sizeof(tag_handle));
    c->next = NULL;
};

/********************/
/* Public Functions */
/********************/


/* Allocates and initializes a master modbus command.  Returns the pointer
 * to the new command or NULL on failure.  If 'port' is not NULL then the
 * new command is added to the port.  The mb_destroy_port() function will
 * take care of deallocating this command in that instance.  Otherwise the
 * caller will have to worry about deallocation using the mb_destroy_cmd()
 * function */
mb_cmd *
mb_new_cmd(mb_port *port)
{
    int result;
    mb_cmd *mcmd;

    mcmd = (mb_cmd *)malloc(sizeof(mb_cmd));
    if(mcmd != NULL) {
        initcmd(mcmd);
        if(port != NULL) {
            result = add_cmd(port, mcmd);
            if(result) {
                free(mcmd);
                return NULL;
            }
        }
    }
    return mcmd;
}

void
mb_destroy_cmd(mb_cmd *cmd) {
    if(cmd->data != NULL) {
        free(cmd->data);
    }
    free(cmd);
}


/* This function checks the arguments and sets up the command passed in cmd.  It also calculates
 * the size of the data in bytes that this command will need so the oepndax tag can be referenced
 * properly. */
int
mb_set_command(mb_cmd *cmd, uint8_t node, uint8_t function, uint16_t reg, uint16_t length)
{
    uint8_t *newdata;
    int newsize = 0;

    if(node < 1 || node > 247) {
        return MB_ERR_BAD_ARG;
    }
    cmd->node = node;
    switch(function) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 15:
        case 16:
            cmd->function = function;
            break;
        default:
            return MB_ERR_FUNCTION;
    }
    cmd->m_register = reg;
    if(function == 1 || function == 2 || function == 15) {
        if(length < 1 || length > 2000) {
            return MB_ERR_BAD_ARG;
        }
    } else if(function == 3 || function == 4 || function == 16) {
        if(length < 1 || length > 125) {
            return MB_ERR_BAD_ARG;
        }
    }
    cmd->length = length;
    switch(function) {
        case 1:
        case 2:
        case 15:
            /* These are multiple bit function codes */
            newsize = (length - 1) / 8 +1;
            break;
        case 3:
        case 4:
        case 16:
            /* These are multiple word function codes */
            newsize = length * 2;
            break;
        case 5:
            /* Single bit function code */
            newsize = 1;
            break;
        case 6:
            /* Single word function code */
            newsize = 2;
            break;
    }
    newdata = realloc(cmd->data, newsize);
    if(newdata == NULL) {
        return MB_ERR_ALLOC;
    } else {
        cmd->data = newdata;
        cmd->datasize = newsize;
    }
    return 0;
}

int
mb_set_interval(mb_cmd *cmd, int interval)
{
    cmd->interval = interval;
    return 0;
}


void
mb_set_mode(mb_cmd *cmd, unsigned char mode)
{
    if(mode == MB_ONCHANGE)
        cmd->mode = MB_ONCHANGE;
    else
        cmd->mode = MB_CONTINUOUS;
}


/* Returns true (1) if the function code for cmd will read from the node */
int
mb_is_read_cmd(mb_cmd *cmd)
{
    switch(cmd->function) {
        case 1:
        case 2:
        case 3:
        case 4:
            return 1;
        default:
            return 0;
    }

}

/* Returns true (1) if the function code for cmd will write to the node */
int
mb_is_write_cmd(mb_cmd *cmd)
{
    switch(cmd->function) {
        case 5:
        case 6:
        case 15:
        case 16:
            return 1;
        default:
            return 0;
    }

}
