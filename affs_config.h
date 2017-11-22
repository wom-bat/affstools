/*
 *  Copyright (C) 2000  Roman Zippel
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */     

#ifndef AFFS_CONFIG_H
#define AFFS_CONFIG_H

#include "config.h"

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#if SIZEOF_UNSIGNED_CHAR==1
typedef unsigned char u8;
#else
#error howto typedef u8?
#endif

#if SIZEOF_SIGNED_CHAR==1
typedef signed char s8;
#else
#error howto typedef s8?
#endif

#if SIZEOF_UNSIGNED_SHORT==2
typedef unsigned short u16;
#elif SIZEOF_UNSIGNED_INT==2
typedef unsigned int u16;
#else
#error howto typedef u16?
#endif

#if SIZEOF_SIGNED_SHORT==2
typedef signed short s16;
#elif SIZEOF_SIGNED_INT==2
typedef signed int s16;
#else
#error howto typedef s16?
#endif

#if SIZEOF_UNSIGNED_INT==4
typedef unsigned int u32;
#elif SIZEOF_UNSIGNED_LONG==4
typedef unsigned long u32;
#else
#error howto typedef u32?
#endif

#if SIZEOF_SIGNED_INT==4
typedef signed int s32;
#elif SIZEOF_SIGNED_LONG==4
typedef signed long s32;
#else
#error howto typedef s32?
#endif

#endif /* AFFS_CONFIG_H */
