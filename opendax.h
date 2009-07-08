/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 */


#ifndef __OPENDAX_H
#define __OPENDAX_H

#include <sys/types.h>

/* Data type definitions */
/* #define DAX_ALIAS   0x0000 // Don't know about alias' at this point */

#define DAX_BOOL    0x0010
/* 8 Bits */
#define DAX_BYTE    0x0003
#define DAX_SINT    0x0013
/* 16 Bits */
#define DAX_WORD    0x0004
#define DAX_INT     0x0014
#define DAX_UINT    0x0024
/* 32 Bits */
#define DAX_DWORD   0x0005
#define DAX_DINT    0x0015
#define DAX_UDINT   0x0025
#define DAX_TIME    0x0035
#define DAX_REAL    0x0045
/* 64 Bits */
#define DAX_LWORD   0x0006
#define DAX_LINT    0x0016
#define DAX_ULINT   0x0026
#define DAX_LREAL   0x0036

/* The highest order bit shows that it's a custom datatype */
#define DAX_CUSTOM  0x80000000

/* The size of the base datatypes are based on the lower 4 bits.  A
 * simple bit shift will give the size of the datatype */
#define DAX_1BIT    0x0000
#define DAX_8BITS   0x0003
#define DAX_16BITS  0x0004
#define DAX_32BITS  0x0005
#define DAX_64BITS  0x0006

/* These are the debug logging topics.  Each debug message is
 * assigned one or more of these topics and will only be logged
 * when the corresponding bit is set in the configuration */
#define LOG_MAJOR   0x00000001  /* Major Program Milestones */
#define LOG_MINOR   0x00000002  /* Minor Program Milestones */
#define LOG_FUNC    0x00000004  /* Function Entries */
#define LOG_COMM    0x00000008  /* Communcations Milestones */
#define LOG_MSG     0x00000010  /* Messages */
#define LOG_CONFIG  0x00000020  /* Configurations */
#define LOG_MODULE  0x00000040  /* Module Milestones */
#define LOG_OBSCURE 0x80000000  /* Used to increase the verbosity of the other topics */

/* Macro to get the size of the datatype */
#define TYPESIZE(TYPE) (0x0001 << (TYPE & 0x0F))

/* Library function errors */
#define ERR_GENERIC   -1  /* Commenting this out may reveal code that needs work */
#define ERR_NO_SOCKET -2  /* The Message socket does not exist or cannot be created*/
#define ERR_2BIG      -3  /* The argument is too big */
#define ERR_ARG       -4  /* Bad argument */
#define ERR_NOTFOUND  -5  /* Not found */
#define ERR_MSG_SEND  -6  /* Unable to send message */
#define ERR_MSG_RECV  -7  /* Unable to receive message */
#define ERR_TAG_BAD   -8  /* Bad name */
#define ERR_TAG_DUPL  -9  /* Duplicate name */
#define ERR_ALLOC     -10 /* Unable to allocate resource */
#define ERR_MSG_BAD   -11 /* Bad Message Received */
#define ERR_DUPL      -12 /* Duplicate */
#define ERR_NO_INIT   -13 /* Not Initialized */
#define ERR_TIMEOUT   -14 /* Timeout */
#define ERR_ILLEGAL   -15 /* Illegal Setting */
#define ERR_INUSE     -16 /* Object is in use */
#define ERR_PARSE     -17 /* Parsing Error */
#define ERR_ARBITRARY -18 /* Arbitrary Argument */
#define ERR_NOTNUMBER -19 /* Non Numeric Argument */

/* Module configuration flags */
#define CFG_ARG_NONE 		0x00 /* No Arguments */
#define CFG_ARG_OPTIONAL	0x01 /* Argument is optional */
#define CFG_ARG_REQUIRED	0x02 /* Argument is required */
#define CFG_CMDLINE			0x04 /* Command line */
#define CFG_DAXCONF			0x08 /* opendax.conf file */
#define CFG_MODCONF		    0x10 /* [module].conf file */
#define CFG_NO_VALUE        0x20 /* Don't store a value, only call callback */


/* Defines the maximum length of a tagname */
#ifndef DAX_TAGNAME_SIZE
 #define DAX_TAGNAME_SIZE 32
#endif

#define IS_CUSTOM(TYPE) ((TYPE) & DAX_CUSTOM)

/* typedefs to the basic DAX_? datatypes */
typedef u_int8_t   dax_byte;
typedef u_int8_t   dax_sint;
typedef u_int16_t  dax_word;
typedef int16_t    dax_int;
typedef u_int16_t  dax_uint;
typedef u_int32_t  dax_dword;
typedef int32_t    dax_dint;
typedef u_int32_t  dax_udint;
typedef u_int32_t  dax_time;
typedef float      dax_real;
typedef u_int64_t  dax_lword;
typedef int64_t    dax_lint;
typedef u_int64_t  dax_ulint;
typedef double     dax_lreal;

typedef int tag_index;
typedef unsigned int tag_type;

struct Handle {
    tag_index index;      /* The Index of the Tag */
    int byte;            /* The byte offset where the data block starts */
    unsigned char bit;   /* The bit offset */
    int count;           /* The number of items represented by the handle */
    u_int32_t size;      /* The total size of the data block in bytes */
    tag_type type;       /* The datatype of the block */
};

typedef struct Handle Handle;

/* This is a generic representation of a tag, it may or may
 * not actually represent how tags are stored. */
