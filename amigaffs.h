/*
 *  Copyright (C) 2000  Roman Zippel
 *
 *  originaly based on /usr/src/linux/include/linux/amigaffs.h
 *  (Copyright missing there, but I think it's safe to assume GPL)
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

#ifndef AMIGAFFS_H
#define AMIGAFFS_H
#include <stdint.h>

#define AFFS_BLOCKSIZE_MIN	512
#define AFFS_BLOCKSHIFT_MIN	9
#define AFFS_BLOCKSIZE_MAX	4096
#define AFFS_BLOCKSHIFT_MAX	12

#define AFFS_ROOT_BMAPS		25

struct affs_info {
	char *name;
	char *device;
	int devfd;
	u32 reserved;
	u32 root;
	u32 blocksize;
	u32 datablocksize;
	u32 blockshift;
	u32 blocks;
	u32 lastalloc;
	int verbose;
	struct {
		/* # of errors during bitmap read */
		u32 bitmap_block;
		/* # of blocks not marked free in bitmap */
		u32 bitmap_free;
		/* # of blocks not allocated in bitmap */
		u32 bitmap_alloc;
	} errstat;
	int read : 1;
	int force : 1;
	int clear : 1;
	int write : 1;
	int ofs : 1;
	int mufs : 1;
	int intl : 1;
	int dcache : 1;
};


struct affs_date {
	u32 days;
	u32 mins;
	u32 ticks;
};

struct affs_short_date {
	u16 days;
	u16 mins;
	u16 ticks;
};

struct affs_root_head {
	u32 primary_type;
	u32 spare1;
	u32 spare2;
	u32 hash_size;
	u32 spare3;
	u32 checksum;
	u32 hashtable[1];
};

struct affs_root_tail {
	u32 bitmap_flag;
	u32 bitmap_blk[AFFS_ROOT_BMAPS];
	u32 bitmap_ext;
	struct affs_date root_change;
	u8 disk_name[32];
	u32 spare1;
	u32 spare2;
	struct affs_date disk_change;
	struct affs_date disk_create;
	u32 spare3;
	u32 spare4;
	u32 dcache;
	u32 secondary_type;
};

struct affs_dir_head {
	u32 primary_type;
	u32 own_key;
	u32 spare1;
	u32 spare2;
	u32 spare3;
	u32 checksum;
	u32 hashtable[1];
};

struct affs_dir_tail {
	u32 spare1;
	u16 owner_uid;
	u16 owner_gid;
	u32 protect;
	u32 spare2;
	u8 comment[92];
	struct affs_date dir_change;
	u8 dir_name[32];
	u32 spare3;
	u32 original;
	u32 link_chain;
	u32 spare[5];
	u32 hash_chain;
	u32 parent;
	u32 dcache;
	u32 secondary_type;
};

struct affs_file_head {
	u32 primary_type;
	u32 own_key;
	u32 block_count;
	u32 spare1;
	u32 first_data;
	u32 checksum;
	u32 blocktable[1];
};

struct affs_file_tail {
	u32 spare1;
	u16 owner_uid;
	u16 owner_gid;
	u32 protect;
	u32 byte_size;
	u8 comment[92];
	struct affs_date file_change;
	u8 file_name[32];
	u32 spare2;
	u32 original;
	u32 link_chain;
	u32 spare[5];
	u32 hash_chain;
	u32 parent;
	u32 extension;
	u32 secondary_type;
};

struct affs_data_head {
	u32 primary_type;
	u32 header_key;
	u32 sequence_number;
	u32 data_size;
	u32 next_data;
	u32 checksum;
	u32 data[1];
};

struct affs_dcache_entry {
	u32 key;
	u32 byte_size;
	u32 protect;
	u32 spare;
	struct affs_short_date change;
	u8 secondary_type;
	u8 name[1];
};

struct affs_dcache_head {
	u32 primary_type;
	u32 own_key;
	u32 parent;
	u32 dcache_count;
	u32 next;
	u32 checksum;
	struct affs_dcache_entry entry[1];
};

struct affs_symlink_head {
	u32 primary_type;
	u32 own_key;
	u32 spare1;
	u32 spare2;
	u32 spare3;
	u32 checksum;
	u8 name[1];
};

extern struct affs_info info;
extern char affs_prog[];

/* bitmap.c */
extern u8 *affs_new_bitmap;
extern u8 *affs_old_bitmap;

extern int affs_alloc_block(u32 block);
extern u32 affs_alloc_new_block(void);
extern int affs_test_block(u32 block);
extern int affs_read_bitmap(void);
extern int affs_write_bitmap(void);
extern u32 affs_cmp_bitmap(void);
extern int affs_create_bitmap(void);

/* buffer.c */
extern u_char affs_rootbuf[AFFS_BLOCKSIZE_MAX];
extern u_char affs_databuf[AFFS_BLOCKSIZE_MAX];

extern int affs_bread(void *data, u32 block);
extern int affs_bwrite(void *data, u32 block);
extern u32 affs_checksum(void *data);

/* inode.c */
extern int affs_detect_type(void);
extern int affs_write_type(void);
extern int affs_find_root(void);
extern void affs_init_root(void);
extern int affs_write_root(void);
extern void affs_print_link(u8 *buf);
extern void affs_print_file(u8 *buf);
extern int affs_read_file(u8 *buf);
extern void affs_print_dir(u8 *buf);
extern int affs_read_dcache(u32 block);
extern int affs_read_dir(u32 *hashtable);

