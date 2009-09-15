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

 * This file contains the function definitions that are used internally
 * by the libarary.  Public functions should go in opendax.h
 */

#ifndef __LIBDAX_H
#define __LIBDAX_H  

#include <common.h>
#include <opendax.h>

//#ifndef TAG_CACHE_LIMIT
//#  define TAG_CACHE_LIMIT "8"
//#endif

#define MIN_TIMEOUT      500
#define MAX_TIMEOUT      30000
#define DEFAULT_TIMEOUT  "1000"

/* Data Conversion Functions */
#define REF_INT_SWAP 0x0001
#define REF_FLT_SWAP 0x0002

/* 16 Bit conversion functions */
#define mtos_word mtos_uint
#define stom_word stom_uint

dax_int mtos_int(dax_int);
dax_uint mtos_uint(dax_uint);

dax_int stom_int(dax_int);
dax_uint stom_uint(dax_uint);

/* 32 Bit conversion functions */
#define mtos_word mtos_uint
#define stom_word stom_uint
#define mtos_time mtos_uint
#define stom_time stom_uint

dax_dint mtos_dint(dax_dint);
dax_udint mtos_udint(dax_udint);
dax_real mtos_real(dax_real);

dax_dint stom_dint(dax_dint);
dax_udint stom_udint(dax_udint);
dax_real stom_real(dax_real);

/* 64 bit Conversions */
#define mtos_lword mtos_ulint
#define stom_lword stom_ulint

dax_lint mtos_lint(dax_lint);
dax_ulint mtos_ulint(dax_ulint);
dax_lreal mtos_lreal(dax_lreal);

dax_lint stom_lint(dax_lint);
dax_ulint stom_ulint(dax_ulint);
dax_lreal stom_lreal(dax_lreal);

/* These functions handle the tag cache */
int init_tag_cache(void);
int check_cache_index(tag_index, dax_tag *);
int check_cache_name(char *, dax_tag *);
int cache_tag_add(dax_tag *);

int opt_get_msgtimeout(void);

/* This is the compound datatype member definition.  The 
 * members are represented as a linked list */
struct cdt_member {
    char *name;
    tag_type type;
    int count;
    struct cdt_member *next;
};

typedef struct cdt_member cdt_member;

/* This is the structure that represents the container for each
 * datatype. */
struct datatype{
    char *name;
    cdt_member *members;
};

typedef struct datatype datatype;

int get_typesize(tag_type type);
datatype *get_cdt_pointer(tag_type, int *);
int add_cdt_to_cache(tag_type type, char *typedesc);
int dax_cdt_get(tag_type type, char *name);

#endif /* !__LIBDAX_H */
