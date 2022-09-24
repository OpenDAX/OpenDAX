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

/*! \file
 * This is the main header file for the libdax API.  This must be included
 * by all client modules that use the libdax C library.
 */


#ifndef __OPENDAX_H
#define __OPENDAX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <float.h>
#include <lua.h>

/* Data type definitions */
#define DAX_BOOL    0x0010
/* 8 Bits */
#define DAX_BYTE    0x0003
#define DAX_SINT    0x0013
#define DAX_CHAR    0x0023
/* 16 Bits */
#define DAX_WORD    0x0004
#define DAX_INT     0x0014
#define DAX_UINT    0x0024
/* 32 Bits */
#define DAX_DWORD   0x0005
#define DAX_DINT    0x0015
#define DAX_UDINT   0x0025
#define DAX_REAL    0x0035
/* 64 Bits */
#define DAX_LWORD   0x0006
#define DAX_LINT    0x0016
#define DAX_ULINT   0x0026
#define DAX_TIME    0x0036 /* milliseconds since the epoch */
#define DAX_LREAL   0x0046

/* The highest order bit shows that it's a custom datatype */
#define DAX_CUSTOM  0x80000000
/* If this bit is set then the datatype is a queue */
#define DAX_QUEUE   0x40000000

/* The size of the base datatypes are based on the lower 4 bits.  A
 * simple bit shift will give the size of the datatype */
#define DAX_1BIT    0x0000
#define DAX_8BITS   0x0003
#define DAX_16BITS  0x0004
#define DAX_32BITS  0x0005
#define DAX_64BITS  0x0006

/* These are the logging topics.  Each log message is
 * assigned one or more of these topics and will only be logged
 * when the corresponding bit is set in the configuration */
#define LOG_MINOR     0x00000001  /* Minor Program Milestones */
#define LOG_MAJOR     0x00000002  /* Major Program Milestones */
#define LOG_WARN      0x00000004  /* Program warnings */
#define LOG_ERROR     0x00000008  /* Program errors */
#define LOG_FATAL     0x00000010  /* Program fatal errors */
#define LOG_MODULE    0x00000020  /* Module Milestones */
#define LOG_COMM      0x00000040  /* Communications Milestones */
#define LOG_MSG       0x00000080  /* Messages */
#define LOG_MSGERR    0x00000100  /* Message Errors */
#define LOG_CONFIG    0x00000200  /* Configuration file prints */
#define LOG_PROTOCOL  0x00000400  /* Protocol Dumps */
#define LOG_INFO      0x00000800  /* Low priority information */
#define LOG_DEBUG     0x00001000  /* Debug messages */
#define LOG_LOGIC     0x00002000  /* Embedded logic messages */
#define LOG_LOGICERR  0x00004000  /* Embedded logic errors */
#define LOG_USER1     0x01000000  /* Module specific log topic */
#define LOG_USER2     0x02000000  /* Module specific log topic */
#define LOG_USER3     0x04000000  /* Module specific log topic */
#define LOG_USER4     0x08000000  /* Module specific log topic */
#define LOG_USER5     0x10000000  /* Module specific log topic */
#define LOG_USER6     0x20000000  /* Module specific log topic */
#define LOG_USER7     0x40000000  /* Module specific log topic */
#define LOG_USER8     0x80000000  /* Module specific log topic */
#define LOG_ALL       0xFFFFFFFF  /* Log everything */

/*! Macro to get the size of the datatype in bits*/
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
#define ERR_EMPTY     -20 /* Empty */
#define ERR_BADTYPE   -21 /* Bad Datatype */
#define ERR_BADINDEX  -22 /* Bad Index */
#define ERR_AUTH      -23 /* Not Authorized */
#define ERR_OVERFLOW  -24 /* Overflow Error */
#define ERR_UNDERFLOW -25 /* Underflow Error */
#define ERR_DELETED   -26 /* Object has been deleted */
#define ERR_READONLY  -27 /* Resource is read only */
#define ERR_WRITEONLY -28 /* Resource is write only */
#define ERR_NOTIMPLEMENTED -29 /* Function not implemented */
#define ERR_FILE_CLOSED   -30 /* File is not open */
#define ERR_DISCONNECTED  -31 /* Client is disconnected */

/* Module configuration flags */
#define CFG_ARG_NONE        0x00 /* No Arguments */
#define CFG_ARG_OPTIONAL    0x01 /* Argument is optional */
#define CFG_ARG_REQUIRED    0x02 /* Argument is required */
#define CFG_CMDLINE         0x04 /* Command line */
#define CFG_MODCONF         0x10 /* [module].conf file */
#define CFG_NO_VALUE        0x20 /* Don't store a value, only call callback */

