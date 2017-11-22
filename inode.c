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
#include <string.h>
#include <time.h>

#include "amigaffs.h"   

int affs_detect_type(void)
{
	u32 reserved = info.reserved;

	/* reset reserved so we can read the first block */
	info.reserved = 0;

	if (affs_bread(affs_databuf, 0)) {
		info.reserved = reserved;
		return 1;
	}

	info.reserved = reserved;

	switch (be32_to_cpu(*(u32 *)affs_databuf)) {
	case MUFS_FS:
	case MUFS_INTLFFS:
		info.mufs = 1;
		/* fall thru */
	case FS_INTLFFS:
		info.intl = 1;
		break;

	case MUFS_DCFFS:
		info.mufs = 1;
		/* fall thru */
	case FS_DCFFS:
		info.dcache = 1;
		break;

	case MUFS_FFS:
		info.mufs = 1;
		/* fall thru */
	case FS_FFS:
		break;

	case MUFS_OFS:
		info.mufs = 1;
		/* fall thru */
	case FS_OFS:
		info.ofs = 1;
		break;

	case MUFS_DCOFS:
		info.mufs = 1;
		/* fall thru */
	case FS_DCOFS:
		info.dcache = 1;
		info.ofs = 1;
		break;

	case MUFS_INTLOFS:
		info.mufs = 1;
		/* fall thru */
	case FS_INTLOFS:
		info.intl = 1;
		info.ofs = 1;
		break;

	default:
		return 1;
	}

	return 0;
}

int affs_write_type(void)
{
	u32 reserved = info.reserved;
	u32 type;

	/* reset reserved so we can read the first block */
	info.reserved = 0;

	if (affs_bread(affs_databuf, 0)) {
		info.reserved = reserved;
		return 1;
	}

	if (info.mufs) {
		if (info.ofs) {
			if (info.intl)
				type = MUFS_INTLOFS;
			else if (info.dcache)
				type = MUFS_DCOFS;
			else
				type = MUFS_OFS;
		} else {
			if (info.intl)
				type = MUFS_INTLFFS;
			else if (info.dcache)
				type = MUFS_DCFFS;
			else
				type = MUFS_FFS;
		}
	} else {
		if (info.ofs) {
			if (info.intl)
				type = FS_INTLOFS;
			else if (info.dcache)
				type = FS_DCOFS;
			else
				type = FS_OFS;
		} else {
			if (info.intl)
				type = FS_INTLFFS;
			else if (info.dcache)
				type = FS_DCFFS;
			else
				type = FS_FFS;
		}
	}

	*(u32 *)affs_databuf = cpu_to_be32(type);
	affs_bwrite(affs_databuf, 0);

	info.reserved = reserved;

	return 0;
}

