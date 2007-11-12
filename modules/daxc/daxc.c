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

void runcmd(char *inst);
char *rl_gets(const char *prompt);
void quit_signal(int sig);
static void getout(int exitstatus);
static void parsecommandline(int argc, char *argv[]);

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
    
 /* Set's the error level and assigns the message callbacks */
    /* TODO: Should be a command line option -v n */
    dax_set_level(10);
    
    /*TODO: These cause warnings because of the function types 
            There has got to be a better way of handling error and debug messages */
    dax_set_debug(&xnotice);
    dax_set_error(&xerror);
    
 /* Check for OpenDAX and register the module */
    if( dax_mod_register("daxc") ) {
        xfatal("Unable to find OpenDAX");
    }
    
    parsecommandline(argc, argv);

    
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
 -X for execute command
 -f for read commands from file
 -v verbosity
 -V print version and bail */
static void parsecommandline(int argc, char *argv[])  {
    char c;
    
    static struct option options[] = {
        {"execute",required_argument, NULL, 'x'},
        {"file",required_argument, NULL, 'f'},
        {"version",no_argument, NULL, 'V'},
        {"verbose",optional_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };
    
    /* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "C:VvD",options, NULL)) != -1) {
        switch (c) {
            case 'V':
                printf("OpenDAX Command Line Client Module Version %s\n",VERSION);
                break;
            case 'v':
                //--config.verbose = strtol(optarg, "NULL", 10);
                break;
            case 'x': 
                //--config.daemonize=1;
                break;
            case 'f': 
                //--config.daemonize=1;
                break;
            case '?':
                printf("Got the big ?\n");
                break;
            case -1:
            case 0:
                break;
            default:
                printf ("?? getopt returned character code 0%o ??\n", c);
        } /* End Switch */
    } /* End While */           
}




/* Main Loop.  Get input produce output */
void runcmd(char *instr) {
    char *tok;
    
    /* get the command */
    tok = strtok(instr, " ");
    /* test the command in sequential if's once found call function and continue */
    if( !strcasecmp(tok,"dax")) {
        printf("Haven't done 'dax' yet\n");
    
    } else if( !strcasecmp(tok,"tag")) {
        tok = strtok(NULL, " ");
        if(tok == NULL) fprintf(stderr,"ERROR: Missing Subcommand\n");
        else if( !strcasecmp(tok, "list")) tag_list();
        else if( !strcasecmp(tok, "set")) tag_set(instr);
        else if( !strcasecmp(tok, "get")) tag_get(instr);
    
    } else if( !strcasecmp(tok,"mod")) {
        printf("Haven't done 'mod' yet!\n");
        
    } else if( !strcasecmp(tok,"db")) {
        printf("Haven't done 'db' yet!\n");
    
    } else if( !strcasecmp(tok,"msg")) {
        printf("Haven't done 'msg' yet!\n");    
    
    } else if( !strcasecmp(tok, "help")) {
        printf("Hehehehe, Yea right!\n");
        printf(" Try TAG LIST, TAG SET or TAG GET\n");
    
    } else if( !strcasecmp(tok,"exit")) {
        getout(0);
    
    } else {
        printf("Unknown Command - %s\n",tok);
    }
}



/* TODO: Need to conditionally compile this function to
   either use libreadline or just use gets */
/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
char *rl_gets(const char *prompt) {
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
void quit_signal(int sig) {
    xerror("Quitting due to signal %d",sig);
    getout(-1);
}

static void getout(int exitstatus) {
    dax_mod_unregister();
    exit(exitstatus);
}
