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
 *  Main source code file for the OpenDAX Command Line Client  module
 */

#include "daxc.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>

#define HISTORY_FILE ".dax_history"

int runcmd(char *inst);
int runfile(char *filename);
char *rl_gets(const char *prompt);
void quit_signal(int sig);
static void getout(int exitstatus);

static int _quitsignal = 0;
static char *history_file = NULL;
dax_state *ds;
int quiet_mode;


/* strtok clone that deals with " delimited strings and
 * has a fixed delimiter of space */
static char *
_str_daxtok(char *input) {
    static char *token = NULL;
    char *start;
    char lastchar;
    int inQuote = 0;

    if(input != NULL) {
        token = input;
        start = input;
        lastchar = '\0';
    } else {
        start = token;
        if(*token =='\0') {
            start = NULL;
        }
    }
    while(*token != '\0') {
        if(inQuote) {
            if(*token == '\"' && lastchar != '\\') {
                inQuote = 0;
            }
            lastchar = *token;
            token++;
        } else {
            if(*token == '\"') {
                inQuote = 1;
                lastchar = *token;
            } else if(*token == ' ') {
                *token = '\0';
                token++;
                break;
            }
            token++;
        }
    }
    return start;
}


/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int flags, result = 0;
    char *instr, *command, *filename;
    char *home_dir;

 /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    ds = dax_init("daxc");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }

    dax_init_config(ds, "daxc");
    flags = CFG_CMDLINE | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "execute","execute", 'x', flags, NULL);
    flags = CFG_CMDLINE;
    result += dax_add_attribute(ds, "quiet","quiet", 'q', flags, NULL);
    result += dax_add_attribute(ds, "interactive","interactive", 'i', flags, NULL);

    dax_configure(ds, argc, argv, CFG_CMDLINE);

 /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_fatal(ds, "Unable to find OpenDAX");
        getout(_quitsignal);
    }
    /* No setup work to do here.  We'll go straight to running */
    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);

    if(dax_get_attr(ds, "quiet")) {
        quiet_mode = 1;
    }

    command = dax_get_attr(ds, "execute");

    if(command) {
        runcmd(command);
    }

    filename = argv[optind];
    if(filename) {
        runfile(filename);
    }
    if((filename || command) && !dax_get_attr(ds, "interactive")) {
        getout(0);
    }

/* At this point we are in interactive mode.  We first read the
 * readline history file and then start an infinite loop where
 * We call the readline function repeatedly and then send the
 * input to runcmd */
    quiet_mode = 0; /* We turn quiet mode off for interactive mode */

    home_dir = getenv("HOME");
    if(home_dir != NULL) {
        history_file = malloc(strlen(home_dir) + strlen(HISTORY_FILE) + 2);
        if(history_file != NULL) {
            using_history();
            sprintf(history_file, "%s/%s", home_dir, HISTORY_FILE);
            read_history(history_file);
            /* TODO: Add to configuration */
            stifle_history(100);
        }
    }
    while(1) {
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }

        instr = rl_gets("dax>");

        if( instr == NULL ) {
            /* End of File */
            getout(0);
        }
        if( instr[0] != '\0' ) {
            runcmd(instr); /* Off we go */
        }
    }

 /* This is just to make the compiler happy */
    return(0);
}

int
get_help(char **tokens) {
    if(tokens[0] == NULL) {
        printf("Usage: command <args>...\n");
        printf("Commands: \n ADD\n DEL\n LIST\n READ\n WRITE\n WAIT\n POLL\n EXIT\n");
        printf("For more information type: help <command>\n");
    } else if(!strncasecmp(tokens[0], "add", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: ADD [TAG|TYPE|MAP|EVENT] <args>...\n\n");
            printf("Add the given type of object\n");
        }
    } else if(!strncasecmp(tokens[0], "del", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: DEL [TAG|TYPE|MAP|EVENT]\n\n");
        } else if(!strncasecmp(tokens[1], "tag", 3)) {
            printf("Usage: DEL TAG tagname\n\n");
            printf("THIS IS NOT CURRENTLY IMPLEMENTED\n");
        } else if(!strncasecmp(tokens[1], "type", 4)) {
            printf("Usage: DEL TYPE typename\n\n");
            printf("THIS IS NOT CURRENTLY IMPLEMENTED\n");
        } else if(!strncasecmp(tokens[1], "map", 3)) {
            printf("Usage: DEL MAP index\n\n");
            printf("Deletes the map given by 'index'\n");
        } else if(!strncasecmp(tokens[1], "event", 5)) {
            printf("Usage: DEL EVENT index\n\n");
            printf("Deletes the event given by 'index'\n");
        }
    } else if(!strncasecmp(tokens[0], "list", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: LIST subcommand <args>...\n");
        }
    } else if(!strncasecmp(tokens[0], "read", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: READ tag\n");
        }
    } else if(!strncasecmp(tokens[0], "write", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: WRITE tag values...\n");
        }
    } else if(!strncasecmp(tokens[0], "wait", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: WAIT tags...\n");
        }
    } else if(!strncasecmp(tokens[0], "poll", 3)) {
        if(tokens[1] == NULL) {
            printf("Usage: POLL tags...\n");
        }
    } else if(!strncasecmp(tokens[0], "exit", 3)) {
        printf("Usage: EXIT\n\n");
        printf("Will cause the program to exit\n");
    }
    return 0;
}

