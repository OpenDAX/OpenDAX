/* Mbd - Modbus (tm) Communications Program
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <termios.h>

#include <common.h>
//#include "func.h"
#include "options.h"
#include "modbus.h"

static void initconfig(void);
static void setdefaults(void);
static void parsecommandline(int, const char**);
static int readconfigfile(void);
static void parseport(FILE *,unsigned char);
static void parsecommand(FILE *,struct mb_port *);
static void printconfig(void);

struct Config config;

/* This function should be called from main() to configure the program.
   First the defaults are set then the configuration file is parsed then
   the command line is handled.  This gives the command line priority.  */
int modbus_configure(int argc, const char *argv[]) {
    initconfig();
    parsecommandline(argc,argv);
    if(readconfigfile()) {
        syslog(LOG_ERR,"Unable to read configuration running with defaults");
    }
    setdefaults();
    if(config.verbose) printconfig();
    return 0;
}

/* Inititialize the configuration to NULL or 0 for cleanliness */
static void initconfig(void) {
    config.pidfile=NULL;
    config.tagname=NULL;
    config.configfile=NULL;
    config.tablesize=0;
    config.verbose=0;
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
            config.configfile=(char *)malloc(sizeof(char)*(strlen(optarg)+1));
	        if(config.configfile) strcpy(config.configfile,optarg);
	        break;
        case 'V':
		    printf("mbd Version %s\n",VERSION);
	        break;
	    case 'v':
            config.verbose=1;
	        break;
        case 'D': 
            config.daemonize=1;
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
        config.pidfile=(char *)malloc(sizeof(char)*(strlen(s)+1));
        if(config.pidfile) strcpy(config.pidfile,s);
    }
    if(!config.tagname) {
        config.tagname = strdup(DEFAULT_TAGNAME);
    }
    //--if(!config.ipaddress[0])
    //--    strcpy(config.ipaddress,"0.0.0.0");
    //--if(!config.port)
    //--    config.port=DEFAULT_PORT;
    if(!config.tablesize)
        config.tablesize=DEFAULT_TABLE_SIZE;
}


/* This function reads the configuration file.  It does not
 * override values that were entered on the commandline. */
