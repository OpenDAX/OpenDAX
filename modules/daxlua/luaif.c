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

 * Main source code file for the Lua script interface functions
 */

#include <daxlua.h>
#include <strings.h>
#include <pthread.h>

extern pthread_mutex_t daxmutex;

int
daxlua_init(void)
{
    pthread_mutex_init(&daxmutex, NULL);
    return 0;
}


/* Functions to expose to lua interpreter */
/* Adds a tag to the dax tagbase.  Three arguments...
    First - tagname
    Second - tag type
    Third - count
    Returns the handle on success
    Raises an error on failure
*/
static int
_dax_tag_add(lua_State *L)
{
    char *name, *type;
    int count;
    handle_t result;
    if(lua_gettop(L) != 3) {
        lua_pushnumber(L, -1.0);
        return 1;
    }
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    type = (char *)lua_tostring(L, 2); /* Get the datatype from the Lua stack */
    count = (int)lua_tonumber(L, 3);   /* Get the count from the Lua stack */
    
    if(name == NULL || type == NULL || count <=0) {
        lua_pushnumber(L, -1.0);
        return 1;
    }
    
    pthread_mutex_lock(&daxmutex);
    result = dax_tag_add(name, dax_string_to_type(type), count);
    pthread_mutex_unlock(&daxmutex);
    
    if( result > 0 ) {
        lua_pushnumber(L, (double)result);
    } else {
        lua_pushnumber(L, -1.0);
    }
    return 1;
}

/* This function figures out what type of data the tag is and translates
*buff appropriately and pushes the value onto the lua stack */
static inline void
_dax_pushdata(lua_State *L, unsigned int type, void *buff)
{
    switch (type) {
        /* Each number has to be cast to the right datatype then dereferenced
           and then cast to double for pushing into the Lua stack */
        case DAX_BYTE:
        case DAX_SINT:
            lua_pushnumber(L, (double)*((dax_sint_t *)buff));
            break;
        case DAX_WORD:
        case DAX_UINT:
            lua_pushnumber(L, (double)*((dax_uint_t *)buff));
            break;
        case DAX_INT:
            lua_pushnumber(L, (double)*((dax_int_t *)buff));
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            lua_pushnumber(L, (double)*((dax_udint_t *)buff));
            break;
        case DAX_DINT:
            lua_pushnumber(L, (double)*((dax_dint_t *)buff));
            break;
        case DAX_REAL:
            lua_pushnumber(L, (double)*((dax_real_t *)buff));
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            lua_pushnumber(L, (double)*((dax_ulint_t *)buff));
            break;
        case DAX_LINT:
            lua_pushnumber(L, (double)*((dax_lint_t *)buff));
            break;
        case DAX_LREAL:
            lua_pushnumber(L, *((dax_lreal_t *)buff));
            break;
    }
}

/* Takes the Lua value at the top of the stack, converts it and places it
   in the proper place in buff.  If mask is not NULL it will be set as well */
