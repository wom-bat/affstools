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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "amigaffs.h"


u8 affs_rootbuf[AFFS_BLOCKSIZE_MAX];
u8 affs_databuf[AFFS_BLOCKSIZE_MAX];

int affs_bread(void *data, u32 block)
{
	int res;

	if (block < info.reserved) {
		affs_error("unable to read block %d (block is reserved)\n", block);
		return 1;
	} else if (block >= info.blocks) {
		affs_error("unable to read block %d (block is out of range)\n", block);
		return 1;
	}

	if (lseek(info.devfd, (off_t)block << info.blockshift, SEEK_SET) < 0) {
		affs_error("unable to seek to block %d (%s)\n", block, strerror(errno));
		return 1;
	}

	res = read(info.devfd, data, info.blocksize);
	if (res == info.blocksize)
		return 0;
	if (res < 0) {
		affs_error("unable to read block %d (%s)\n", block, strerror(errno));
		return 1;
	}
	affs_error("short read of block %d (%d of %d)\n", block, res, info.blocksize);
	return 1;
}

int affs_bwrite(void *data, u32 block)
{
	int res;

	if (block < info.reserved) {
		affs_error("unable to write block %d (block is reserved)\n", block);
		return 1;
	} else if (block >= info.blocks) {
		affs_error("unable to write block %d (block is out of range)\n", block);
		return 1;
	}

	if (lseek(info.devfd, (off_t)block << info.blockshift, SEEK_SET) < 0) {
		affs_error("unable to seek to block %d (%s)\n", block, strerror(errno));
		return 1;
	}

	res = write(info.devfd, data, info.blocksize);
	if (res == info.blocksize)
		return 0;
	if (res < 0) {
		affs_error("unable to write block %d (%s)\n", block, strerror(errno));
		return 1;
	}
	affs_error("short write of block %d (%d of %d)\n", block, res, info.blocksize);
	return 1;
}

u32 affs_checksum(void *data)
{
	u32 *ptr = (u32 *)data;
	u32 chksum = 0;
	int cnt;

	for (cnt = info.blocksize / 4; cnt > 0; ++ptr, --cnt)
		chksum += be32_to_cpu(*ptr);
	return chksum;
}

