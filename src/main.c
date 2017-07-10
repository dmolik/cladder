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
#include <sys/wait.h>

#include <sys/mount.h>
#include <linux/loop.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <uuid/uuid.h>
#include <sched.h>

#include "src/squashfs/squashfs_fs.h"
#include "src/squashfs/mksquashfs.h"

#include <sys/syscall.h>
#define pivot_root(new_root,put_old) syscall(SYS_pivot_root,new_root,put_old)

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

static int init(void *arg)
{
	if (arg != NULL)
		fprintf(stderr, "arg is not null\n");
	if (sethostname(arg, strlen(arg)) != 0)
		fprintf(stderr, "failed to set hostname to %s, [%s]\n", (char *)arg, strerror(errno));
	char *args[] = { "/sbin/init", 0 };
	char *term   = malloc(32);
	sprintf(term, "TERM=%s", getenv("TERM"));
	printf("term: %s\n", term);
	char **envp;
	envp = malloc(sizeof(char *));
	envp[0] = malloc(32);
	envp[0] = "PATH=/bin";
	envp[1] = malloc(32);
	strcpy(envp[1], term);
	envp[2] = malloc(2);
	envp[2] = 0;

	free(term);
	if (mount("proc", "/proc", "proc",  0, NULL) != 0) {
		fprintf(stderr, "failed to mount proc [%s]\n", strerror(errno));
	}
	unshare(CLONE_NEWUSER);
	return execve(args[0], &args[0], envp);
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
			printf("Usage: %s [-h?V] src\n",
			         PACKAGE_NAME);

			printf("Required:\n");
			printf("  src     the source directory to turn into an archive\n");

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

	if (argv[1] == NULL) {
		printf("please provide a src, see -? for more information\n");
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
	_mkpnt("root");

	char *squashed = malloc(256);
	sprintf(squashed, "%s/%s.sqsh", _wrk, argv[1]);
	squash(argv[1], squashed);

	if (unshare(CLONE_NEWNS) != 0) {
		fprintf(stderr, "failed to segregate filesystem, unshare[%s]\n", strerror(errno));
		exit(1);
	}

	char *u_opt = malloc(64);
	sprintf(u_opt, "size=134217728,mode=0750,uid=%d", uid);
	_mnt("tmpfs", "wrk", "tmpfs", 0, u_opt);
	free(u_opt);

	_mkpnt("wrk/up");
	_mkpnt("wrk/work");

	int   file_fd, device_fd, loop_ctl;
	long  dev_num;
	char *loop_name;

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
	if ((dev_num = ioctl(loop_ctl, LOOP_CTL_GET_FREE, 0)) < 0) {
		perror("failed to get a free loop");
		exit(1);
	}
	close(loop_ctl);

	loop_name = malloc(64);
	sprintf(loop_name, "/dev/loop%ld", dev_num);
	device_fd = open(loop_name, O_RDWR|O_CLOEXEC);
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

	_mnt(loop_name, "sqsh", "squashfs", MS_RDONLY, NULL);
	free(squashed);

	char *tree = malloc(256);
	sprintf(tree, "lowerdir=%s/sqsh,upperdir=%s/wrk/up,workdir=%s/wrk/work", _wrk, _wrk, _wrk);
	_mnt("overlay", "root", "overlay", 0, tree);
	free(tree);

	_mkpnt("root/proc");
	_mkpnt("root/dev");
	_mkpnt("root/sys");
	_mkpnt("root/tmp");
	_mkpnt("root/old");
	//_mnt("proc", "root/proc", "proc",  0,       NULL);
	_mnt("sys",  "root/sys",  "sysfs", 0,       NULL);
	_mnt("/dev", "root/dev",  "none",  MS_BIND, NULL);

	// if (unshare(CLONE_FS|CLONE_NEWIPC|CLONE_NEWPID|CLONE_NEWUSER|CLONE_NEWUTS|CLONE_SYSVSEM) != 0) {
	if (unshare(CLONE_FS|CLONE_FILES|CLONE_NEWUTS|CLONE_SYSVSEM|CLONE_NEWIPC) != 0) {
		fprintf(stderr, "failed to unshare manythings [%s]\n", strerror(errno));
	}
	char *_root     = malloc(256);
	char *_old_root = malloc(256);
	sprintf(_root,     "%s/root",     _wrk);
	sprintf(_old_root, "%s/root/old", _wrk);

	pivot_root(_root, _old_root);
	if (chdir("/") != 0) {
		fprintf(stderr, "failed to set working to / [%s]\n", strerror(errno));
	}
	if (umount2("/old", MNT_FORCE|MNT_DETACH) != 0) {
		fprintf(stderr, "failed to unmount old root [%s]\n", strerror(errno));
	}
	unshare(CLONE_NEWNS);
	unlink("/old");

	pid_t pid, w;
	int wstatus;
	char *stack     = malloc(1024 * 1024 * 32);
	char *stack_top = stack + 1024 * 1024 * 32; // counting on order of operations
	// pid = clone(init, stack_top, CLONE_NEWPID|CLONE_NEWUSER, NULL);
	if ((pid = clone(init, stack_top, CLONE_NEWPID|SIGCHLD, id)) == -1) {
		fprintf(stderr, "clone failed [%s]\n", strerror(errno));
	}
	do {
		w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
		if (w == -1)
			perror("waitpid");
	} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
	printf("child has terminated\n");

	return 0;
}
