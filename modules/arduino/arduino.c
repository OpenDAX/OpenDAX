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
 *  Main source code file for the OpenDAX Arduino I/O module
 */

#include <arduino.h>
#include <lib/arduinoio.h>

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds; /* DaxState Object */
static ar_network *_an; /* Arduino IO Network Object */
static int _quitsignal;
/* Configuration */
static char *_device;
static unsigned int _baudrate;
static int _retries;
static int _timeout;    /* Message Timeout (mSec) */
static int _rate;       /* Polling Rate (mSec) */

static ar_node _nodes[MAX_NODES];
static int _node_count;

static void
_add_pin(u_int8_t number, u_int8_t type, u_int8_t pullup, char *tagname)
{
    struct ar_pin *new_pin, *this;
    if(tagname == NULL) {
        dax_debug(ds, LOG_ERROR, "No tagname given for pin %d", number);
        return;
    }
    if(type == 0) {
        dax_debug(ds, LOG_ERROR, "Unknown type given for pin %d", number);
        return;
    }
    new_pin = malloc(sizeof(struct ar_pin));
    if(new_pin == NULL) {
        dax_fatal(ds, "Unable to allocate memory");
    }
    new_pin->type = type;
    new_pin->number = number;
    new_pin->pullup = pullup;
    new_pin->tagname = strdup(tagname);
    new_pin->next = NULL;
    
    this = _nodes[_node_count].pins;
    if(this == NULL) {
        _nodes[_node_count].pins = new_pin;
    } else {
        while(this != NULL) {
            if(this->next == NULL) {
                this->next = new_pin;
                return;
            }
            this = this->next;
        }
    }
}

static void
_add_analog(u_int8_t number, u_int8_t reference, char *tagname)
{
    struct ar_analog *new_analog, *this;
    if(tagname == NULL) {
        dax_debug(ds, LOG_ERROR, "No tagname given for analog %d", number);
        return;
    }
    if(reference == 0) {
        dax_debug(ds, LOG_ERROR, "Unknown reference given for analog %d", number);
        return;
    }
    new_analog = malloc(sizeof(struct ar_analog));
    if(new_analog == NULL) {
        dax_fatal(ds, "Unable to allocate memory");
    }
    new_analog->reference = reference;
    new_analog->number = number;
    new_analog->tagname = strdup(tagname);
    new_analog->next = NULL;
    
    this = _nodes[_node_count].analogs;
    if(this == NULL) {
        _nodes[_node_count].analogs = new_analog;
    } else {
        while(this != NULL) {
            if(this->next == NULL) {
                this->next = new_analog;
                return;
            }
            this = this->next;
        }
    }

}

/* Four arguments are passed to this function.
 * name - string for the name of the node
 * address - 2 character string that represents the address of the node
 * pins - a table array of tables that define the pins
 * analogs - a table array of tables that define the analog points
 */
