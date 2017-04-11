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

#include <libmount/libmount.h>
#include "src/squashfs/squashfs_fs.h"
#include "src/squashfs/mksquashfs.h"

int main (int argc, char *argv[])
{

	struct libmnt_context *ctx;
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
			printf("  %s      test  writeable\n");

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

	mnt_init_debug(0);
	ctx = mnt_new_context();
	if (!ctx) {
		fprintf(stderr, "failed to allocate mount context\n");
		exit(1);
	}
	/*if (mnt_context_append_options(ctx, "")) {
		fprintf(stderr, "failed to add mount option\n");
		exit(1);
	}
	*/
	pwd = getpwnam(getenv("SUDO_USER"));
	if (pwd == NULL) {
		exit(1);
	}
	uid = pwd->pw_uid;
	mkdir(".cladder", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	chown(".cladder", uid, -1);
	mkdir(".cladder.sqsh", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	chown(".cladder.sqsh", uid, -1);

	mnt_context_set_fstype(ctx, "tmpfs");
	mnt_context_set_source(ctx, "tmpfs");
	mnt_context_append_options(ctx, "size=128m");
	mnt_context_append_options(ctx, "mode=0750");
	char *u_opt = malloc(64);
	sprintf(u_opt, "uid=%s", getenv("SUDO_USER"));
	mnt_context_append_options(ctx, u_opt);
	free(u_opt);
	mnt_context_set_target(ctx, ".cladder");

	int rc = mnt_context_mount(ctx);
	mnt_free_context(ctx);

	mkdir(".cladder/up", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	mkdir(".cladder/work", S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	chown(".cladder/up",   uid, -1);
	chown(".cladder/work", uid, -1);

	char *squashed = malloc(64);
	sprintf(squashed, "%s.sqsh", argv[1]);
	squash(argv[1], squashed);
	ctx = mnt_new_context();
	if (!ctx) {
		fprintf(stderr, "failed to allocate mount context\n");
		exit(1);
	}
	mnt_context_set_fstype(ctx, "squashfs");
	mnt_context_set_source(ctx, squashed);
	mnt_context_set_target(ctx, ".cladder.sqsh");


	rc = mnt_context_mount(ctx);
	mnt_free_context(ctx);

	ctx = mnt_new_context();
	if (!ctx) {
		fprintf(stderr, "failed to allocate mount context\n");
		exit(1);
	}
	mnt_context_set_fstype(ctx, "overlay");
	mnt_context_set_source(ctx, "overlay");

	mkdir(argv[2], S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP);
	chown(argv[2], uid, -1);

	mnt_context_set_target(ctx, argv[2]);
	//char *lower = malloc(64);
	//sprintf(lower, "lowerdir=%s", argv[1]);
	mnt_context_append_options(ctx, "lowerdir=.cladder.sqsh");
	//free(lower);
	mnt_context_append_options(ctx, "upperdir=.cladder/up");
	mnt_context_append_options(ctx, "workdir=.cladder/work");

	rc = mnt_context_mount(ctx);
	mnt_free_context(ctx);

	return rc;
}