/* Tag attribute bits */
#define TAG_ATTR_READONLY   0x0001 /* Read only tag */
#define TAG_ATTR_VIRTUAL    0x0002 /* Tag is a virtual tag */
#define TAG_ATTR_RETAIN     0x0004 /* Tag will be retained */
#define TAG_ATTR_OVR_SET    0x0008 /* Tag override is set */
#define TAG_ATTR_MAPPING    0x1000 /* Tag is the source of at least one map */
#define TAG_ATTR_EVENT      0x2000 /* Tag has at least one event */
#define TAG_ATTR_OVERRIDE   0x4000 /* Tag has override installed */


/* Module parameters */
#define MOD_CMD_RUNNING     0x01 /* Set/Clear Modules Running Flag */

/* Event Types */
#define EVENT_READ     0x01 /* Called before a tag is read - Not implemented */
#define EVENT_WRITE    0x02 /* When a tag is written whether or not it has changed */
#define EVENT_CHANGE   0x03 /* When a change to the tags value has occurred */
#define EVENT_SET      0x04 /* Bit set to 1*/
#define EVENT_RESET    0x05 /* Bit cleared to 0*/
#define EVENT_EQUAL    0x06 /* Equal to */
#define EVENT_GREATER  0x07 /* Greater Than */
#define EVENT_LESS     0x08 /* Less Than */
#define EVENT_DEADBAND 0x09 /* Changed by X amount since last event */
#define EVENT_DELETED  0x0A /* Tag gets deleted */

/* Event Options */
#define EVENT_OPT_SEND_DATA  0x01 /* Send the affected data with the event */

/* Atomic Operations */
#define ATOMIC_OP_INC  0x0001  /* Increment */
#define ATOMIC_OP_DEC  0x0002  /* Decrement */
#define ATOMIC_OP_NOT  0x0003  /* Compliment */
#define ATOMIC_OP_OR   0x0004  /* Bitwise OR */
#define ATOMIC_OP_AND  0x0005  /* Bitwise AND */
#define ATOMIC_OP_NOR  0x0006  /* Bitwise NOR */
#define ATOMIC_OP_NAND 0x0007  /* Bitwise NAND */
#define ATOMIC_OP_XOR  0x0008  /* Bitwise XOR */


/* Defines the maximum length of a tagname */
#ifndef DAX_TAGNAME_SIZE
 #define DAX_TAGNAME_SIZE 32
#endif

#define IS_CUSTOM(TYPE) ((TYPE) & DAX_CUSTOM)
#define IS_QUEUE(TYPE) ((TYPE) & DAX_QUEUE)

/* 8 Bit */
#define DAX_BYTE_MIN    0
#define DAX_BYTE_MAX    255
#define DAX_SINT_MIN   -128
#define DAX_SINT_MAX    127
/* 16 Bit */
#define DAX_WORD_MIN    0
#define DAX_WORD_MAX    65535
#define DAX_UINT_MIN    DAX_WORD_MIN
#define DAX_UINT_MAX    DAX_WORD_MAX
#define DAX_INT_MIN    -32768
#define DAX_INT_MAX     32767
/* 32 Bit */
#define DAX_DWORD_MIN   0
#define DAX_DWORD_MAX   4294967295
#define DAX_DINT_MIN   -2147483648
#define DAX_DINT_MAX    2147483647
#define DAX_UDINT_MIN   DAX_DWORD_MIN
#define DAX_UDINT_MAX   DAX_DWORD_MAX
/* 64 Bit */
#define DAX_LWORD_MIN    0
#define DAX_LWORD_MAX    18446744073709551615ULL
#define DAX_LINT_MIN    -9223372036854775808LL
#define DAX_LINT_MAX     9223372036854775807LL
#define DAX_ULINT_MIN    DAX_LWORD_MIN
#define DAX_ULINT_MAX    DAX_LWORD_MAX
#define DAX_TIME_MIN     DAX_LINT_MIN
#define DAX_TIME_MAX     DAX_LINT_MAX
/* Floating Point */
#define DAX_REAL_MIN   -FLT_MAX
#define DAX_REAL_MAX    FLT_MAX
#define DAX_LREAL_MIN  -DBL_MAX
#define DAX_LREAL_MAX   DBL_MAX

/* typedefs to the basic DAX_? data types */
typedef uint8_t   dax_byte;
typedef int8_t     dax_sint;
typedef char       dax_char;
typedef uint16_t  dax_word;
typedef int16_t    dax_int;
typedef uint16_t  dax_uint;
typedef uint32_t  dax_dword;
typedef int32_t    dax_dint;
typedef uint32_t  dax_udint;
typedef int64_t    dax_time;
typedef float      dax_real;
typedef uint64_t  dax_lword;
typedef int64_t    dax_lint;
typedef uint64_t  dax_ulint;
typedef double     dax_lreal;