/* util.c */
extern void affs_print(int level, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
extern void affs_error(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));


#ifdef WORDS_BIGENDIAN
#define be32_to_cpu(x)	(x)
#define cpu_to_be32(x)	(x)
#else
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#else
#define bswap_32(x)	((((x)>>24)&0xff)|(((x)>>8)&0xff00)|(((x)<<8)&0xff0000)|((x)<<24))
#endif
#define be32_to_cpu(x)	bswap_32(x)
#define cpu_to_be32(x)	bswap_32(x)
#endif


#define AFFS_ROOT_HEAD(buf)	((struct affs_root_head *)buf)
#define AFFS_ROOT_TAIL(buf)	((struct affs_root_tail *)((uintptr_t)buf+info.blocksize-sizeof(struct affs_root_tail)))

#define AFFS_DIR_HEAD(buf)	((struct affs_dir_head *)buf)
#define AFFS_DIR_TAIL(buf)	((struct affs_dir_tail *)((uintptr_t)buf+info.blocksize-sizeof(struct affs_dir_tail)))

#define AFFS_DCACHE_HEAD(buf)	((struct affs_dcache_head *)buf)

#define AFFS_FILE_HEAD(buf)	((struct affs_file_head *)buf)
#define AFFS_FILE_TAIL(buf)	((struct affs_file_tail *)((uintptr_t)buf+info.blocksize-sizeof(struct affs_file_tail)))


#define AFFS_HASHTABLESIZE	((info.blocksize/4)-56)
#define AFFS_BLOCKTABLESIZE	((info.blocksize/4)-56)


#define AFFS_PTYPE(buf)		(*(u_int *)buf)
#define AFFS_STYPE(buf)		(*(u_int *)((uintptr_t)buf+info.blocksize-4))


#define FS_OFS		0x444F5300
#define FS_FFS		0x444F5301
#define FS_INTLOFS	0x444F5302
#define FS_INTLFFS	0x444F5303
#define FS_DCOFS	0x444F5304
#define FS_DCFFS	0x444F5305
#define MUFS_FS		0x6d754653   /* 'muFS' */
#define MUFS_OFS	0x6d754600   /* 'muF\0' */
#define MUFS_FFS	0x6d754601   /* 'muF\1' */
#define MUFS_INTLOFS	0x6d754602   /* 'muF\2' */
#define MUFS_INTLFFS	0x6d754603   /* 'muF\3' */
#define MUFS_DCOFS	0x6d754604   /* 'muF\4' */
#define MUFS_DCFFS	0x6d754605   /* 'muF\5' */


#define T_SHORT		2
#define T_DATA		8
#define T_LIST		16
#define T_DCACHE	17

#define ST_LINKFILE	-4
#define ST_FILE		-3
#define ST_ROOT		1
#define ST_USERDIR	2
#define ST_SOFTLINK	3
#define ST_LINKDIR	4

/* Permission bits */

#define FIBF_OTR_READ		0x8000
#define FIBF_OTR_WRITE		0x4000
#define FIBF_OTR_EXECUTE	0x2000
#define FIBF_OTR_DELETE		0x1000
#define FIBF_GRP_READ		0x0800
#define FIBF_GRP_WRITE		0x0400
#define FIBF_GRP_EXECUTE	0x0200
#define FIBF_GRP_DELETE		0x0100

#define FIBF_SCRIPT		0x0040
#define FIBF_PURE		0x0020		/* no use under linux */
#define FIBF_ARCHIVE		0x0010		/* never set, always cleared on write */
#define FIBF_READ		0x0008		/* 0 means allowed */
#define FIBF_WRITE		0x0004		/* 0 means allowed */
#define FIBF_EXECUTE		0x0002		/* 0 means allowed, ignored under linux */
#define FIBF_DELETE		0x0001		/* 0 means allowed */

#define FIBF_OWNER		0x000F		/* Bits pertaining to owner */

#define AFFS_UMAYWRITE(prot)	(((prot) & (FIBF_WRITE|FIBF_DELETE)) == (FIBF_WRITE|FIBF_DELETE))
#define AFFS_UMAYREAD(prot)	((prot) & FIBF_READ)
#define AFFS_UMAYEXECUTE(prot)	((prot) & FIBF_EXECUTE)
#define AFFS_GMAYWRITE(prot)	(((prot)&(FIBF_GRP_WRITE|FIBF_GRP_DELETE))==\
							(FIBF_GRP_WRITE|FIBF_GRP_DELETE))
#define AFFS_GMAYREAD(prot)	((prot) & FIBF_GRP_READ)
#define AFFS_GMAYEXECUTE(prot)	((prot) & FIBF_EXECUTE)
#define AFFS_OMAYWRITE(prot)	(((prot)&(FIBF_OTR_WRITE|FIBF_OTR_DELETE))==\
							(FIBF_OTR_WRITE|FIBF_OTR_DELETE))
#define AFFS_OMAYREAD(prot)	((prot) & FIBF_OTR_READ)
#define AFFS_OMAYEXECUTE(prot)	((prot) & FIBF_EXECUTE)

#endif