static inline void
_lua_to_dax(lua_State *L, unsigned int type, void *buff, void *mask, int index)
{
    lua_Integer x;
    
    switch (type) {
        case DAX_BYTE:
        case DAX_SINT:
            x = lua_tointeger(L, -1) % 256;
            ((dax_sint_t *)buff)[index] = x;
            if(mask) ((dax_sint_t *)mask)[index] = 0xFF;
            break;
        case DAX_WORD:
        case DAX_UINT:
            x = lua_tointeger(L, -1);
            ((dax_uint_t *)buff)[index] = x;
            if(mask) ((dax_uint_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_INT:
            x = lua_tointeger(L, -1);
            ((dax_int_t *)buff)[index] = x;
            if(mask) ((dax_int_t *)mask)[index] = 0xFFFF;
            break;
        case DAX_DWORD:
        case DAX_UDINT:
        case DAX_TIME:
            x = lua_tointeger(L, -1);
            ((dax_udint_t *)buff)[index] = x;
            if(mask) ((dax_udint_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_DINT:
            x = lua_tointeger(L, -1);
            ((dax_dint_t *)buff)[index] = x;
            if(mask) ((dax_dint_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_REAL:
            ((dax_real_t *)buff)[index] = (dax_real_t)lua_tonumber(L, -1);
            if(mask) ((dax_real_t *)mask)[index] = 0xFFFFFFFF;
            break;
        case DAX_LWORD:
        case DAX_ULINT:
            x = lua_tointeger(L, -1);
            ((dax_ulint_t *)buff)[index] = x;
            if(mask) ((dax_ulint_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LINT:
            x = lua_tointeger(L, -1);
            ((dax_lint_t *)buff)[index] = x;
            if(mask) ((dax_lint_t *)mask)[index] = DAX_64_ONES;
            break;
        case DAX_LREAL:
            ((dax_lreal_t *)buff)[index] = lua_tonumber(L, -1);
            if(mask) ((dax_lreal_t *)mask)[index] = DAX_64_ONES;
            break;
    }
}

/* This function finds the tag given by *tagname, get's the data from
   the server and puts the result on the top of the Lua stack. */
int
fetch_tag(lua_State *L, char *tagname)
{
    dax_tag tag;
    int size, n, result;
    void *buff;

    if( (result = dax_tag_byname(tagname, &tag)) ) {
        luaL_error(L, "Unable to retrieve tag - %s, error %d", tagname, result);
    }
    
    if(tag.type == DAX_BOOL) {
        size = tag.count / 8 +1;
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
    }
    buff = alloca(size);
    if(buff == NULL) {
        luaL_error(L, "Unable to allocate buffer size = %d", size);
    }
    result = dax_read_tag(tag.handle, 0, buff, tag.count, tag.type);
    if(result) {
        luaL_error(L, "Unable to read tag - %s", tag.name);
    }
    
    /* We have to treat Booleans differently */
    if(tag.type == DAX_BOOL) {
        /* Check to see if it's an array */
        if(tag.count > 1 ) {
            lua_createtable(L, tag.count, 0);
            for(n = 0; n < tag.count ; n++) {
                if(((u_int8_t *)buff)[n/8] & (1 << n%8)) { /* If *buff bit is set */
                    lua_pushboolean(L, 1);
                } else {  /* If the bit in the buffer is not set */
                    lua_pushboolean(L, 0);
                }
                lua_rawseti(L, -2, n + 1);
            }
        } else {
            lua_pushboolean(L, ((char *)buff)[0]);
        }
        /* Not a boolean */
    } else {
        /* Push the data up to the lua interpreter stack */
        if(tag.count > 1) { /* We need to return a table */
            lua_createtable(L, tag.count, 0);
            for(n = 0; n < tag.count ; n++) {
                _dax_pushdata(L, tag.type, buff + (TYPESIZE(tag.type) / 8) * n);
                lua_rawseti(L, -2, n + 1); /* Lua likes 1 indexed arrays */
            }
        } else { /* It's a single value */
            _dax_pushdata(L, tag.type, buff);
        }
    }
    
    return 0;
}

/* Gets the value(s) of the tag.  Accepts one argument, the tagname
and returns the value requested or raises an error on failure */
static int
_dax_read(lua_State *L)
{
    char *name;
    
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    /* Make sure that name is not NULL and we get the tagname */
    if( name == NULL) {
        luaL_error(L, "No tagname passed");
    }
    pthread_mutex_lock(&daxmutex);
    fetch_tag(L, name);
    pthread_mutex_unlock(&daxmutex);

    return 1; /* return number of retvals */
}


/* This function reads the variable from the top of the Lua stack
   and sends it to the opendax tag given by *tagname */
int
send_tag(lua_State *L, char *tagname)
{
    dax_tag tag;
    int size, n;
    void *buff, *mask;
    
    if( dax_tag_byname(tagname, &tag) ) {
        luaL_error(L, "Unable to retrieve tag - %s", tagname);
    }
    if( tag.type == DAX_BOOL ) {
        size = tag.count / 8 + 1;
    } else {
        size = tag.count * TYPESIZE(tag.type) / 8;
    }
    /* Allocate a buffer and a mask */
    buff = alloca(size);
    bzero(buff, size);
    mask = alloca(size);
    bzero(mask, size);
    if(buff == NULL || mask == NULL) {
        luaL_error(L, "Unable to allocate buffer to write tag %s", tagname);
    }
    
    if(tag.count > 1) { /* The tag is an array */
        /* Check that the second parameter is a table */
        if( ! lua_istable(L, -1) ) { 
            luaL_error(L, "Table needed to set - %s", tagname);
        }
        /* We're just searching for indexes in the table.  Anything
         other than numerical indexes in the table don't count */
        for(n = 0; n < tag.count; n++) {
            lua_rawgeti(L, -1, n+1);
            if(lua_isnil(L, -1)) {
                lua_pop(L, 1); /* Better pop that nil off the stack */
            } else { /* Ok we have something let's deal with it */
                if(tag.type == DAX_BOOL) {
                    /* Handle the boolean */
                    if(lua_toboolean(L, -1)) {
                        ((u_int8_t *)buff)[n/8] |= (1 << (n % 8));
                    } else {  /* If the bit in the buffer is not set */
                        ((u_int8_t *)buff)[n/8] &= ~(1 << (n % 8));
                    }
                    ((u_int8_t *)mask)[n/8] |= (1 << (n % 8));
                } else {
                    /* Handle the non-boolean */
                    _lua_to_dax(L, tag.type, buff, mask, n);
                }
            }
            lua_pop(L, 1);
        }
        /* Write the data to DAX */
        dax_mask_tag(tag.handle, 0, buff, mask, tag.count, tag.type);
    } else { /* Retrieved tag is a single point */
        if(tag.type == DAX_BOOL) {
            ((char *)buff)[0] = lua_toboolean(L, -1);
        } else {
            _lua_to_dax(L, tag.type, buff, mask, 0);
        }
        dax_write_tag(tag.handle, 0, buff, tag.count, tag.type);
    }
    return 0;
}

/* Sets a given tag to the given value.  Two arguments...
    First argument is the string or handle for the dax_tag
    Second argument is the value to set the tag too.
    Raises an error on failure
*/
static int
_dax_write(lua_State *L)
{
    char *name;
        
    name = (char *)lua_tostring(L, 1); /* Get the tagname from the Lua stack */
    /* Make sure that name is not NULL and we get the tagname */
    if(name) {
        pthread_mutex_lock(&daxmutex);
        send_tag(L, name);
        pthread_mutex_unlock(&daxmutex);
    } else {
        luaL_error(L, "No Name Given to Write");
        return -1;
    }
    
    return 0; /* return number of retvals */
}

static int
_add_global(char *script, char *varname, unsigned char mode)
{
    global_t *glo;
    script_t *scr;
    
    scr = get_script_name(script);
    if(scr == NULL) return -1;
    
    glo = malloc(sizeof(global_t));
    if(glo) {
        glo->name = strdup( varname );
        glo->mode = mode;
        glo->ref = LUA_NOREF;
        glo->next = scr->globals;
        scr->globals = glo;
    } else {
        return -1;
    }
    return 0;
}

/* Adds a tag to the *globals list for registration
   Three arguments.  First is name of the script
   second is the name of the tag and the third is
   is the read/write mode ("R", "W" or "RW") */
/* TODO: should this be allowed in a normal script?  I guess
   I don't want any limits but couldn't the scriptname be assumed
   if run from a normal script or given if meant for another script? */
static int
_register_tag(lua_State *L)
{
    char *script, *varname, *modestring;
    unsigned char mode = 0;
    int n;
    
    script = (char *)lua_tostring(L, 1);
    if(script == NULL) {
        luaL_error(L, "Script name argument not supplied");
    }
    varname = (char *)lua_tostring(L, 2);
    if(varname == NULL) {
        luaL_error(L, "Variable name argument not supplied");
    }
    modestring = (char *)lua_tostring(L, 3);
    if(modestring == NULL) {
        luaL_error(L, "Mode string arguemtn not supplied");
    }
    for(n = 0; modestring[n] != '\0'; n++) {
        if(modestring[n] == 'r' || modestring[n] == 'R') {
            mode |= MODE_READ;
        } else if(modestring[n] == 'w' || modestring[n] == 'W') {
            mode |= MODE_WRITE;
        } else {
            luaL_error(L, "Invalid mode string");
        }
    }
    if(_add_global(script, varname, mode) < 0) {
        luaL_error(L, "Problem adding tag to global registry");
    }
    return 0;
}

/* Adds a static definition to the *globals list.
   Takes two arguments, script name, static name */
/* TODO: should this be allowed in a normal script?  I guess
 I don't want any limits but couldn't the scriptname be assumed
 if run from a normal script or given if meant for another script? */
static int
_register_static(lua_State *L)
{
    char *script, *varname;
    
    script = (char *)lua_tostring(L, 1);
    if(script == NULL) {
        luaL_error(L, "Script name argument not supplied");
    }
    varname = (char *)lua_tostring(L, 2);
    if(varname == NULL) {
        luaL_error(L, "Variable name argument not supplied");
    }
    if(_add_global(script, varname, MODE_STATIC) < 0) {
        luaL_error(L, "Problem adding variable to global registry");
    }
    return 0;    
}
    


/* TODO: Stuff to add.
 * give the scripts a way to enable and disable each other
 */

/* Use this function to setup each interpreter with all the right function calls
and libraries */
int
setup_interpreter(lua_State *L)
{
    lua_pushcfunction(L, _dax_tag_add);
    lua_setglobal(L, "dax_tag_add");
    
    lua_pushcfunction(L, _dax_read);
    lua_setglobal(L, "dax_read");
    
    lua_pushcfunction(L, _dax_write);
    lua_setglobal(L, "dax_write");
    
    lua_pushcfunction(L, _register_tag);
    lua_setglobal(L, "register_tag");
    
    lua_pushcfunction(L, _register_static);
    lua_setglobal(L, "register_static");
    
    /* register the libraries that we need*/
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);
    
    return 0; /* I don't think anything returns an error */
}