typedef dax_dint tag_index;
typedef dax_udint tag_type;

/*!
 * This structure is used to define a specific tag or a part of the tag for
 * reading and writing.
 */
struct tag_handle {
    tag_index index;     /*!< The Database Index of the Tag */
    uint32_t byte;      /*!< The byte offset where the data block starts */
    unsigned char bit;   /*!< The bit offset */
    uint32_t count;     /*!< The number of items represented by the handle */
    uint32_t size;      /*!< The total size of the data block in bytes */
    tag_type type;       /*!< The data type of the block */
};

typedef struct tag_handle tag_handle;

/* This is a generic representation of a tag, it may or may
 * not actually represent how tags are stored. */
struct dax_tag {
    tag_index idx;        /* Unique tag index */
    tag_type type;        /* Tags data type */
    unsigned int count;   /* The number of items in the tag array */
    uint16_t attr;
    char name[DAX_TAGNAME_SIZE + 1];
};

typedef struct dax_tag dax_tag;

/*!
 * Identifier that is passed back and forth from modules to the server to
 * uniquely identify events or mappings in the system.
 */
typedef struct dax_id
{
    tag_index index;     /* The Index of the Tag */
    int id;              /* The byte offset where the data block starts */
} dax_id;

/*! Opaque pointer for storing a dax_state object in the library */
typedef struct dax_state dax_state;
/*! Opaque pointer for tag group */
typedef struct tag_group_id tag_group_id;

/* Easy way to store base datatypes.  Doesn't include BOOL */
typedef union dax_type_union {
    dax_byte   dax_byte;
    dax_sint   dax_sint;
    dax_char   dax_char;
    dax_word   dax_word;
    dax_int    dax_int;
    dax_uint   dax_uint;
    dax_dword  dax_dword;
    dax_dint   dax_dint;
    dax_udint  dax_udint;
    dax_time   dax_time;
    dax_lword  dax_lword;
    dax_lint   dax_lint;
    dax_ulint  dax_ulint;
    dax_real   dax_real;
    dax_lreal  dax_lreal;
} dax_type_union;

/* These functions are for module configuration */
dax_state *dax_init(char *name);
int dax_init_config(dax_state *ds, char *name);
int dax_set_luafunction(dax_state *ds, int (*f)(void *L), char *name);
int dax_clear_luafunction(dax_state *ds, char *name);
lua_State *dax_get_luastate(dax_state *ds);
int dax_add_attribute(dax_state *ds, char *name, char *longopt, char shortopt, int flags, char *defvalue);
int dax_configure(dax_state *ds, int argc, char **argv, int flags);
char *dax_get_attr(dax_state *ds, char *name);
int dax_set_attr(dax_state *ds, char *name, char *value);
int dax_attr_callback(dax_state *ds, char *name, int (*attr_callback)(char *name, char *value));
int dax_free_config(dax_state *ds);
int dax_free(dax_state *ds);

/* Logging library functions */
uint32_t dax_parse_log_topics(char *topic_string);
int dax_log_topic_callback(void (*topic_callback)(char *topic));
int dax_init_logger(const char *name, uint32_t topics);
int dax_log_set_lua_function(lua_State *L);
void dax_log_set_default_mask(uint32_t mask);
void dax_log_set_default_topics(char *topics);
void dax_log(uint32_t topic, const char *format, ...);

/* Create and destroy connections to the server */
int dax_connect(dax_state *ds);      /* Connect to the server */
int dax_disconnect(dax_state *ds);   /* Disconnect from the server */

int dax_mod_get(dax_state *ds, char *modname);  /* Not implemented yet */
int dax_mod_set(dax_state *ds, uint8_t cmd, void *param);  /* Set module parameters in the server */

/* Adds a tag to the opendax server database. */
int dax_tag_add(dax_state *ds, tag_handle *h, char *name, tag_type type, int count, uint32_t attr);

/* Delete the tag give by index */
int dax_tag_del(dax_state *ds, tag_index index);

/* Get tag by name, will not decode members and subscripts */
int dax_tag_byname(dax_state *ds, dax_tag *tag, char *name);
/* Get tag by index */
int dax_tag_byindex(dax_state *ds, dax_tag *tag, tag_index index);

/* The handle is a complete description of where in the tagbase the
 * data that we wish to retrieve is located.  This can be used in place
 * of a tagname string such as "Tag1.member1[5]".  Count is the number of
 * items, that we want. */
int dax_tag_handle(dax_state *ds, tag_handle *h, char *str, int count);

/* Returns the size of the datatype in bytes */
int dax_get_typesize(dax_state *ds, tag_type type);

/* These are low level functions that write data directly to the server.
 * They do no type checking or bounds checking of the data and should
 * be avoided by client modules unless there is a really good reason.
 */
