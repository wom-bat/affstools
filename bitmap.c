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

#include <stdlib.h>
#include <stdio.h>

#include "amigaffs.h"


u8 *affs_new_bitmap;
u8 *affs_old_bitmap;

int affs_alloc_block(u32 block)
{
	u8 *ptr;
	u8 mask;

	if (block < info.reserved || block >= info.blocks) {
		affs_error("can't allocate block %d (block is %s)\n", block,
			block < info.reserved ? "reserved" : "out of range");
		return 1;
	}

	block -= info.reserved;
	ptr = affs_new_bitmap + ((block / 8) ^ 3);
	mask = 1 << (block & 7);

	if (*ptr & mask) {
		*ptr &= ~mask;
		return 0;
	}

	affs_error("block %d already allocated\n", block + info.reserved);
	return 1;
}


u32 affs_alloc_new_block(void)
{
	u32 i, last, block = info.lastalloc - info.reserved;
	u8 mask, byte;
	u8 *ptr;

	last = info.blocks - info.reserved;
	i = block & 7;
	if (i) {
		block &= -8;
		ptr = affs_new_bitmap + ((block / 8) ^ 3);
		if ((byte = *ptr)) {
			mask = 1 << i;
			goto into_loop;
		}
	}
again:
	for (; block < last; block += 8) {
		ptr = affs_new_bitmap + ((block / 8) ^ 3);
		if (!(byte = *ptr))
			continue;
		mask = 1;
		i = 0;
	into_loop:
		for (; i < 8; mask <<= 1, ++i) {
			if ((byte & mask) && (block + i < last))
				goto done;
		}
	}
	if (last == info.blocks - info.reserved) {
		last = info.root - info.reserved;
		goto again;
	}
	return 0;
done:
	*ptr = byte ^ mask;
	block += i + info.reserved;
	info.lastalloc = block;
	return block; 
}

int affs_test_block(u32 block)
{
	u8 *ptr;
	u8 mask;

	if (block < info.reserved || block >= info.blocks)
		return 1;

	block -= info.reserved;
	ptr = affs_new_bitmap + ((block / 8) ^ 3);
	mask = 1 << (block & 7);

	if (*ptr & mask)
		return 0;

	return 1;
}


int affs_read_bitmap(void)
{
	struct affs_root_tail *tail = AFFS_ROOT_TAIL(affs_rootbuf);
	u32 bitmap_blocks, blocks, bits;
	u32 extmap[AFFS_BLOCKSIZE_MAX/4];
	u32 ext, i, size;
	u8 *ptr;

	/* bit number in bitmap block */
	bits = (info.blocksize - 4) * 8;
	/* # of bitmap blocks */
	bitmap_blocks = (info.blocks - info.reserved + bits - 1) / bits;
	size = bitmap_blocks * info.blocksize;

	affs_new_bitmap = malloc(size);
	affs_old_bitmap = malloc(size);
	if (!affs_old_bitmap || !affs_new_bitmap) {
		affs_error("unable to allocate bitmap\n");
		return 1;
	}
	memset(affs_old_bitmap, 0xff, size);
	memset(affs_new_bitmap, 0xff, size);
	ptr = affs_old_bitmap;

	affs_alloc_block(info.root);
	info.lastalloc = info.root;

	blocks = AFFS_ROOT_BMAPS;
	if (bitmap_blocks < AFFS_ROOT_BMAPS)
		blocks = bitmap_blocks;
	for (i = 0; i < blocks; ptr += info.blocksize - 4, ++i) {
		if (affs_bread(affs_databuf, be32_to_cpu(tail->bitmap_blk[i]))) {
			info.errstat.bitmap_block++;
			continue;
		}
		if (affs_checksum(affs_databuf)) {
			info.errstat.bitmap_block++;
			continue;
		}
		affs_alloc_block(be32_to_cpu(tail->bitmap_blk[i]));
		affs_print(2, "read bitmap %d : %d\n", i, be32_to_cpu(tail->bitmap_blk[i]));
		memcpy(ptr, affs_databuf + 4, info.blocksize - 4);
	}

	if (bitmap_blocks <= AFFS_ROOT_BMAPS)
		return 0;
	bitmap_blocks -= AFFS_ROOT_BMAPS;

	ext = be32_to_cpu(tail->bitmap_ext); 
	while (ext) {
		if (affs_bread(extmap, ext)) {
			info.errstat.bitmap_block++;
			break;
		}
		affs_alloc_block(ext);
		affs_print(2, "read ext bitmap: %u\n", ext);

		blocks = (info.blocksize - 4) / 4;
		if (bitmap_blocks < blocks)
			blocks = bitmap_blocks;
		for (i = 0; i < blocks; ptr += info.blocksize - 4, ++i) {
			if (affs_bread(affs_databuf, be32_to_cpu(extmap[i]))) {
				info.errstat.bitmap_block++;
				continue;
			}
			if (affs_checksum(affs_databuf)) {
				info.errstat.bitmap_block++;
				continue;
			}
			affs_alloc_block(be32_to_cpu(extmap[i]));
			affs_print(2, "read bitmap %d: %u\n", i, be32_to_cpu(extmap[i]));
			memcpy(ptr, affs_databuf + 4, info.blocksize - 4);
		}

		ext = be32_to_cpu(extmap[i]);
		if (!ext && bitmap_blocks > blocks) {
			affs_error("bitmap blocks missing\n");
			info.errstat.bitmap_block++;
			return 1;
		}
		bitmap_blocks -= blocks;
	}

	return 0;
}

static void inline affs_set_bitmap_checksum(void *data)
{
	u32 *buf = data;

	*buf = 0;
	*buf = cpu_to_be32(-affs_checksum(buf));
}