static int
_add_node(lua_State *L)
{
    u_int8_t type, pullup, n;
    const char *str;
    
    if(_node_count >= MAX_NODES) {
        luaL_error(L, "Maximum number of nodes has been exceeded");
    }
    
    /* Check the arguments */
    if(lua_gettop(L) != 4) {
        luaL_error(L, "Wrong number of arguments passed to add_node()");
    }
    if(!lua_istable(L, 3)) {
        luaL_error(L, "add_node() argument 3 must be a table");
    }
    if(!lua_istable(L, 4)) {
        luaL_error(L, "add_node() argument 4 must be a table");
    }
    
    str = lua_tostring(L, 1);
    _nodes[_node_count].name = strdup(str);
    
    str = lua_tostring(L, 2);
    _nodes[_node_count].address = strdup(str);

    for(n = MIN_PIN; n < MAX_PIN; n++) {
        lua_rawgeti(L, 3, n);
        if(lua_istable(L, -1)) {
            lua_pushstring(L, "type");
            lua_rawget(L, -2);
            str = lua_tostring(L, -1);
            type = ARIO_PIN_DI;
            if(str != NULL) {
                if(strcasecmp("DI", str)==0) {
                    type = ARIO_PIN_DI;
                } else if(strcasecmp("DO", str)==0) {
                    type = ARIO_PIN_DO;
                } else if(strcasecmp("PWM", str)==0) {
                    type = ARIO_PIN_PWM;
                }
            }
            lua_pop(L, 1);
            lua_pushstring(L, "pullup");
            lua_rawget(L, -2);
            pullup = lua_toboolean(L, -1);
            lua_pop(L, 1);
            lua_pushstring(L, "tagname");
            lua_rawget(L, -2);
            _add_pin(n, type, pullup, (char *)lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    for(n = MIN_AI; n < MAX_AI; n++) {
        lua_rawgeti(L, 4, n);
        if(lua_istable(L, -1)) {
            lua_pushstring(L, "reference");
            lua_rawget(L, -2);
            str = lua_tostring(L, -1);
            type = ARIO_REF_DEFAULT;
            if(str != NULL) {
                if(strcasecmp("DEFAULT", str)==0) {
                    type = ARIO_REF_DEFAULT;
                } else if(strcasecmp("INTERNAL", str)==0) {
                    type = ARIO_REF_INTERNAL;
                } else if(strcasecmp("EXTERNAL", str)==0) {
                    type = ARIO_REF_EXTERNAL;
                    fprintf(stderr, "Pine %d set as external\n", n);
                }
            }
            lua_pop(L, 1);
            lua_pushstring(L, "tagname");
            lua_rawget(L, -2);
            _add_analog(n, type, (char *)lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    _node_count++;

    //lua_getfield(L, -1, "name");
    //name = (char *)lua_tostring(L, -1);
    
    //lua_pop(L, 1);
    return 0;
}

static int
_run_config(int argc, char *argv[])
{
    int flags, result;
    /* Create and initialize the configuration subsystem in the library */
    dax_init_config(ds, "arduino");
    flags = CFG_CMDLINE | CFG_MODCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "device","device", 'd', flags, "/dev/ttyS0");
    result += dax_add_attribute(ds, "baudrate","baudrate", 'b', flags, "9600");
    result += dax_add_attribute(ds, "retries","retries", 'r', flags, "3");
    result += dax_add_attribute(ds, "timeout","timeout", 't', flags, "500");
    result += dax_add_attribute(ds, "rate","rate", 'p', flags, "1000");
        
    dax_set_luafunction(ds, (void *)_add_node, "add_node");
    /* Execute the configuration */
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);

    /* Get the results of the configuration */
    _device = strdup(dax_get_attr(ds, "device"));
    _baudrate = strtol(dax_get_attr(ds, "baudrate"), NULL, 0);
    _retries = strtol(dax_get_attr(ds, "retries"), NULL, 0);
    _timeout = strtol(dax_get_attr(ds, "timeout"), NULL, 0);
    _rate = strtol(dax_get_attr(ds, "rate"), NULL, 0);
    /* Free the configuration data */
    dax_free_config (ds);
    return 0;
}

static void
_print_config(void)
{
    int n;
    struct ar_pin *pin;
    struct ar_analog *analog;
    fprintf(stderr, "device = %s\n", _device);
    fprintf(stderr, "baudrate = %d\n", _baudrate);
    fprintf(stderr, "retries = %d\n", _retries);
    fprintf(stderr, "timeout = %d\n", _timeout);

    for(n = 0; n < _node_count; n++) {
        fprintf(stderr, "Node[%d] name = %s : address = %s\n", n, _nodes[n].name, _nodes[n].address);
        pin = _nodes[n].pins;
        while(pin != NULL) {
            fprintf(stderr, " Pin[%d] type = %d : tagname = %s : pullup = %d\n", pin->number, pin->type, pin->tagname, pin->pullup);
            pin = pin->next;
        }
        analog = _nodes[n].analogs;
        while(analog != NULL) {
            fprintf(stderr, " Analog[%d] tagname = %s : reference = %d\n", analog->number, analog->tagname, analog->reference);
            analog = analog->next;
        }
    }
}

/* main inits and then calls run */
int main(int argc,char *argv[])
{
    struct sigaction sa;
    int result, n;
    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("arduino");
    if(ds == NULL) {
        /* dax_fatal() logs an error and causes a quit 
         * signal to be sent to the module */
        dax_fatal(ds, "Unable to Allocate DaxState Object\n");
    }

    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    _run_config(argc, argv);

    /* Check for OpenDAX and register the module */
    if( dax_mod_register(ds) ) {
        dax_fatal(ds, "Unable to find OpenDAX");
    }
    
    //--_print_config();
    
    _an = ario_init();
    if(_an == NULL) dax_fatal(ds, "Unable to allocate Arduino Network Object");
    
    result = ario_openport(_an, _device, _baudrate);
    if(result < 0) {
        dax_fatal(ds, "Unable to open Arduino Network on device %s", _device);
    }
    
    result = ario_pin_mode(_an, "AA", 3, ARIO_PIN_DI);
    fprintf(stderr, "ario_pin_mode(3) returned %d\n", result);
    result = ario_pin_mode(_an, "AA", 4, ARIO_PIN_DO);
    fprintf(stderr, "ario_pin_mode(4) returned %d\n", result);
    result = ario_pin_pullup(_an, "AA", 3, 1);
    fprintf(stderr, "ario_pin_pullup() returned %d\n", result);
    while(1) {
        usleep(_rate * 1000);
        result = ario_pin_read(_an, "AA", 3);
        fprintf(stderr, "ario_pin_read() returned %d\n", result);
        if(n == 1) {
            n = 0;
        } else {
            n = 1;
        }
        result = ario_pin_write(_an, "AA", 4, n);
        fprintf(stderr, "ario_pin_write(4, %d) returned %d\n", n, result);

        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
    }
    
 /* This is just to make the compiler happy */
    return(0);
}


/* Signal handler */
void
quit_signal(int sig)
{
    _quitsignal = sig;
}

/* We call this function to exit the program */
static void
getout(int exitstatus)
{
    dax_mod_unregister(ds);
    exit(exitstatus);
}
