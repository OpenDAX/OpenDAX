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

#include <daxc.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>

int runcmd(char *inst);
char *rl_gets(const char *prompt);
void quit_signal(int sig);
static void getout(int exitstatus);
static void parsecommandline(int argc, char *argv[]);

static int verbosity = 0;
static char *command = NULL;
static char *filename = NULL;

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    char *instr;
    
 /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    

    parsecommandline(argc, argv);

    /* Set's the error level and assigns the message callbacks */
    dax_set_verbosity(verbosity);
        
 /* Check for OpenDAX and register the module */
    if( dax_mod_register("daxc") ) {
        dax_fatal("Unable to find OpenDAX");
    }
    
    if(command) {
        if(runcmd(command)) getout(1);
    }
    
    if(filename) {
        printf("Gonna try to run file %s\n", filename);
    }
    
    if(filename || command) getout(0);
    
 /* At this point we are in interactive mode.  We call the readline
    function repeatedly and then send the input to runcmd */
    while(1) {
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

/* TODO: Finish this function 
 -x for execute command
 -f for read commands from file
 -v verbosity
 -V print version and bail
 -h for help*/
static void parsecommandline(int argc, char *argv[])  {
    char c;
    
    static struct option options[] = {
        {"execute", required_argument, NULL, 'x'},
        {"file", required_argument, NULL, 'f'},
        {"version", no_argument, NULL, 'V'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };
    
    /* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "f:Vvhx:",options, NULL)) != -1) {
        switch (c) {
            case 'V':
                printf("OpenDAX Command Line Client Module Version %s\n", VERSION);
                getout(0);
                break;
            case 'v':
                verbosity++;
                break;
            case 'x': 
                command = strdup(optarg);
                break;
            case 'f': 
                filename = strdup(optarg);
                break;
            case 'h': 
                printf("Printing Help\n");
                break;
            case '?':
                //--printf("Got the big ?\n");
                break;
            case -1:
            case 0:
                break;
            default:
                printf ("?? getopt returned character code 0%o ??\n", c);
        } /* End Switch */
    } /* End While */           
}


/* Takes the execution string tokenizes it and runs the appropriate function */
int
runcmd(char *instr)
{
    int tcount = 0, n, result;
    char *last, *temp, *tok;
    char **tokens;
    
    /* Store the string to protect if from strtok_r() */
    temp = strdup(instr);
    if(temp == NULL) {
        fprintf(stderr, "ERROR: Unable to allocate memory\n");
        return -1;
    }
    /* First we Count the Tokens */
    tok = strtok_r(temp, " ", &last);
    if(tok) {
        tcount = 1;
        while( (tok = strtok_r(NULL, " ", &last)) ) {
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
    tokens[0] = strtok_r(temp, " ", &last);
    /* Get all the tokens */
    for(n = 1; n < tcount; n++) {
        tokens[n] = strtok_r(NULL, " ", &last);
    }
    tokens[tcount] = NULL; /* Terminate the array with a NULL */    
    
    
    /* test the command in sequential if's once found call function and continue */
    if( !strcasecmp(tokens[0],"dax")) {
        printf("Haven't done 'dax' yet\n");
    } else if( !strncasecmp(tokens[0], "add", 1)) {
        result = tag_add(&tokens[1]);
    } else if( !strncasecmp(tokens[0], "list", 4)) {
        result = tag_list(&tokens[1]);
    } else if( !strncasecmp(tokens[0], "read", 1)) {
        result = tag_read(&tokens[1]);
    } else if( !strncasecmp(tokens[0], "write", 1)) {
        result = tag_write(&tokens[1], tcount-1);
/* The tag stuff needs to be removed.  The single commands are simpler */
    //} else if( !strcasecmp(tok,"tag")) {
    //    tok = strtok(NULL, " ");
    //    if(tok == NULL) fprintf(stderr,"ERROR: Missing Subcommand\n");
    //    else if( !strcasecmp(tok, "list")) return(tag_list());
    //    else if( !strcasecmp(tok, "set")) return(tag_set());
    //    else if( !strcasecmp(tok, "get")) return(tag_get());
    //    else fprintf(stderr, "ERROR: Unknown Subcommand - %s\n", tok);
    
    } else if( !strncasecmp(tokens[0], "mod", 3)) {
        printf("Haven't done 'mod' yet!\n");
    } else if( !strcasecmp(tokens[0],"db")) {
        if(tokens[1] == NULL) fprintf(stderr, "ERROR: Missing Subcommand\n");
        else if( !strcasecmp(tokens[1], "read")) result = db_read();
    //  else if( !strcasecmp(tokens[1], "readbit")) result = db_read_bit();
        else if( !strcasecmp(tokens[1], "write")) result = db_write();
        else if( !strcasecmp(tokens[1], "format")) result = db_format();
        else fprintf(stderr, "ERROR: Unknown Subcommand - %s\n", tokens[0]);
        
    } else if( !strcasecmp(tokens[0],"msg")) {
        printf("Haven't done 'msg' yet!\n");    
    /* TODO: Really should work on the help command */
    } else if( !strcasecmp(tokens[0], "help")) {
        printf("Hehehehe, Yea right!\n");
        printf(" Try READ, WRITE, LIST, ADD\n");
    
    } else if( !strcasecmp(tokens[0],"exit")) {
        getout(0);
    
    } else {
        printf("Unknown Command - %s\n", tokens[0]);
    }
    
    free(tokens);
    return result;
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
       save it on the history. */
    if (line_read && *line_read)
        add_history(line_read);

    return(line_read);
}

/* Signal handler - mainly jsut cleanup */
void
quit_signal(int sig)
{
    dax_fatal("Quitting due to signal %d", sig);
    getout(-1);
}

static void
getout(int exitstatus)
{
    dax_mod_unregister();
    exit(exitstatus);
}
