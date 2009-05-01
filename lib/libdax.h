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

dax_int_t mtos_int(dax_int_t);
dax_uint_t mtos_uint(dax_uint_t);

dax_int_t stom_int(dax_int_t);
dax_uint_t stom_uint(dax_uint_t);

/* 32 Bit conversion functions */
#define mtos_word mtos_uint
#define stom_word stom_uint
#define mtos_time mtos_uint
#define stom_time stom_uint

dax_dint_t mtos_dint(dax_dint_t);
dax_udint_t mtos_udint(dax_udint_t);
dax_real_t mtos_real(dax_real_t);

dax_dint_t stom_dint(dax_dint_t);
dax_udint_t stom_udint(dax_udint_t);
dax_real_t stom_real(dax_real_t);

/* 64 bit Conversions */
#define mtos_lword mtos_ulint
#define stom_lword stom_ulint

dax_lint_t mtos_lint(dax_lint_t);
dax_ulint_t mtos_ulint(dax_ulint_t);
dax_lreal_t mtos_lreal(dax_lreal_t);

dax_lint_t stom_lint(dax_lint_t);
dax_ulint_t stom_ulint(dax_ulint_t);
dax_lreal_t stom_lreal(dax_lreal_t);

/* These functions handle the tag cache */
int init_tag_cache(void);
int check_cache_index(tag_idx_t, dax_tag *);
int check_cache_name(char *, dax_tag *);
int cache_tag_add(dax_tag *);

int opt_get_msgtimeout(void);

/* This is the custom datatype member definition.  The 
 * members are represented as a linked list */
typedef struct CDT_Member {
    char *name;
    unsigned int type;
    size_t count;
    struct CDT_Member *next;
} cdt_member;

/* This is the structure that represents the container for each
 * datatype. */
typedef struct {
    char *name;
    cdt_member *members;
} datatype;

int add_cdt_to_cache(type_t type, char *typedesc);
int dax_cdt_get(type_t type, char *name);

#endif /* !__LIBDAX_H */