int affs_find_root(void)
{
	u32 orig_size;
	u32 root = info.blocks / 2;
	int i = 8;
	struct affs_root_head *head = AFFS_ROOT_HEAD(affs_rootbuf);
	struct affs_root_tail *tail;

	/* remember blocksize if it was specified */
	orig_size = info.blocksize;

	/* if a root block was specified only try that */
	if (info.root) {
		root = info.root;
		if (info.blocksize)
			root *= info.blocksize / 512;
		i = 1;
	}

	for (; i > 0; ++root, --i) {
		/* initialize values needed for affs_bread */
		info.blocksize = AFFS_BLOCKSIZE_MIN;
		info.blockshift = AFFS_BLOCKSHIFT_MIN;
		if (affs_bread(affs_rootbuf, root))
			break;

	recheck:
		if (be32_to_cpu(head->primary_type) == T_SHORT &&
		    be32_to_cpu(head->spare1) == 0 &&
		    be32_to_cpu(head->spare2) == 0 &&
		    be32_to_cpu(head->spare3) == 0) {
			info.blocksize = (be32_to_cpu(head->hash_size) + 56) * 4;
			/* accept only if calculated and specified blocksize agree */
			if (orig_size && info.blocksize != orig_size)
				continue;
			switch (info.blocksize) {
			case 512:
				info.blockshift = 9;
				break;
			case 1024:
				info.blockshift = 10;
				break;
			case 2048:
				info.blockshift = 11;
				break;
			case 4096:
				info.blockshift = 12;
				break;
			default:
				continue;
			}
			info.root = root >> (info.blockshift - AFFS_BLOCKSHIFT_MIN);
			if (info.blocksize > AFFS_BLOCKSIZE_MIN) {
				/* reread root block and check again
				 * (this detects a misaligned block)
				 */
				affs_bread(affs_rootbuf, root);
				goto recheck;
			}
			tail = AFFS_ROOT_TAIL(affs_rootbuf);
			if (be32_to_cpu(tail->secondary_type) == ST_ROOT &&
			    affs_checksum(affs_rootbuf) == 0) {
				affs_print(1, "root block found at %d\n", info.root);
				info.blocks = info.blocks >> (info.blockshift - AFFS_BLOCKSHIFT_MIN);
				return 0;
			}
		}
	}

	affs_error("unable to find root block\n");
	return 1;
}

void affs_init_root(void)
{
	struct affs_root_head *head = AFFS_ROOT_HEAD(affs_rootbuf);
	struct affs_root_tail *tail = AFFS_ROOT_TAIL(affs_rootbuf);
	struct affs_date date;
	int size;
	time_t curtime;

	memset(affs_rootbuf, 0, info.blocksize);
	head->primary_type = cpu_to_be32(T_SHORT);
	head->hash_size = cpu_to_be32((info.blocksize / 4) - 56);

	memset(&date, 0, sizeof(date));
	curtime = time(NULL);
	if (curtime > (8 * 365 + 2) * 24 * 60 * 60) {
		curtime -= (8 * 365 + 2) * 24 * 60 * 60;
		date.days = cpu_to_be32(curtime / (24 * 60 * 60));
		curtime -= be32_to_cpu(date.days) * (24 * 60 * 60);
		date.mins = cpu_to_be32(curtime / 60);
		curtime -= be32_to_cpu(date.mins) * 60;
		date.ticks = cpu_to_be32(curtime * 50);
	}
	tail->root_change = tail->disk_change = tail->disk_create = date;

	size = strlen(info.name);
	if (size > 30) {
		affs_error("name exceeds max name length of 30 characters\n");
		size = 30;
	}
	tail->disk_name[0] = size;
	strncpy(tail->disk_name + 1, info.name, size);
	tail->secondary_type = cpu_to_be32(ST_ROOT);
}

int affs_write_root(void)
{
	struct affs_root_head *head = AFFS_ROOT_HEAD(affs_rootbuf);

	head->checksum = 0;
	head->checksum = cpu_to_be32(-affs_checksum(affs_rootbuf));

	affs_print(1, "writing root block at %d\n", info.root);
	return affs_bwrite(affs_rootbuf, info.root);
}

void affs_print_link(u_char *buf)
{
	struct affs_dir_head *head = AFFS_DIR_HEAD(buf);
	struct affs_dir_tail *tail = AFFS_DIR_TAIL(buf);
	time_t time;
	u8 date[] = "                         ";

	affs_print(1, "%10u ", be32_to_cpu(head->own_key));
	affs_print(1, "%-30.*s ", tail->dir_name[0], tail->dir_name + 1);
	switch (be32_to_cpu(tail->secondary_type)) {
	case ST_LINKFILE:
		affs_print(1, "<FILELINK> ");
		break;
	case ST_LINKDIR:
		affs_print(1, " <LINKDIR> ");
		break;
	case ST_SOFTLINK:
		affs_print(1, "<SOFTLINK> ");
		break;
	default:
		affs_print(1, "     <???> ");
		break;
	}
	time = (be32_to_cpu(tail->dir_change.days) * (24 * 60 * 60)) +
	       (be32_to_cpu(tail->dir_change.mins) * 60) +
	       (be32_to_cpu(tail->dir_change.ticks) / 50) +
	       ((8 * 365 + 2) * 24 * 60 * 60);
	strftime(date, sizeof(date) - 1, "%a %b %e %T %Y ", localtime(&time));
	affs_print(1, date);
	// protection bits
	affs_print(1, " %*s\n", tail->comment[0], tail->comment + 1);
}

