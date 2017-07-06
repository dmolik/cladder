/*
 * Copyright 2017 Dan Molik <dan@danmolik.com>
 *
 * This file is part of Cladder
 *
 * Cladder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Cladderis distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Cladder.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cladder.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <sys/mount.h>
#include <linux/loop.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <uuid/uuid.h>
#include <sched.h>

#include "src/squashfs/squashfs_fs.h"
#include "src/squashfs/mksquashfs.h"

char  *_wrk = NULL;
char   id[37];
uid_t  uid;
struct passwd *pwd;

void _mkpnt(char *pnt)
{
	char   *tmp = malloc(64);
	struct  stat sb;
	if (_wrk == NULL) {
		_wrk = malloc(64);
		sprintf(_wrk, "/var/lib/cladder/%s", id);
		if (stat("/var/lib/cladder", &sb) != 0 || !S_ISDIR(sb.st_mode)) {
			unlink("/var/lib/cladder");
			if (mkdir("/var/lib/cladder", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0) {
				fprintf(stderr, "failed to create data dir [%s]\n", strerror(errno));
				exit(1);
			}
		}
		if (stat(_wrk, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
			unlink(_wrk);
			if (mkdir(_wrk, S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP) != 0) {
				fprintf(stderr, "failed to create wrking dir [%s]\n", strerror(errno));
				exit(1);
			}
			if (chown(_wrk, uid, -1) != 0) {
				fprintf(stderr, "failed to chown wrking dir [%s]\n", strerror(errno));
				exit(1);
			}
		}
	}

	sprintf(tmp, "%s/%s", _wrk, pnt);
	if (mkdir(tmp, S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP) != 0) {
		fprintf(stderr, "failed to create mnt point in wrking dir [%s]\n", strerror(errno));
		exit(1);
	}
	if (chown(tmp, uid, -1) != 0) {
		fprintf(stderr, "failed to chown wrking dir [%s]\n", strerror(errno));
		exit(1);
	}
	free(tmp);
}

void _mnt(char *src, char *dst, char *type, int flags, char *opts)
{
	char *tmp = malloc(64);
	sprintf(tmp, "%s/%s", _wrk, dst);
	if (mount(src, tmp, type, flags, opts) != 0) {
		fprintf(stderr, "failed to mount %s onto %s as %s [%s]\n", src, dst, type, strerror(errno));
		exit(1);
	}
	free(tmp);
}

int main (int argc, char *argv[])
{

	struct option long_opts[] = {
		{ "help",             no_argument, NULL, 'h' },
		{ "version",          no_argument, NULL, 'V' },
		{ 0, 0, 0, 0 },
	};
	for (;;) {
		int idx = 1;
		int c = getopt_long(argc, argv, "h?V", long_opts, &idx);
		if (c == -1) break;

		switch (c) {
		case 'h':
		case '?':
			printf("%s v%s\n", PACKAGE_NAME, PACKAGE_VERSION);
			printf("Usage: %s [-h?V] src dst\n",
			         PACKAGE_NAME);

			printf("Required:\n");
			printf("  src     the source directory to turn into an archive\n");
			printf("  dst     the destination directory to overlay mount tmpfs onto the archive\n");

			printf("Options:\n");
			printf("  -?, -h  show this help screen\n");
			printf("  -V      display the version\n");

			printf("Example:\n");
			printf("  %s      test  writeable\n", PACKAGE_NAME);

			printf("\n");
			exit(0);

		case 'V':
			printf("%s version: %s\n", PACKAGE_NAME, PACKAGE_VERSION);
			exit(0);

		}
	}

	if (argv[1] == NULL || argv[2] == NULL) {
		printf("please provide a src and dst, see -? for more information\n");
		exit(1);
	}
	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse_lower(uuid, id);

	pwd = getpwnam(getenv("SUDO_USER"));
	if (pwd == NULL) {
		fprintf(stderr, "not running as sudo\n");
		exit(1);
	}
	uid = pwd->pw_uid;
	_mkpnt("wrk");
	_mkpnt("sqsh");

	char *u_opt = malloc(64);
	sprintf(u_opt, "size=134217728,mode=0750,uid=%d", uid);
	_mnt("tmpfs", "wrk", "tmpfs", 0, u_opt);
	free(u_opt);

	_mkpnt("wrk/up");
	_mkpnt("wrk/work");

	char *squashed = malloc(256);
	sprintf(squashed, "%s/%s.sqsh", _wrk, argv[1]);
	squash(argv[1], squashed);

	int file_fd, device_fd, loop_ctl;

	file_fd = open(squashed, O_RDWR|O_CLOEXEC);
	if (file_fd < -1) {
		perror("open backing file failed");
		exit(1);
	}
	loop_ctl = open("/dev/loop-control", O_RDWR|O_CLOEXEC);
	if (loop_ctl < -1) {
		perror("failed to open loop-control");
		exit(1);
	}
	if (ioctl(loop_ctl, LOOP_CTL_GET_FREE, 0) < 0) {
		perror("failed to get a free loop");
		exit(1);
	}
	close(loop_ctl);
	device_fd = open("/dev/loop0", O_RDWR|O_CLOEXEC);
	if (device_fd < -1) {
		perror("open loop device failed");
		close(file_fd);
		exit(1);
	}
	if (ioctl(device_fd, LOOP_SET_FD, file_fd) < 0) {
		perror("ioctl LOOP_SET_FD failed");
		close(file_fd);
		close(device_fd);
		exit(1);
	}
	struct loop_info64 loopinfo;
	memset(&loopinfo, 0, sizeof(loopinfo));
	memcpy(loopinfo.lo_file_name, (unsigned char *)squashed, LO_NAME_SIZE);
	loopinfo.lo_file_name[LO_NAME_SIZE - 1] = 0;
	loopinfo.lo_offset = 0;
	loopinfo.lo_flags = LO_FLAGS_AUTOCLEAR;
	if (ioctl(device_fd, LOOP_SET_STATUS64, &loopinfo) < 0) {
		perror("failed to set device status");
		close(file_fd);
		close(device_fd);
		exit(1);
	}
	close(file_fd);
	close(device_fd);

	_mnt("/dev/loop0", "sqsh", "squashfs", MS_RDONLY, NULL);
	free(squashed);

	/*
	if (unshare(CLONE_NEWNS) != 0) {
		fprintf(stderr, "failed to unshare [%s]\n", strerror(errno));
		exit(1);
	}
	*/

	char *tree = malloc(256);
	sprintf(tree, "lowerdir=%s/sqsh,upperdir=%s/wrk/up,workdir=%s/wrk/work", _wrk, _wrk, _wrk);
	_mkpnt("root");
	_mnt("overlay", "root", "overlay", 0, tree);
	free(tree);
	/*
	if (mount("none", argv[2], NULL, MS_PRIVATE, NULL) != 0) {
		fprintf(stderr, "failed to remount root at %s as shared [%s]\n", argv[2], strerror(errno));
		exit(1);
	}
	if (mount("none", argv[2], NULL, MS_SHARED, NULL) != 0) {
		fprintf(stderr, "failed to remount root at %s as shared [%s]\n", argv[2], strerror(errno));
		exit(1);
	}
	*/

	return 0;
}