static int readconfigfile(void)  {
    //struct mbd_cmd tempcmd;
    FILE *fd=NULL;
	int length,count;
	char *result,*find;
	char string[MAX_LINE_LENGTH];
	char token[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
    
	/* if the name of the configuration file is in the config.configfile
	 * member it will open that file.  If not the config file name is 
	 * ETC_DIR/mbd.conf */
	if(!config.configfile) {
        length=strlen(ETC_DIR) + strlen("/modbus.conf") +1;
		config.configfile=(char *)malloc(sizeof(char) * length);
		if(config.configfile) 
		    sprintf(config.configfile,"%s%s",ETC_DIR,"/modbus.conf");
	}
	fd=fopen(config.configfile,"r");
	if(!fd) {
        syslog(LOG_ERR,"Unable to open configuration file %s",config.configfile);
        return(-1);
	}
	/* parse the strings in the file */
	while( (result=fgets(string,MAX_LINE_LENGTH-1,fd)) )  {
        if(strlen(string)>=MAX_LINE_LENGTH) {
            syslog(LOG_NOTICE,"Extra long line found in config file");
		}
		/* Find the '#' in the string and place the \0 in it's place to \
		 * truncate the string and remove the comment */
		find=strpbrk(string,"#");
		if(find) *find='\0';
                count=sscanf(string,"%s %s",(char *)&token,(char *)&value);
		if(count==2) { /* if sscanf() found both strings */
            if(!strcasecmp(token,"Tablesize")) {
                if(!config.tablesize) {
                    config.tablesize=strtol(value,NULL,10);
                }
            //--} else if(!strcasecmp(token,"IPAddress")) {
            //--    if(! config.ipaddress[0]) {
            //--        strncpy(config.ipaddress,value,15);
			//--	    config.ipaddress[15]='\0';
			//--    }
            //--} else if(!strcasecmp(token,"Port")) {
			//--    if(! config.port) {
            //--        config.port=strtol(value,NULL,10);
			//--    }
		    } else if(!strcasecmp(token,"PIDFile")) {
			     if(!config.pidfile) {
				     config.pidfile=strdup(value);
                     //--config.pidfile=(char *)malloc(sizeof(char)*(strlen(value)+1));
				     //--if(config.pidfile) strcpy(config.pidfile,value);
			     }
		    } else if(!strcasecmp(token,"Tagname")) {
                if(!config.pidfile) {
                    config.pidfile=strdup(value);
                }
            } else if(!strcasecmp(token,"Verbose")) {
                if(!strcasecmp(value,"yes"))
                    config.verbose=1;
            } else if(!strcasecmp(token,"Daemonize")) {
                if(!strcasecmp(value,"yes"))
                    config.daemonize=1;
            }
		} else if(strcasestr(string,"<port>")) {
            parseport(fd,0);
        } else if(strcasestr(string,"<defaultport>")) {
            parseport(fd,1);
        }
    }

    fclose(fd);
    return 0;
}

/* When the main parsing routine finds a <port> or <defaultport> it
   calls this function to get the port configuration.  def should be
   1 if this is the configuration for the default port. */
static void parseport(FILE *fd,unsigned char def) {
    static struct mb_port dp;
    struct mb_port *p;
    int count;
	char *result,*find;
	char string[MAX_LINE_LENGTH];
	char token[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
	
    if(fd==NULL) { /* Used to reset the port configuration */
        config.portcount=0;
        /* Get a new initialized port and copy it to dp (the default port config) */
        p=mb_new_port();
        memcpy(&dp,p,sizeof(struct mb_port));
        free(p);
    }
    if(def==1) {
        p=&dp; /* If this is the default port config */
    } else {
        if(config.portcount==MAX_PORTS) {
            syslog(LOG_ERR,"Number of ports is limited to %d",MAX_PORTS);
            return;
        }
        p=&(config.ports[config.portcount++]);
        /* copy the default port to 'p' */
        memcpy(p,&dp,sizeof(struct mb_port));
    }
    
    while( (result=fgets(string,MAX_LINE_LENGTH-1,fd)) )  {
        if(strlen(string)>=MAX_LINE_LENGTH) {
            syslog(LOG_NOTICE,"Extra long line found in config file");
		}
		/* Find the '#' in the string and place the \0 in it's place to \
        * truncate the string and remove the comment */
		find=strpbrk(string,"#");
		if(find) *find='\0';
                count=sscanf(string,"%s %s",(char *)&token,(char *)&value);
		if(count==2) { /* if sscanf() found both strings */
            if(!strcasecmp(token,"enable")) {
                if(!strcasecmp(value,"0"))  p->enable=0;
                else                        p->enable=1;
            } else if(!strcasecmp(token,"name")) {
                p->name=(char *)malloc(strlen(value)+1);
                strcpy(p->name,value);
            } else if(!strcasecmp(token,"ipaddress")) {
                if(strlen(value)>15) value[15]='\0';
                strcpy(p->ipaddress,value);
            } else if(!strcasecmp(token,"bindport")) {
                p->bindport=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"type")) {
                if(!strcasecmp(value,"master")) {
                    p->type=MASTER;
                }else if(!strcasecmp(value,"slave")) {
                    p->type=SLAVE;
                } else {
                    if(config.verbose)
                        printf("Unknown port type - %s\n",value);
                }
            } else if(!strcasecmp(token,"device")) {
                p->device=(char *)malloc(strlen(value)+1);
                strcpy(p->device,value);
            } else if(!strcasecmp(token,"protocol")) {
                if(!strcasecmp(value,"RTU")) {
                    p->protocol=RTU;
                } else if(!strcasecmp(value,"ASCII")) {
                    p->protocol=ASCII;
                } else if(!strcasecmp(value,"TCP")) {
                    p->protocol=TCP;
                } else if(!strcasecmp(value,"LAN")) {
                    p->protocol=LAN;
                } else {
                    if(config.verbose)
                        printf("Unknown port protocol - %s\n",value);
                }
            } else if(!strcasecmp(token,"slaveid")) {
                p->slaveid=strtoul(value,NULL,10);
                if((p->slaveid < 1) || (p->slaveid > 247)) {
                    if(config.verbose)
                        printf("Invalid Slave ID - %d\n",p->slaveid);
                    p->slaveid=0;
                }
            //TODO: Need to write a routine to sanity check the configuration
            //      but it may need to be after everything is read because some
            //      stuff depends on other stuff and it'll make this routine really ugly
            } else if(!strcasecmp(token,"holdreg")) {
                p->holdreg=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"holdsize")) {
                p->holdsize=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"inputreg")) {
                p->inputreg=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"inputsize")) {
                p->inputsize=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"coilreg")) {
                p->coilreg=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"coilsize")) {
                p->coilsize=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"baudrate")) {
                p->baudrate=strtol(value,NULL,10);
                if(getbaudrate(p->baudrate)==0) {
                    if(config.verbose)
                        printf("Unknown Baudrate - %s\n",value);
                    p->baudrate=9600;
                }
            } else if(!strcasecmp(token,"databits")) {
                p->databits=strtoul(value,NULL,10);
                if(p->databits < 7 && p->databits > 8) {
                    if(config.verbose)
                        printf("Unknown databits - %d\n",p->databits);
                    p->databits=8;
                }
            } else if(!strcasecmp(token,"stopbits")) {
                p->stopbits=strtoul(value,NULL,10);
                if(p->stopbits != 1 && p->stopbits !=2) {
                    if(config.verbose)
                        printf("Wrong stopbits - %d\n",p->stopbits);
                    p->stopbits=1;
                }
            } else if(!strcasecmp(token,"parity")) {
                if(!strcasecmp(value,"none"))
                    p->parity=NONE;
                if(!strcasecmp(value,"even"))
                    p->parity=EVEN;
                if(!strcasecmp(value,"odd"))
                    p->parity=ODD;
            } else if(!strcasecmp(token,"scanrate")) {
                p->scanrate=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"timeout")) {
                p->timeout=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"wait")) {
                p->wait=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"delay")) {
                p->delay=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"retries")) {
                p->retries=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"maxtimeouts")) {
               p->maxtimeouts=strtoul(value,NULL,10);
            } else {
                if(config.verbose)
                    printf("Unknown Port Parameter - %s\n",token);
            }
        } else if(strcasestr(string,"</port>") || strcasestr(string,"</defaultport>")) {
            return;
        } else if(strcasestr(string,"<command>")) {
            parsecommand(fd,p);
        }
    }
    if(config.verbose) {
        printf("No </port> to end port configuration\n");
    }       
}


