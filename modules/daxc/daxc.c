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
 *  Main source code file for the OpenDAX Commadn Line Client  module
 */

#include <common.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

char *rl_gets(const char *prompt);

int main(int argc,char *argv[]) {
    char *instr;
    while(1) {
        instr = rl_gets("dax>");
        if( !strcmp(instr,"exit")) exit(0);
        printf("You entered %s\n",instr);
    }
}

/* TODO: Need to conditionally compile this function to
   either use libreadline or just use gets */
/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
char * rl_gets(const char *prompt) {
    static char *line_read = (char *)NULL;
    
    /* If the buffer has already been allocated,
       return the memory to the free pool. */
    if (line_read) {
        free(line_read);
        line_read = (char *)NULL;
    }

    /* Get a line from the user. */
       line_read = readline(prompt);

    /* If the line has any text in it,
       save it on the history. */
    if (line_read && *line_read)
        add_history(line_read);

    return(line_read);
}
