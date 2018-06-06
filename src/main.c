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
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <grp.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/mount.h>

#include <sys/socket.h>

#include <linux/loop.h>
#include <sys/capability.h>
#include <linux/capability.h>
#include <sys/prctl.h>

#include <uuid/uuid.h>
#include <sched.h>


/* project headers */
#include <squashfs/squashfs_fs.h>
#include <squashfs/mksquashfs.h>

#include <net/cl.h>

#define USERNS_OFFSET 65534
#define USERNS_COUNT 10
#define _DATA "/var/lib/cladder"
#define pivot_root(new_root,put_old) syscall(SYS_pivot_root,new_root,put_old)


char  *_wrk = NULL;
uid_t  uid;
struct passwd *pwd;

typedef struct {
	int argc;
	char id[37];
	uid_t uid;
	int fd;
	char **argv;
	char *mount_dir;
} config_t;

void _mkpnt(char *pnt, char *id)
{
	char   *tmp = malloc(64);
	struct  stat sb;
	if (_wrk == NULL) {
		_wrk = malloc(64);
		sprintf(_wrk, "%s/%s", _DATA, id);
		if (stat(_DATA, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
			unlink(_DATA);
			if (mkdir(_DATA, S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0) {
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
	config_t *config = arg;
	if (sethostname(config->id, strlen(config->id)) != 0)
		fprintf(stderr, "failed to set hostname to %s, [%s]\n", (char *)arg, strerror(errno));
	char *args[] = { "/sbin/init", 0 };
	char *term   = malloc(32);
	sprintf(term, "TERM=%s", getenv("TERM"));
	char **envp;
	envp = malloc(sizeof(char *));
	envp[0] = malloc(32);
	envp[0] = "PATH=/bin:/sbin:/usr/bin:/usr/sbin";
	envp[1] = malloc(32);
	strcpy(envp[1], term);
	envp[2] = malloc(2);
	envp[2] = 0;
	free(term);

	DIR *d;
	struct dirent *dir;
	d = opendir("/");
	if (d) {
		while ((dir = readdir(d)) != NULL)
			printf("%s\n", dir->d_name);
		closedir(d);
	}
	printf("uid: %d, euid: %d\n", getuid(), geteuid());
	if (mount("proc", "/proc", "proc",  0, NULL) != 0) {
		fprintf(stderr, "failed to mount proc [%s]\n", strerror(errno));
	}
	int has_userns = !unshare(CLONE_NEWUSER);
	if (write(config->fd, &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
		fprintf(stderr, "couldn't write: %m\n");
		return -1;
	}
	int result = 0;
	if (read(config->fd, &result, sizeof(result)) != sizeof(result)) {
		fprintf(stderr, "couldn't read: %m\n");
		return -1;
	}
	if (result) return -1;
	printf("uid: %d, euid: %d\n", getuid(), geteuid());
	if (setgroups(1, & (gid_t) { config->uid }) ||
		setresgid(config->uid, config->uid, config->uid) ||
		setresuid(config->uid, config->uid, config->uid)) {
		fprintf(stderr, "%m\n");
		return -1;
	}
	printf("uid: %d, euid: %d\n", getuid(), geteuid());
	d = opendir("/");
	if (d) {
		while ((dir = readdir(d)) != NULL)
			printf("%s\n", dir->d_name);
		closedir(d);
	}
	return execve(args[0], &args[0], envp);

	int drop_caps[] = {
		CAP_AUDIT_CONTROL,
		CAP_AUDIT_READ,
		CAP_AUDIT_WRITE,
		CAP_BLOCK_SUSPEND,
		CAP_DAC_READ_SEARCH,
		CAP_FSETID,
		CAP_IPC_LOCK,
		CAP_MAC_ADMIN,
		CAP_MAC_OVERRIDE,
		CAP_MKNOD,
		CAP_SETFCAP,
		CAP_SYSLOG,
		CAP_SYS_ADMIN,
		CAP_SYS_BOOT,
		CAP_SYS_MODULE,
		CAP_SYS_NICE,
		CAP_SYS_RAWIO,
		CAP_SYS_RESOURCE,
		CAP_SYS_TIME,
		CAP_WAKE_ALARM
	};
	size_t num_caps = sizeof(drop_caps) / sizeof(*drop_caps);
	for (size_t i = 0; i < num_caps; i++) {
		if (prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0)) {
			fprintf(stderr, "prctl failed: %m\n");
			return 1;
		}
	}
	cap_t caps = NULL;
	if (!(caps = cap_get_proc())
			|| cap_set_flag(caps, CAP_INHERITABLE, num_caps, drop_caps, CAP_CLEAR)
			|| cap_set_proc(caps)) {
		fprintf(stderr, "failed: %m\n");
		if (caps) cap_free(caps);
		return 1;
	}
	cap_free(caps);

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
	config_t config = {0};
	uuid_t uuid;
	uuid_generate(uuid);
	uuid_unparse_lower(uuid, config.id);

	pwd = getpwnam(getenv("SUDO_USER"));
	if (pwd == NULL) {
		fprintf(stderr, "not running as sudo\n");
		exit(1);
	}
	pwd = getpwnam("root");
	uid = pwd->pw_uid;
	_mkpnt("wrk", config.id);
	_mkpnt("sqsh", config.id);
	_mkpnt("root", config.id);

	char *squashed = malloc(256);
	sprintf(squashed, "%s/%s.sqsh", _wrk, argv[1]);
	squash(argv[1], squashed);

	if (unshare(CLONE_NEWNS) != 0) {
		fprintf(stderr, "failed to segregate filesystem, unshare[%s]\n", strerror(errno));
		exit(1);
	}

	char *u_opt = malloc(64);
	sprintf(u_opt, "size=268435456,mode=0777,uid=%d", uid);
	_mnt("tmpfs", "wrk", "tmpfs", 0, u_opt);
	free(u_opt);

	_mkpnt("wrk/up", config.id);
	_mkpnt("wrk/work", config.id);

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

	_mkpnt("root/proc", config.id);
	_mkpnt("root/dev",  config.id);
	_mkpnt("root/sys",  config.id);
	_mkpnt("root/tmp",  config.id);
	_mkpnt("root/old",  config.id);
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
	int sockets[2] = {0};
	char *stack     = malloc(1024 * 1024 * 64);
	char *stack_top = stack + 1024 * 1024 * 64;
	// pid = clone(init, stack_top, CLONE_NEWPID|CLONE_NEWUSER, NULL);
	if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets)) {
		fprintf(stderr, "socketpair failed: %m\n");
		//goto error;
	}
	if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC)) {
		fprintf(stderr, "fcntl failed: %m\n");
		//goto error;
	}
	config.fd = sockets[1];
	if (mount("proc", "/proc", "proc",  0, NULL) != 0) {
		fprintf(stderr, "failed to mount proc [%s]\n", strerror(errno));
	}
	if ((pid = clone(init, stack_top, CLONE_NEWNS|CLONE_NEWPID|SIGCHLD, &config)) == -1) {
		fprintf(stderr, "clone failed [%s]\n", strerror(errno));
	}
	close(sockets[1]);
	sockets[1] = 0;
	int uid_map = 0;
	int has_userns = -1;
	if (read(sockets[0], &has_userns, sizeof(has_userns)) != sizeof(has_userns)) {
		fprintf(stderr, "couldn't read from child!\n");
		return -1;
	}
	if (has_userns) {
		char path[PATH_MAX] = {0};
		for (char **file = (char *[]) { "uid_map", "gid_map", 0 }; *file; file++) {
			if (snprintf(path, sizeof(path), "/proc/%d/%s", pid, *file)
			    > sizeof(path)) {
				fprintf(stderr, "snprintf too big? %m\n");
				return -1;
			}
			if ((uid_map = open(path, O_WRONLY)) == -1) {
				fprintf(stderr, "open failed: %m\n");
				return -1;
			}
			if (dprintf(uid_map, "0 %d %d\n", getuid(), USERNS_COUNT) == -1) {
				fprintf(stderr, "dprintf failed: %m\n");
				close(uid_map);
				return -1;
			}
			close(uid_map);
		}
	}
	if (write(sockets[0], & (int) { 0 }, sizeof(int)) != sizeof(int)) {
		fprintf(stderr, "couldn't write: %m\n");
		return -1;
	}
	close(sockets[0]);
	sockets[0] = 0;
	do {
		w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
		if (w == -1)
			perror("waitpid");
	} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
	printf("child has terminated\n");

	return 0;
}