void affs_print_file(u_char *buf)
{
	struct affs_file_head *head = AFFS_FILE_HEAD(buf);
	struct affs_file_tail *tail = AFFS_FILE_TAIL(buf);
	time_t time;
	u8 date[] = "                         ";

	affs_print(1, "%10u ", be32_to_cpu(head->own_key));
	affs_print(1, "%-30.*s ", tail->file_name[0], tail->file_name + 1);
	affs_print(1, "%10u ", be32_to_cpu(tail->byte_size));
	time = (be32_to_cpu(tail->file_change.days) * (24 * 60 * 60)) +
	       (be32_to_cpu(tail->file_change.mins) * 60) +
	       (be32_to_cpu(tail->file_change.ticks) / 50) +
	       ((8 * 365 + 2) * 24 * 60 * 60);
	strftime(date, sizeof(date) - 1, "%a %b %e %T %Y ", localtime(&time));
	affs_print(1, date);
	// protection bits
	affs_print(1, "%*s\n", tail->comment[0], tail->comment + 1);
}

int affs_read_file(u_char *buf)
{
	struct affs_file_head *head = AFFS_FILE_HEAD(buf);
	struct affs_file_tail *tail = AFFS_FILE_TAIL(buf);
	int i;
	u32 entry, block, block_cnt;

	entry = be32_to_cpu(head->own_key);
	block_cnt = (be32_to_cpu(tail->byte_size) + info.datablocksize - 1) / info.datablocksize;
	do {
		block = be32_to_cpu(head->block_count);
		if ((block_cnt > AFFS_BLOCKTABLESIZE && block != AFFS_BLOCKTABLESIZE) ||
		    (block_cnt <= AFFS_BLOCKTABLESIZE && block != block_cnt)) {
			affs_error("wrong blockcount %d in %d\n", block, entry);
		}
		for (i = AFFS_BLOCKTABLESIZE - 1; i >= 0; --i) {
			block = be32_to_cpu(head->blocktable[i]);
			if (!block && !block_cnt)
				continue;
			if (!block_cnt) {
				affs_error("block %d exceeds file size\n", block);
				continue;
			}
			affs_print(2, " [%u]", block);
			affs_alloc_block(block);
			block_cnt--;
		}
		entry = be32_to_cpu(tail->extension);
		if (entry) {
			affs_bread(buf, entry);
			if (!block_cnt)
				affs_error("extended block %d exceeds file size\n", block);
			else {
				affs_alloc_block(entry);
				affs_print(2, "\n [ext:%u]", entry);
			}
		}
	} while (entry);
	affs_print(2, "\n");
	return 0;
}

void affs_print_dir(u_char *buf)
{
	struct affs_dir_head *head = AFFS_DIR_HEAD(buf);
	struct affs_dir_tail *tail = AFFS_DIR_TAIL(buf);
	time_t time;
	char date[] = "                         ";

	affs_print(1, "%10u ", be32_to_cpu(head->own_key));
	affs_print(1, "%-30.*s ", tail->dir_name[0], tail->dir_name + 1);
	affs_print(1, "     <DIR> ");
	time = (be32_to_cpu(tail->dir_change.days) * (24 * 60 * 60)) +
	       (be32_to_cpu(tail->dir_change.mins) * 60) +
	       (be32_to_cpu(tail->dir_change.ticks) / 50) +
	       ((8 * 365 + 2) * 24 * 60 * 60);
	strftime(date, sizeof(date) - 1, "%a %b %e %T %Y ", localtime(&time));
	affs_print(1, date);
	// protection bits
	affs_print(1, " %*s\n", tail->comment[0], tail->comment + 1);
}

