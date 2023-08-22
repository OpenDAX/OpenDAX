/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
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
 *  Main header file for the OpenDAX Command Line Client module
 */

#define _GNU_SOURCE
#include <common.h>
#include <opendax.h>
#include <signal.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)     (((a) < 0) ? -(a) : (a))


/* This is the maximum number of events that we can store. */
#define MAX_EVENTS 64
#define LINE_BUFF_SIZE 256

#define DAX_FMT_HEX  0x01
#define DAX_FMT_BIN  0x02
#define DAX_FMT_DEC  0x03
#define DAX_FMT_CHR  0x04

/* TAG commands */
int tag_add(char **tokens);
int tag_del(char **tokens);
int list_tags(char **tokens);
int tag_read(char **tokens);
int tag_write(char **tokens, int tcount);

/* Custom datatype commands */
int cdt_add(char **tokens, int tcount);
int list_types(char **tokens);

/* DB commands (DataBase) */
int db_read_bit(void);
int db_read(char **tokens);
int db_write(void);
int db_format(void);

int event_add(char **tokens, int count);
int event_del(char **tokens);
int event_wait(char **tokens);
int event_poll(void);

int map_add(char **tokens, int count);
int map_del(char **tokens, int count);
int map_get(char **tokens, int count);

int override_add(char **tokens, int count);
int override_del(char **tokens, int count);
int override_get(char **tokens, int count);
int override_set(char **tokens, int count);
int override_clr(char **tokens, int count);

int atomic_op(char **tokens, int count);