struct dax_tag {
    tag_index idx;         /* Unique tag index */
    tag_type type;        /* Tags data type */
    unsigned int count;   /* The number of items in the tag array */
    char name[DAX_TAGNAME_SIZE + 1];
};

typedef struct dax_tag dax_tag;

/* These functions are for module configuration */
int dax_init_config(char *name);
/* is void* the right thing to do with the lua_State? */
int dax_set_luafunction(int (*f)(void *L), char *name);
int dax_add_attribute(char *name, char *longopt, char shortopt, int flags, char *defvalue);
int dax_configure(int argc, char **argv, int flags);
char *dax_get_attr(char *name);
int dax_set_attr(char *name, char *value);
int dax_attr_callback(char *name, int (*attr_callback)(char *name, char *value));
int dax_free_config(void);

void dax_set_verbosity(int); /* TODO: This should be deleted eventually */
void dax_set_debug_topic(u_int32_t);


/* These functions accept a function pointer to functions that would
 * print debug and error messages.  The functions should be declared as...
 * void functionname(const char *); These are required because the library
 * uses some functions for error messaging and this allows the module to deal
 * with these messages how it sees fit.  These would normally not be used and
 * when these callbacks are not set the default messaging is used. */
int dax_set_debug(void (*debug)(const char *msg));
int dax_set_error(void (*error)(const char *msg));
int dax_set_log(void (*log)(const char *msg));

/* These are the functions that a module should actually call to log
 * a message or log and exit. These are the functions that are basically
 * overridden by the above _set functions.  The reason for the complexity
 * is that these functions are also used internally by the library and
 * this keeps everything consistent. */
void dax_debug(int topic, const char *format, ...);
void dax_error(const char *format, ...);
void dax_log(const char *format, ...);
void dax_fatal(const char *format, ...);

/* Only registered modules will get responses from the server */
int dax_mod_register(char *);   /* Registers the Module with the server */
int dax_mod_unregister(void);   /* Unregister the Module with the server */

/* Adds a tag to the opendax server database. */
int dax_tag_add(Handle *h, char *name, tag_type type, int count);
//--tag_index dax_tag_add(char *name, tag_type type, unsigned int count);

/* Get tag by name, will not decode members and subscripts */
int dax_tag_byname(dax_tag *tag, char *name);
/* Get tag by index */
int dax_tag_byindex(dax_tag *tag, tag_index index);

/* The handle is a complete description of where in the tagbase the
 * data that we wish to retrieve is located.  This can be used in place
 * of a tagname string such as "Tag1.member1[5]".  Count is the number of
 * items, that we want. */
int dax_tag_handle(Handle *h, char *str, int count);

/* Retrieves the tagname and index from the form of Tagname[i] */
/* TODO: dax_tag_parse should probably be deprecated */
int dax_tag_parse(char *name, char *tagname, int *index);

/* The following functions are for reading and writing data.  These 
 * are generic functions and are used by other functions within the
 * library.  They can be used by the modules as well but they don't
 * do any bounds checking of the tag handles.  This will make them
 * more efficient but less stable if the module misbehaves.  These
 * may go away. */

/* simple untyped tag reading function */
int dax_read(tag_index idx, int offset, void *data, size_t size);
/* simple untyped tag writing function */
int dax_write(tag_index idx, int offset, void *data, size_t size);
/* simple untyped masked tag write */
int dax_mask(tag_index idx, int offset, void *data, void *mask, size_t size);

/* These are the bread and butter tag handling functions.  The functions
 * understand the type of tag being written and take care of all the
 * data formatting necessary to read / write the tag to the server.  These
 * functions actually end up calling the above untyped functions.  The
 * caller must take care to allocate enough space in *data so that the
 * read function doesn't overflow the buffer an that the write functions
 * don't try to dereference memory that is not allocated. */

/* idx is the index of the tag that was returned by dax_tag_add() or
 * other query function.  index is the index into the tag to be written.
 * *data is a pointer to the buffer of memory that will be written or read.
 * count is the number of items in the tag to read/write.  The mask is
 * a binary mask that only allows data through where bits are high.
 * type is the tag type. */
int dax_read_tag(Handle handle, void *data);
int dax_write_tag(Handle handle, void *data);
int dax_mask_tag(Handle handle, void *data, void *mask);
//--int dax_read_tag(tag_idx_t idx, int index, void *data, int count, unsigned int type);
//--int dax_write_tag(tag_idx_t idx, int index, void *data, int count, unsigned int type);
//--int dax_mask_tag(tag_idx_t idx, int index, void *data, void *mask, int count, unsigned int type);

/* Event control functions */
int dax_event_add(char *tag, int count);
int dax_event_del(int id);
int dax_event_get(int id);

/* Custom Datatype Functions */

/* Get the datatype from a string */
tag_type dax_string_to_type(char *type);
/* Get a string that is the datatype, i.e. "BOOL" */
const char *dax_type_to_string(tag_type type);

/* TODO: This could be improved. It really needs to be atomic */
tag_type dax_cdt_create(char *name, int *error);
int dax_cdt_add(tag_type cdt_type, char *name, tag_type mem_type, unsigned int count);
int dax_cdt_finalize(tag_type type);

/* Custom Datatype Iterator */
struct cdt_iter {
    const char *name;
    tag_type type;
    int count;
    int byte;
    int bit;
};

typedef struct cdt_iter cdt_iter;

int dax_cdt_iter(tag_type type, void *udata, void (*callback)(cdt_iter member, void *udata));

#endif /* !__OPENDAX_H */
