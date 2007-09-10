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
 
 *  Source code file for opendax configuration
 *  
 */

/*  TODO: This entire configuration mechanism needs to be changed.  We need a
    a way to make it easy to use configuration files in DAX as well as all the
    modules.  It would help to enforce a common look and feel and it would help
    get modules built faster.  If enough thought is put into the system it'd
    make it relatively easy to build a GUI configuration system too. */


#include <common.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
//#include <termios.h>


#include <common.h>
#include <func.h>
#include <module.h>
#include <options.h>

static void initconfig(void);
static void setdefaults(void);
static void parsecommandline(int, const char**);
static int readconfigfile(void);
static void parsemodule(FILE *);
static void printconfig(void);

struct Config config;

/* This function should be called from main() to configure the program.
   First the defaults are set then the configuration file is parsed then
   the command line is handled.  This gives the command line priority.  */
int dax_configure(int argc, const char *argv[]) {
    initconfig();
    parsecommandline(argc,argv);
    if(readconfigfile()) {
        xerror("Unable to read configuration running with defaults");
    }
    setdefaults();
    //if(config.verbose) printconfig();
    return 0;
}

/* Inititialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {
    config.pidfile=NULL;
    config.tagname=NULL;
    config.configfile=NULL;
    //config.verbose=0;
    config.daemonize=0;
}


/* This function parses the command line options and sets
   the proper members of the configuration structure */
static void parsecommandline(int argc, const char *argv[])  {
    char c;

    static struct option options[] = {
        {"config",required_argument,0,'C'},
        {"version",no_argument, 0, 'V'},
        {"verbose",no_argument,0,'v'},
        {0, 0, 0, 0}
    };
      
/* Get the command line arguments */ 
    while ((c = getopt_long (argc, (char * const *)argv, "C:VvD",options, NULL)) != -1) {
        switch (c) {
        case 'C':
            config.configfile = strdup(optarg);
	        break;
        case 'V':
		    printf("%s Version %s\n",PACKAGE,VERSION);
	        break;
	    case 'v':
            setverbosity(10);
            //config.verbose = 1 ;
	        break;
        case 'D': 
            config.daemonize = 1;
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

/* This function sets the defaults for the configuration if the
   commandline or the config file set them. */
static void setdefaults(void) {
    char s[]=DEFAULT_PID;
   
    if(!config.pidfile) {
        config.pidfile = strdup(s);
    }
    //--if(!config.ipaddress[0])
    //--    strcpy(config.ipaddress,"0.0.0.0");
    //--if(!config.port)
    //--    config.port=DEFAULT_PORT;
}


/* This function reads the configuration file.  It does not
 * override values that were entered on the commandline. */
static int readconfigfile(void)  {
    FILE *fd=NULL;
	int length,count;
	char *result,*find;
	char string[MAX_LINE_LENGTH];
	char token[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
    
	/* if the name of the configuration file is in the config.configfile
	 * member it will open that file.  If not the config file name is 
	 * ETC_DIR/opendax.conf */
	if(!config.configfile) {
        length=strlen(ETC_DIR) + strlen("/opendax.conf") +1;
		config.configfile=(char *)malloc(sizeof(char) * length);
		if(config.configfile) 
		    sprintf(config.configfile,"%s%s",ETC_DIR,"/opendax.conf");
	}
	fd=fopen(config.configfile,"r");
	if(!fd) {
        syslog(LOG_ERR,"Unable to open configuration file %s",config.configfile);
        return(-1);
	}
	/* parse the strings in the file */
	while( (result=fgets(string, MAX_LINE_LENGTH-1, fd)) )  {
        if(strlen(string) >= MAX_LINE_LENGTH) {
            xnotice("Extra long line found in config file");
		}
		/* Find the '#' in the string and place the \0 in it's place to \
		 * truncate the string and remove the comment */
		find = strpbrk(string,"#");
		if(find) *find = '\0';
        count = sscanf(string, "%s %s",(char *)&token, (char *)&value);
		if(count == 2) { /* if sscanf() found both strings */
            if(!strcasecmp(token,"Tablesize")) {
                if(!config.tablesize) {
                    config.tablesize=strtol(value,NULL,10);
                }
            } else if(!strcasecmp(token,"PIDFile")) {
			     if(!config.pidfile) {
				     config.pidfile=strdup(value);
                 }
		    /* TODO: Should be able to put in a number */
            } else if(!strcasecmp(token, "Verbose")) {
                if(!strcasecmp(value, "yes"))
                    //config.verbose = 10;
                    setverbosity(10);
            } else if(!strcasecmp(token,"Daemonize")) {
                if(!strcasecmp(value,"yes"))
                    config.daemonize = 1;
            }
		} else if(strcasestr(string,"<module>")) {
            parsemodule(fd);
        }
    }

    fclose(fd);
    return 0;
}

/* When the main parsing routine finds a <port> or <defaultport> it
   calls this function to get the port configuration.  def should be
   1 if this is the configuration for the default port. */
static void parsemodule(FILE *fd) {
    int count;
    unsigned int flags;
    char *name,*path,*arglist;
    char *result,*find;
	char string[MAX_LINE_LENGTH];
	char token[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
	
    if(fd == NULL) { /* Used to reset the module configuration */
        fd = NULL;
    }
    
    name = NULL;
    path = NULL;
    arglist = NULL;
    xlog(10,"Found a module to parse");
    
    while( (result = fgets(string,MAX_LINE_LENGTH-1,fd)) )  {
        if(strlen(string) >= MAX_LINE_LENGTH) {
            xnotice("Extra long line found in config file");
		}
		/* Find the '#' in the string and place the \0 in it's place to \
        * truncate the string and remove the comment */
		find = strpbrk(string,"#");
		if(find) *find = '\0';
        count = sscanf(string,"%s %s",(char *)&token,(char *)&value);
		if(count == 2) { /* if sscanf() found both strings */
            if(!strcasecmp(token,"name")) {
                name = strdup(value);
            } else if(!strcasecmp(token,"path")) {
                path = strdup(value);
            /* TODO: All this is just goofy.  I need to read the whole path from the line
                that'll come with the rewrite of the config file reading code that's comin */
            } else if(!strcasecmp(token,"arg")) {
                if(arglist == NULL) {
                    arglist = strdup(value);
                } else {
                    arglist = xrealloc(arglist, strlen(arglist) + strlen(value) + 1);
                    arglist = strcat(arglist, " ");
                    arglist = strcat(arglist, value);
                }
            } else if(!strcasecmp(token,"restart")) {
                if(!strcasecmp(value,"no")) {
                    flags |= MFLAG_NORESTART;
                }
            } else if(!strcasecmp(token,"openpipes")) {
                if(!strcasecmp(value,"yes")) {
                    flags |= MFLAG_OPENPIPES;
                }
            }
        } else if(strcasestr(string,"</module>")) {
            module_add(name, path, arglist, flags);
            return;
        }
    }
    xerror("No </module> to end subsection\n");
}


/* TODO: Really should print out more than this*/
static void printconfig(void) {

}
