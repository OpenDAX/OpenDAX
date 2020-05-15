/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2020 Phil Birkelbach
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

 *  Source file that contains all of the virtual tag callback functions.
 */


/* Virtual tags are ones whose data is not stored in memory in the tag
 * database.  Instead the .data field of the tag database entry, which
 * is normally a pointer to the allocated data, is used as a pointer to
 * one of these functions.  They should all have the same prototype and
 * that prototype is...
 * 
 * int vfunction(int offset, void *data, int size);
 * 
 * the functions should check that size is correct and then write the
 * information into the memory pointed to by *data.  offset may be
 * used as a parameter which would allow these tags to appear to be
 * arrays.
 */

#include "virtualtag.h"

int
server_time(int offset, void *data, int size)
{
    dax_time t;
    
    t = xtime();
    memcpy(data, &t, size);
    return 0;
}    