/* Takes the execution string tokenizes it and runs the appropriate function */
int
runcmd(char *instr)
{
    int tcount = 0, n, result = 0;
    char *temp, *tok;
    char **tokens;

    if(instr[0] == '\0') return 0; /* ignore blank lines */
    if(instr[0] == '#') return 0; /* ignore comments */
    /* Store the string to protect if from strtok_r() */
    temp = strdup(instr);
    if(temp == NULL) {
        fprintf(stderr, "ERROR: Unable to allocate memory\n");
        return -1;
    }
    /* First we Count the Tokens */
    tok = _str_daxtok(temp);
    if(tok) {
        tcount = 1;
        while( (tok = _str_daxtok(NULL)) ) {
            tcount++;
        }
    }

    /* Now that we know how many tokens we can allocate the array
     * We get one more than needed so that we can add the NULL */
    tokens = malloc(sizeof(char *) * tcount+1);

    if(tokens == NULL) {
        fprintf(stderr, "ERROR: Unable to allocate memory\n");
        free(temp);
        return -1;
    }

    /* Since strtok_r() will mess up our string we need to copy it again */
    strcpy(temp, instr);
    tokens[0] = _str_daxtok(temp);
    /* Get all the tokens */
    for(n = 1; n < tcount; n++) {
        tokens[n] = _str_daxtok(NULL);
    }
    tokens[tcount] = NULL; /* Terminate the array with a NULL */

    /* test the command in sequential if's once found call function and continue */
    if( !strcasecmp(tokens[0],"dax")) {
        printf("Haven't done 'dax' yet\n");
    } else if( !strncasecmp(tokens[0], "add", 1)) {
        if(tokens[1] == NULL) {
            result = tag_add(&tokens[1]);
        } else if(!strncasecmp(tokens[1], "tag", 3)) {
            result = tag_add(&tokens[2]);
        } else if( !strncasecmp(tokens[1], "type", 4)) {
            result = cdt_add(&tokens[2], tcount -2);
        } else if( !strncasecmp(tokens[1], "map", 3)) {
            result = map_add(&tokens[2], tcount -2);
        } else if( !strncasecmp(tokens[1], "event", 5)) {
            result = event_add(&tokens[2], tcount - 2);
        }
    } else if( !strncasecmp(tokens[0], "del", 3)) {
        if(tokens[1] == NULL) {
            fprintf(stderr, "ERROR: DEL What?\n");
        } else if( !strncasecmp(tokens[1], "tag", 3)) {
            result = tag_del(&tokens[2]);
        } else if( !strncasecmp(tokens[1], "map", 3)) {
            result = map_del(&tokens[2], tcount-2);
        } else if( !strncasecmp(tokens[1], "event", 5)) {
            event_del(&tokens[2]);
        }

    } else if( !strncasecmp(tokens[0], "list", 4)) {
        if(tokens[1] == NULL) {
            result = list_tags(NULL);
        } else if(!strncasecmp(tokens[1], "tag", 3)) {
            result = list_tags(&tokens[2]);
        } else if(!strncasecmp(tokens[1], "type", 4)) {
            result = list_types(&tokens[2]);
        } else if(!strncasecmp(tokens[1], "map", 3)) {
            result = map_get(&tokens[2], tcount-2);
        } else {
            fprintf(stderr, "ERROR: Unknown list parameter %s\n", tokens[1]);
        }

    } else if( !strncasecmp(tokens[0], "read", 1)) {
        result = tag_read(&tokens[1]);
    } else if( !strncasecmp(tokens[0], "write", 1)) {
        result = tag_write(&tokens[1], tcount-1);
    } else if( !strncasecmp(tokens[0], "wait", 4)) {
        result = event_wait(&tokens[1]);
    } else if( !strncasecmp(tokens[0], "poll", 4)) {
        event_poll();

    } else if( !strcasecmp(tokens[0], "help")) {
        get_help(&tokens[1]);

    } else if( !strcasecmp(tokens[0],"exit")) {
        getout(0);

    } else {
        fprintf(stderr, "Unknown Command - %s\n", tokens[0]);
    }

    free(tokens);
    return result;
}

int
runfile(char *filename)
{
    char string[LINE_BUFF_SIZE];
    FILE *file;
    int len;

    file = fopen(filename, "r");
    if(file == NULL) {
        fprintf(stderr, "ERROR: Unable to open file - %s\n", filename);
        return -1;
    }
    while(!feof(file)) {
        fgets(string, LINE_BUFF_SIZE, file);
        len = strlen(string);
        if(string[len-1] == '\n') string[len-1] = '\0';
        runcmd(string);
    }
    return 0;
}

/* TODO: Need to conditionally compile this function to
   either use libreadline or just use gets */
/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
char *
rl_gets(const char *prompt)
{
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
     * save it on the history. */
    if (line_read && *line_read)
        add_history(line_read);

    return(line_read);
}

/* Signal handler - mainly jsut cleanup */
void
quit_signal(int sig)
{
    _quitsignal = sig;
}

static void
getout(int exitstatus)
{
    if(history_file != NULL) {
        write_history(history_file);
        free(history_file);
    }
    dax_disconnect(ds);
    exit(exitstatus);
}