int affs_read_dcache(u32 block)
{
	struct affs_dcache_head *head;
	u8 buf[AFFS_BLOCKSIZE_MAX];

	head = AFFS_DCACHE_HEAD(buf);
	while (block) {
		if (affs_bread(buf, block))
			return 1;
		if (affs_checksum(buf)) {
			affs_error("dcache entry %d has invalid checksum\n", block);
			return 1;
		}
		if (be32_to_cpu(head->own_key) != block)
			affs_error("dcache entry %d has key\n", block);
		affs_print(2, " [%u]", block);
		affs_alloc_block(block);
		block = be32_to_cpu(head->next);
	}
	affs_print(2, "\n");
	return 0;
}

int affs_read_dir(u32 *hashtable)
{
	u32 entry;
	u8 buf[AFFS_BLOCKSIZE_MAX];
	int i;

	for (i = 0; i < AFFS_HASHTABLESIZE; ++i) {
		entry = be32_to_cpu(hashtable[i]);
		while (entry) {
			affs_bread(buf, entry);
			if (affs_checksum(buf)) {
				affs_error("dir entry %d has invalid checksum (%x)\n", entry, affs_checksum(buf));
				entry = 0;
				continue;
			}
			if (be32_to_cpu(AFFS_PTYPE(buf)) != T_SHORT) {
				affs_error("dir entry %d has invalid primary type %d\n",
					entry, be32_to_cpu(AFFS_PTYPE(buf)));
				entry = 0;
				continue;
			}
			switch (be32_to_cpu(AFFS_STYPE(buf))) {
			case ST_ROOT:
				affs_error("dir entry %d has root type\n", entry);
				entry = 0;
				break;
			case ST_USERDIR: {
				struct affs_dir_head *head = AFFS_DIR_HEAD(buf);
				struct affs_dir_tail *tail = AFFS_DIR_TAIL(buf);
				if (be32_to_cpu(head->own_key) != entry) {
					affs_error("wrong header key (%u, %u)\n", be32_to_cpu(head->own_key), entry);
				}
				affs_print_dir(buf);
				affs_alloc_block(entry);
				if (info.dcache)
					affs_read_dcache(be32_to_cpu(tail->dcache));
				entry = be32_to_cpu(tail->hash_chain);
				affs_read_dir(head->hashtable);
				break;
			}
			case ST_SOFTLINK: {
				struct affs_dir_tail *tail = AFFS_DIR_TAIL(buf);
				affs_print_link(buf);
				affs_alloc_block(entry);
				entry = be32_to_cpu(tail->hash_chain);
				break;
			}
			case ST_LINKDIR: {
				struct affs_dir_tail *tail = AFFS_DIR_TAIL(buf);
				affs_print_link(buf);
				affs_alloc_block(entry);
				entry = be32_to_cpu(tail->hash_chain);
				break;
			}
			case ST_FILE: {
				struct affs_file_head *head = AFFS_FILE_HEAD(buf);
				struct affs_file_tail *tail = AFFS_FILE_TAIL(buf);
				if (be32_to_cpu(head->own_key) != entry)
					affs_error("wrong header key (%u, %u)\n", be32_to_cpu(head->own_key), entry);
				affs_print_file(buf);
				affs_alloc_block(entry);
				entry = be32_to_cpu(tail->hash_chain);
				affs_read_file(buf);
				break;
			}
			case ST_LINKFILE: {
				struct affs_file_tail *tail = AFFS_FILE_TAIL(buf);
				affs_print_link(buf);
				affs_alloc_block(entry);
				entry = be32_to_cpu(tail->hash_chain);
				break;
			}
			}
		}
	}
	return 0;
}
