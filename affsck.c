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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "amigaffs.h"


#define BLKGETSIZE	_IO(0x12,96)
#define BLKSSZGET	_IO(0x12,104)

struct affs_info info;
char affs_prog[] = "affsck";

#if HAVE_ARGP_H
#include <argp.h>

static error_t parse_opt(int key, char *arg, struct argp_state *state);

const char *argp_program_version = "affsck " VERSION;

static char args_doc[] = "device";

static struct argp_option argo[] = {
	{ "verbose",	'v',	0,		0,	"Be verbose" },
	{ "root",	'b',	"block",	0,	"Specify root block" },
	{ "size",	's',	"size",		0,	"Force blocksize" },
	{ "reserve",	'r',	"blocks",	0,	"Set reserved blocks" },
	{ "readonly",	'n',	0,		0,	"No changes to the filesystem" },
	{ "force",	'f',	0,		0,	"Force filesystem check" },
	{ "clear",	'c',	0,		0,	"Clear bitmap flag" },
	{ "write",	'w',	0,		0,	"Write bitmap" },
	{ 0 }
};

static struct argp argp = {
	argo,
	parse_opt,
	args_doc,
	NULL,
};
#else
struct argp_state;
typedef int error_t;

static void argp_usage(struct argp_state *state)
{
	fprintf(stderr,"Usage: affsck [-fvncw] [-b root] [-s blocksize] [-r reserved] devicefile\n");
	exit(1);
}
#endif

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'v':
		info.verbose++;
		break;
	case 'b':
		info.root = atoi(arg);
		break;
	case 's':
		info.blocksize = atoi(arg);
		switch (info.blocksize) {
		case 512: case 1024: case 2048: case 4096:
			break;
		default:
			affs_error("invalid block size %d\n", info.blocksize);
			exit(1);
		}
		break;
	case 'n':
		info.read = 1;
		break;
	case 'f':
		info.force = 1;
		break;
	case 'r':
		info.reserved = atoi(arg);
		if (!info.reserved) {
			affs_error("at least 1 block must reserved\n");
			exit(1);
		}
		break;
	case 'c':
		info.clear = 1;
		break;
	case 'w':
		info.write = 1;
		break;
#if HAVE_ARGP_H
	case ARGP_KEY_ARG:
		if (state->arg_num >= 1)
			argp_usage(state);
		info.device = arg;
		break;
	case ARGP_KEY_NO_ARGS:
		argp_usage(state);
	default:
		return ARGP_ERR_UNKNOWN;
#else
	default:
		affs_error("unknown option '%c'\n", optopt);
	case '?':
		argp_usage(state);
#endif
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct stat stat;
	struct affs_root_tail *root_tail;

	memset(&info, 0, sizeof(info));
	info.reserved = 2;

#if HAVE_ARGP_H
	if (argp_parse(&argp, argc, argv, 0, 0, &info))
		return 1;
#else
{
	int c;
	while ((c = getopt (argc, argv, "vb:s:r:nf")) != -1) {
		parse_opt(c, optarg, NULL);
	}
	if (optind >= argc) {
		affs_error("devicefile missing\n");
		argp_usage(NULL);
	}
	info.device = argv[optind];
}
#endif

	info.devfd = open(info.device, info.read ? O_RDONLY : O_RDWR);
	if (info.devfd < 0) {
		perror("open");
		return 1;
	}

	if (fstat(info.devfd, &stat)) {
		perror("stat");
		return 1;
	}

	if (S_ISREG(stat.st_mode)) {
		info.blocks = stat.st_size / 512;
	} else if (S_ISBLK(stat.st_mode)) {
		ioctl(info.devfd, BLKGETSIZE, &info.blocks);
	} else {
		affs_error("'%s' isn't a valid device\n", info.device);
		return 1;
	}

	if (affs_find_root())
		return 1;

	root_tail = AFFS_ROOT_TAIL(affs_rootbuf);

	if (info.clear) {
		root_tail->bitmap_flag = 0;
		affs_write_root();
		return 0;
	}

	if (affs_detect_type())
		return 1;

	affs_print(0, "detected a ");
	if (info.intl)
		affs_print(0, "international ");
	if (info.dcache)
		affs_print(0, "dircache ");
	if (info.mufs)
		affs_print(0, "multiuser ");
	affs_print(0, "%s amiga filesystem%s\n", info.ofs ? "old" : "fast",
		   root_tail->bitmap_flag ? "" : " (not cleanly unmounted)");

	info.datablocksize = info.blocksize;
	if (info.ofs)
		info.datablocksize -= 6 * 4;

	if (affs_read_bitmap())
		return 1;

	if (info.dcache)
		affs_read_dcache(be32_to_cpu((root_tail->dcache)));

	if (affs_read_dir(AFFS_ROOT_HEAD(affs_rootbuf)->hashtable))
		return 1;

	affs_cmp_bitmap();

	if (info.write) {
		affs_write_bitmap();
		affs_write_root();
	}

	return 0;
}
