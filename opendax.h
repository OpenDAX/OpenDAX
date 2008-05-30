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
#define DAX_ALIAS   0x0000

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
   simple bit shift will give the size of the datatype */
#define DAX_1BIT    0x0000
#define DAX_8BITS   0x0003
#define DAX_16BITS  0x0004
#define DAX_32BITS  0x0005
#define DAX_64BITS  0x0006

/* These are the debug logging topics.  Each debug message is
   assigned one or more of these topics and will only be logged
   when the corresponding bit is set in the configuration */
#define LOG_MAJOR   0x00000001  /* Major Program Milestones */
#define LOG_MINOR   0x00000002  /* Minor Program Milestones */
#define LOG_FUNC    0x00000004  /* Function Entries */
#define LOG_COMM    0x00000008  /* Communcations Milestones */
#define LOG_MSG     0x00000010  /* Messages */
#define LOG_CONFIG  0x00000020  /* Configurations */
#define LOG_OBSCURE 0x80000000  /* Obscure Flag: to be used with the rest of the flags */

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
#define ERR_TAG_BAD   -8  /* Bad tagname */
#define ERR_TAG_DUPL  -9  /* Duplicate tagname */
#define ERR_ALLOC     -10 /* Unable to allocate resource */
#define ERR_MSG_BAD   -11 /* Bad Message Received */

/* Defines the maximum length of a tagname */
/* TODO: Get rid of this. */
#ifndef DAX_TAGNAME_SIZE
 #define DAX_TAGNAME_SIZE 32
#endif

/* typedefs to the basic DAX_? datatypes */
typedef u_int8_t   dax_byte_t;
typedef u_int8_t   dax_sint_t;
typedef u_int16_t  dax_word_t;
typedef int16_t    dax_int_t;
typedef u_int16_t  dax_uint_t;
typedef u_int32_t  dax_dword_t;
typedef int32_t    dax_dint_t;
typedef u_int32_t  dax_udint_t;
typedef u_int32_t  dax_time_t;
typedef float      dax_real_t;
typedef u_int64_t  dax_lword_t;
typedef int64_t    dax_lint_t;
typedef u_int64_t  dax_ulint_t;
typedef double     dax_lreal_t;

typedef int handle_t;

/* This is a generic representation of a tag, it may or may
   not actually represent how tags are stored. */
typedef struct {
    handle_t handle;    /* Unique tag handle */
    unsigned int type;  /* Tags data type */
    unsigned int count; /* The number of items in the tag array */
    int index;          /* Index if we are looking at a particular item */
    //int bit;            /* Bit offset if we are using bitwise operations */
    char name[DAX_TAGNAME_SIZE + 1];
} dax_tag;

void dax_set_verbosity(int); /* TODO: This should be deleted eventually */
void dax_set_debug_topic(u_int32_t);


/* These functions accept a function pointer to functions that would
   print debug and error messages.  The functions should be declared as...
   void functionname(const char *); */
int dax_set_debug(void (*debug)(const char *msg));
int dax_set_error(void (*error)(const char *msg));
int dax_set_log(void (*log)(const char *msg));

/* These are the functions that a module should actually call to log
   a message or log and exit. These are the functions that are basically
   overridden by the above _set functions.  The reason for the complexity
   is that these functions are also used internally by the library and
   this keeps everything consistent. */
void dax_debug(int topic, const char *format, ...);
void dax_error(const char *format, ...);
void dax_log(const char *format, ...);
void dax_fatal(const char *format, ...);

/* Get the datatype from a string */
int dax_string_to_type(const char *type);
/* Get a string that is the datatype, i.e. "BOOL" */
const char *dax_type_to_string(int type);
/* Retrieves the tagname and index from the form of Tagname[i] */
int dax_tag_parse(char *name, char *tagname, int *index);

/* Only registered modules will get responses from the server */
int dax_mod_register(char *);   /* Registers the Module with the server */
int dax_mod_unregister(void);   /* Unregister the Module with the server */
handle_t dax_tag_add(char *name, unsigned int type, unsigned int count);

/* Get tag by name, will decode members and subscripts */
int dax_tag_byname(char *name, dax_tag *tag);
/* Get tag by index */
int dax_tag_byhandle(handle_t index, dax_tag *tag);

/* The following functions are for reading and writing data.  These 
   are generic functions and are used by other functions within the
   library.  They can be used by the modules as well but they don't
   do any bounds checking of the tag handles.  This will make them
   more efficient but less stable if the module misbehaves.  These
   may go away. */

/* These functions are where all the data transfer takes place. */
/* simple untyped tag reading function */
int dax_read(handle_t handle, int offset, void *data, size_t size);
/* simple untyped tag writing function */
int dax_write(handle_t handle, int offset, void *data, size_t size);
/* simple untyped masked tag write */
int dax_mask(handle_t handle, int offset, void *data, void *mask, size_t size);

int dax_read_tag(handle_t handle, int index, void *data, int count, unsigned int type);
int dax_write_tag(handle_t handle, int index, void *data, int count, unsigned int type);
int dax_mask_tag(handle_t handle, int index, void *data, void *mask, int count, unsigned int type);


/* reads a single bit */
char dax_tag_read_bit(handle_t handle);
/* writes a single bit */
int dax_tag_write_bit(handle_t handle, u_int8_t data);
/* simple multiple bit read */
int dax_tag_read_bits(handle_t handle, void *data, size_t size);
/* simple multiple bit write */
int dax_tag_write_bits(handle_t handle, void *data, size_t size);
/* multiple bit write with maks */
int dax_tag_mask_write_bits(handle_t handle, void *data, void *mask, size_t size);

int dax_event_add(char *tag, int count);
int dax_event_del(int id);
int dax_event_get(int id);

#endif /* !__OPENDAX_H */
