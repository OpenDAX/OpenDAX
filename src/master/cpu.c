/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2012 Phil Birkelbach
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

 * This file contains the code that is used to determine cpu and memory
 * usage for the processes.  Since this is very architecture and operating
 * system dependent there are a lot of conditional compilations and
 * interface to the auto configuration.
 */

#include <common.h>
#include <cpu.h>

/* Returns the cpu usage as a fraction (0.0 - 1.0) */
float
get_cpu_usage(pid_t pid)
{
    return 0.0;
}

/* Returns the memory usage in kB of the process indicated by pid */
int
get_mem_usage(pid_t pid)
{
    return 0;
}