int affs_write_bitmap(void)
{
	struct affs_root_tail *tail = AFFS_ROOT_TAIL(affs_rootbuf);
	u32 bitmap_blocks, blocks, bits;
	u32 extmap[AFFS_BLOCKSIZE_MAX/4];
	u32 ext, i;
	u8 *ptr;

	if (info.errstat.bitmap_block) {
		affs_error("error in bitmap. abort writing bitmap\n");
		return 1;
	}

	/* bit number in bitmap block */
	bits = (info.blocksize - 4) * 8;
	/* # of bitmap blocks */
	bitmap_blocks = (info.blocks - info.reserved + bits - 1) / bits;

	ptr = affs_new_bitmap;
	blocks = AFFS_ROOT_BMAPS;
	if (bitmap_blocks < AFFS_ROOT_BMAPS)
		blocks = bitmap_blocks;
	for (i = 0; i < blocks; ptr += info.blocksize - 4, ++i) {
		memcpy(affs_databuf + 4, ptr, info.blocksize - 4);
		affs_set_bitmap_checksum(affs_databuf);
		affs_print(2, "write bitmap %d : %d\n", i, be32_to_cpu(tail->bitmap_blk[i]));
		affs_bwrite(affs_databuf, be32_to_cpu(tail->bitmap_blk[i]));
	}

	if (bitmap_blocks <= AFFS_ROOT_BMAPS)
		goto done;
	bitmap_blocks -= AFFS_ROOT_BMAPS;

	ext = be32_to_cpu(tail->bitmap_ext);
	while (ext) {
		if (affs_bread(extmap, ext))
			break;
		blocks = (info.blocksize - 4) / 4;
		if (bitmap_blocks < blocks)
			blocks = bitmap_blocks;
		for (i = 0; i < blocks; ptr += info.blocksize - 4, ++i) {
			memcpy(affs_databuf + 4, ptr, info.blocksize - 4);
			affs_set_bitmap_checksum(affs_databuf);
			affs_print(2, "write bitmap %d: %u\n", i, be32_to_cpu(extmap[i]));
			affs_bwrite(affs_databuf, be32_to_cpu(extmap[i]));
		}

		ext = be32_to_cpu(extmap[i]);
		if (bitmap_blocks <= blocks)
			break;
		bitmap_blocks -= blocks;
	}

done:
	tail->bitmap_flag = cpu_to_be32(-1);
	return 0;
}

u32 affs_cmp_bitmap(void)
{
	u32 i, size, block, err;
	u8 mask;

	affs_print(2, "bitmap check: ");
	err = 0;
	size = (info.blocks - info.reserved - 1) / 8;
	for (i = 0; i <= size; ++i) {
		mask = affs_new_bitmap[i ^ 3] ^ affs_old_bitmap[i ^ 3];
		if (!mask)
			continue;

		if (i == size)
			mask &= 0xff >> (8 - ((info.blocks - info.reserved) & 7));

		for (block = i * 8; mask; ++block, mask >>= 1) {
			if (mask & 1) {
				if (affs_test_block(block + info.reserved)) {
					affs_print(2, "<%d> ", block + info.reserved);
					info.errstat.bitmap_alloc++;
				} else {
					affs_print(2, "{%d} ", block + info.reserved);
					info.errstat.bitmap_free++;
				}
				err++;
			}
		}
	}
	affs_print(2, "\n");
	return err;
}

int affs_create_bitmap(void)
{
	struct affs_root_tail *tail = AFFS_ROOT_TAIL(affs_rootbuf);
	u32 bitmap_blocks, blocks, bits;
	u32 extmap[AFFS_BLOCKSIZE_MAX/4];
	u32 ext, i, new, size;

	/* bit number in bitmap block */
	bits = (info.blocksize - 4) * 8;
	/* # of bitmap blocks */
	bitmap_blocks = (info.blocks - info.reserved + bits - 1) / bits;
	size = bitmap_blocks * info.blocksize;

	affs_new_bitmap = malloc(size);
	affs_old_bitmap = malloc(size);
	if (!affs_old_bitmap || !affs_new_bitmap) {
		affs_error("unable to allocate bitmap\n");
		return 1;
	}
	memset(affs_old_bitmap, 0xff, size);
	memset(affs_new_bitmap, 0xff, size);

	affs_alloc_block(info.root);
	info.lastalloc = info.root;

	blocks = AFFS_ROOT_BMAPS;
	if (bitmap_blocks < AFFS_ROOT_BMAPS)
		blocks = bitmap_blocks;

	for (i = 0; i < blocks; ++i) {
		new = affs_alloc_new_block();
		affs_print(2, "alloc bitmap %d at %d\n", i, new);
		tail->bitmap_blk[i] = cpu_to_be32(new);
	}

	if (bitmap_blocks <= AFFS_ROOT_BMAPS)
		return 0;
	bitmap_blocks -= AFFS_ROOT_BMAPS;

	ext = affs_alloc_new_block();
	affs_print(2, "alloc ext bitmap at %d\n", ext);
	tail->bitmap_ext = cpu_to_be32(ext);

	while (1) {
		memset(extmap, 0, info.blocksize);

		blocks = (info.blocksize - 4) / 4;
		if (bitmap_blocks < blocks)
			blocks = bitmap_blocks;
		for (i = 0; i < blocks; ++i) {
			new = affs_alloc_new_block();
			affs_print(2, "alloc bitmap %d at %d\n", i, new);
			extmap[i] = cpu_to_be32(new);
		}

		affs_bwrite(extmap, ext);
		if (bitmap_blocks <= blocks)
			break;
		ext = affs_alloc_new_block();
		affs_print(2, "alloc ext bitmap at %d\n", ext);
		extmap[i] = cpu_to_be32(ext);
	}

	return 0;
}

