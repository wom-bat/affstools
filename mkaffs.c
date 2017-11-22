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
char affs_prog[] = "mkaffs";

#if HAVE_ARGP_H
#include <argp.h>

static error_t parse_opt(int key, char *arg, struct argp_state *state);

const char *argp_program_version = "mkaffs " VERSION;

static char args_doc[] = "devicefile name";

static struct argp_option argo[] = {
	{ "verbose",	'v',	0,		0,	"Be verbose" },
	{ "root",	'b',	"rootblock",	0,	"Specify root block" },
	{ "size",	's',	"size",		0,	"Set blocksize" },
	{ "reserve",	'r',	"blocks",	0,	"Set reserved blocks" },
	{ "ofs",	'o',	0,		0,	"Old filesystem format"  },
	{ "intl",	'i',	0,		0,	"International dir format" },
	{ "dircache",	'd',	0,		0,	"Use dir cache" },
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
	fprintf(stderr,"Usage: mkaffs [-void] [-b root] [-s blocksize] [-r reserved] devicefile name\n");
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
			affs_error("invalid block size %d\n", info.blocksize);
			exit(1);
		}
		break;
	case 'r':
		info.reserved = atoi(arg);
		break;
	case 'o':
		info.ofs = 1;
		break;
	case 'i':
		info.intl = 1;
		break;
	case 'd':
		info.dcache = 1;
		break;
#if HAVE_ARGP_H
	case ARGP_KEY_ARG:
		if (state->arg_num == 0)
			info.device = arg;
		else if (state->arg_num == 1)
			info.name = arg;
		else
			argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (state->arg_num != 2)
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

	memset(&info, 0, sizeof(info));
	info.reserved = 2;
	info.blocksize = 512;
	info.blockshift = 9;

#if HAVE_ARGP_H
	if (argp_parse(&argp, argc, argv, 0, 0, &info))
		return 1;
#else
{
	int c;
	while ((c = getopt (argc, argv, "vb:s:r:nf")) != -1) {
		parse_opt(c, optarg, NULL);
	}
	if (optind > argc - 2) {
		affs_error("devicefile missing\n");
		argp_usage(NULL);
	}
	info.device = argv[optind++];
	info.name = argv[optind];
}
#endif

	info.devfd = open(info.device, O_RDWR);
	if (info.devfd < 0) {
		perror("open");
		return 1;
	}

	if (fstat(info.devfd, &stat)) {
		perror("stat");
		return 1;
	}

	if (S_ISREG(stat.st_mode)) {
		info.blocks = stat.st_size >> info.blockshift;
	} else if (S_ISBLK(stat.st_mode)) {
		ioctl(info.devfd, BLKGETSIZE, &info.blocks);
		info.blocks >>= info.blockshift - 9;
	} else {
		affs_error("'%s' isn't a valid device\n", info.device);
		return 1;
	}

	info.root = (info.blocks + info.reserved - 1) / 2;

	affs_init_root();

	printf("blocks: %d\n", info.blocks);
	printf("blocksize: %d\n", info.blocksize);
	printf("reserved blocks: %d\n", info.reserved);
	printf("rootblock: %d\n", info.root);

	affs_create_bitmap();
	affs_write_bitmap();
	affs_write_root();
	affs_write_type();

	return 0;
}
