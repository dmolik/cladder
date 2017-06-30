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

#include "src/squashfs/squashfs_fs.h"
#include "src/squashfs/mksquashfs.h"

int main (int argc, char *argv[])
{

	struct passwd *pwd;
	uid_t uid;

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

	pwd = getpwnam(getenv("SUDO_USER"));
	if (pwd == NULL) {
		exit(1);
	}
	uid = pwd->pw_uid;
	mkdir(".cladder", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	if (chown(".cladder", uid, -1) != 0) {
		fprintf(stderr, "failed to chown %s to %s, [%s]\n", ".cladder", getenv("SUDO_USER"), strerror(errno));
		exit(1);
	}
	mkdir(".cladder.sqsh", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	if (chown(".cladder.sqsh", uid, -1) != 0) {
		fprintf(stderr, "failed to chown %s to %s, [%s]\n", ".cladder.sqsh", getenv("SUDO_USER"), strerror(errno));
		exit(1);
	}

	char *u_opt = malloc(64);
	sprintf(u_opt, "size=134217728,mode=0750,uid=%d", uid);
	if (mount("tmpfs", ".cladder", "tmpfs", 0, u_opt) != 0) {
		fprintf(stderr, "failed to mount ram in .cladder [%s]\n", strerror(errno));
		exit(1);
	}
	free(u_opt);

	mkdir(".cladder/up", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	mkdir(".cladder/work", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	if (chown(".cladder/up",   uid, -1) != 0) {
		fprintf(stderr, "failed to chown %s to %s, [%s]\n", ".cladder/up", getenv("SUDO_USER"), strerror(errno));
		exit(1);
	}
	if (chown(".cladder/work", uid, -1) != 0) {
		fprintf(stderr, "failed to chown %s to %s, [%s]\n", ".cladder/work", getenv("SUDO_USER"), strerror(errno));
		exit(1);
	}

	char *squashed = malloc(64);
	sprintf(squashed, "%s.sqsh", argv[1]);
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
	if (ioctl(device_fd, LOOP_CLR_FD, 0) < 0) {
		perror("ioctl LOOP_CLR_FD failed");
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

	if (mount("/dev/loop0", ".cladder.sqsh", "squashfs", MS_RDONLY, NULL) != 0) {
		fprintf(stderr, "failed to mount %s to .cladder.sqsh [%s]\n", squashed, strerror(errno));
		exit(1);
	}
	free(squashed);

	mkdir(argv[2], S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	if (chown(argv[2], uid, -1) != 0) {
		fprintf(stderr, "failed to chown %s to %s, [%s]\n", argv[2], getenv("SUDO_USER"), strerror(errno));
		exit(1);
	}

	if (mount("overlay", argv[2], "overlay", 0,
		"lowerdir=.cladder.sqsh,upperdir=.cladder/up,workdir=.cladder/work") != 0) {
		fprintf(stderr, "failed to overlay %s onto %s [%s]\n", argv[2], ".cladder.sqsh", strerror(errno));
		exit(1);
	}

	return 0;
}