/* idx is the index of the tag, offset is the byte offset within the tag
 * that we are requesting, *data is a pointer to the memory that we are
 * reading or writing, size is the number of bytes that we are reading or
 * writing. *mask is a binary mask that only allows data through where
 * bits are high.  *data and *mask should point to data that at least
 * 'size' bytes.
 */
/* simple untyped tag reading function */
int dax_read(dax_state *ds, tag_index idx, uint32_t offset, void *data, size_t size);
/* simple untyped tag writing function */
int dax_write(dax_state *ds, tag_index idx, uint32_t offset, void *data, size_t size);
/* simple untyped masked tag write */
int dax_mask(dax_state *ds, tag_index idx, uint32_t offset, void *data,
             void *mask, size_t size);

/* These are the bread and butter tag handling functions.  The functions
 * understand the type of tag being written and take care of all the
 * data formatting necessary to read / write the tag to the server.  These
 * functions actually end up calling the above untyped functions.  The
 * caller must take care to allocate enough space in *data so that the
 * read function doesn't overflow the buffer and that the write functions
 * don't try to dereference memory that is not allocated. */

/* handle is the identifier of the tag that was returned by dax_tag_add() or
 * other query function. *data is a pointer to the buffer of memory
 * that will be written or read. The mask is a binary mask that only
 * allows data through where bits are high. */
int dax_read_tag(dax_state *ds, tag_handle handle, void *data);
int dax_write_tag(dax_state *ds, tag_handle handle, void *data);
int dax_mask_tag(dax_state *ds, tag_handle handle, void *data, void *mask);

/* These functions handle the tag value override feature */
int dax_tag_add_override(dax_state *ds, tag_handle handle, void *data);
int dax_tag_del_override(dax_state *ds, tag_handle handle);
int dax_tag_get_override(dax_state *ds, tag_handle handle, void *data, void *mask);
int dax_tag_set_override(dax_state *ds, tag_handle handle);
int dax_tag_clr_override(dax_state *ds, tag_handle handle);

int dax_atomic_op(dax_state *ds, tag_handle handle, void *data, uint16_t operation);

/* Event handling functions */
int dax_event_add(dax_state *ds, tag_handle *handle, int event_type, void *data,
                  dax_id *id, void (*callback)(dax_state *ds, void *udata), void *udata,
                  void (*free_callback)(void *udata));
int dax_event_del(dax_state *ds, dax_id id);
int dax_event_get(dax_state *ds, dax_id id);
int dax_event_options(dax_state *ds, dax_id id, uint32_t options);
int dax_event_wait(dax_state *ds, int timeout, dax_id *id);
int dax_event_poll(dax_state *ds, dax_id *id);
//int dax_event_get_fd(dax_state *ds);
int dax_event_get_data(dax_state *ds, void* buff, int len);

/* Event Utility Functions */
int dax_event_string_to_type(char *string);
const char *dax_event_type_to_string(int type);

/* Custom Datatype Functions */
typedef struct datatype dax_cdt;

/* Get the datatype from a string */
tag_type dax_string_to_type(dax_state *ds, char *type);
/* Get a string that is the datatype, i.e. "BOOL" */
const char *dax_type_to_string(dax_state *ds, tag_type type);

dax_cdt *dax_cdt_new(char *name, int *error);
int dax_cdt_member(dax_state *ds, dax_cdt *cdt, char *name,
                   tag_type mem_type, unsigned int count);
int dax_cdt_create(dax_state *ds, dax_cdt *cdt, tag_type *type);
void dax_cdt_free(dax_cdt *cdt);

/* Custom Datatype Iterator */
struct cdt_iter {
    const char *name;
    tag_type type;
    int count;
    int byte;
    int bit;
};

typedef struct cdt_iter cdt_iter;

/* Iterate over the members of a CDT */
int dax_cdt_iter(dax_state *ds, tag_type type, void *udata, void (*callback)(cdt_iter member, void *udata));

/* Add remove and get data table mapping functions */
int dax_map_add(dax_state *ds, tag_handle *src, tag_handle *dest, dax_id *id);

/* Tag data group functions */
tag_group_id *dax_group_add(dax_state *ds, int *result, tag_handle *h, int count, uint8_t options);
int dax_group_get_size(tag_group_id *id);
int dax_group_read(dax_state *ds, tag_group_id *id, void *buff, size_t size);
int dax_group_write(dax_state *ds, tag_group_id *id, void *buff);
int dax_group_del(dax_state *ds, tag_group_id *id);

/* Convenience functions for converting strings to basic DAX values and back */
int dax_val_to_string(char *buff, int size, tag_type type, void *val, int index);
int dax_string_to_val(char *instr, tag_type type, void *buff, void *mask, int index);

#ifdef __cplusplus
}
#endif

#endif /* !__OPENDAX_H */