static void parsecommand(FILE *fd,struct mb_port *p) {
    struct mb_cmd *c;
    int count;
	char *result,*find;
	char string[MAX_LINE_LENGTH];
	char token[MAX_LINE_LENGTH];
	char value[MAX_LINE_LENGTH];
	
    /* Allocate the new command and add it to the port */
    c=mb_new_cmd();
    mb_add_cmd(p,c);
    if(c==NULL) {
        syslog(LOG_ERR,"Can't allocate a new command");
        exit(-1);
    }
    
    while( (result=fgets(string,MAX_LINE_LENGTH-1,fd)) )  {
        if(strlen(string)>=MAX_LINE_LENGTH) {
            syslog(LOG_NOTICE,"Extra long line found in config file");
        }
        /* Find the '#' in the string and place the \0 in it's place to \
        * truncate the string and remove the comment */
        find=strpbrk(string,"#");
        if(find) *find='\0';
        count=sscanf(string,"%s %s",(char *)&token,(char *)&value);
        if(count==2) { /* if sscanf() found both strings */
            if(!strcasecmp(token,"method")) {
                c->method=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"node")) {
                c->node=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"function")) {
                c->function=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"register")) {
                c->m_register=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"length")) {
                c->length=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"address")) {
                c->address=strtoul(value,NULL,10);
            } else if(!strcasecmp(token,"interval")) {
                c->interval=strtoul(value,NULL,10);
            } else {
                printf("Unknown token %s\n",token);
            }
        } else if(strcasestr(string,"</command>")) {
            return;
        } else {
            if(config.verbose)
                printf("Unknown command parameter - %s\n",token);
        }
    }
    if(config.verbose)
        printf("No </command> to end command configuration\n");
}




int getbaudrate(int b_in) {
    switch(b_in) {
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 1800:
            return B1800;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        default:
            return 0;
    }
}


/* TODO: Really should print out more than this*/
static void printconfig(void) {
    int n,i;
    struct mb_cmd *mc;
    printf("\n----------mbd Configuration-----------\n\n");
    printf("Configuration File: %s\n",config.configfile);
    printf("PID File: %s\n",config.pidfile);
    printf("Table Size: %d\n",config.tablesize);
    //--printf("IP Address: %s\n",config.ipaddress);
    //--printf("TCP Port: %d\n",config.port);
    printf("\n");
    for(n=0;n<config.portcount;n++) {
        if(config.ports[n].protocol & LAN || config.ports[n].protocol & TCP) {
            printf("Port[%d] %s %s -%s- %d\n",n,config.ports[n].name,
                                             config.ports[n].device,
                                             config.ports[n].ipaddress,
                                             config.ports[n].bindport);
        } else {
            printf("Port[%d] %s %s %d,%d,%d,%d\n",n,config.ports[n].name,
                                             config.ports[n].device,
                                             config.ports[n].baudrate,
                                             config.ports[n].databits,
                                             config.ports[n].parity,
                                             config.ports[n].stopbits);
        }
        mc=config.ports[n].commands;
        i=0;
        while(mc!=NULL) {
            printf(" Command[%d] %d %d %d %d %d\n",i++,mc->node,
                                                  mc->function,
                                                  mc->m_register,
                                                  mc->length,
                                                  mc->address);
            mc=mc->next;
        }
    }
}
