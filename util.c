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

#include "affs_config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "amigaffs.h"

void affs_print(int level, char *fmt, ...)
{
	va_list ap;

	if (level > info.verbose)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void affs_error(char *fmt, ...)
{
	va_list ap;

	printf("%s: ", affs_prog);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

