###############################################
#          Compression build options          #
###############################################
#
#
LZ4_SUPPORT = 1


COMP_DEFAULT = lz4
XATTR_SUPPORT = 1
XATTR_DEFAULT = 1


###############################################
#        End of BUILD options section         #
###############################################

INCLUDEDIR = -I.
INSTALL_DIR = /usr/bin

MKSQUASHFS_OBJS = mksquashfs.o read_fs.o action.o swap.o pseudo.o compressor.o \
	sort.o read_file.o info.o restore.o process_fragments.o \
	caches-queues-lists.o lz4_wrapper.o xattr.o read_xattrs.o

CFLAGS ?= -O2
CFLAGS += $(EXTRA_CFLAGS) $(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 \
	-D_LARGEFILE_SOURCE -D_GNU_SOURCE -DCOMP_DEFAULT=\"$(COMP_DEFAULT)\" \
	-Wall -DLZ4_SUPPORT -DXATTR_SUPPORT -DXATTR_DEFAULT

LIBS = -lpthread -lm -llz4
COMPRESSORS = lz4


.PHONY: all
all: mksquashfs

mksquashfs: $(MKSQUASHFS_OBJS)
	$(CC) $(LDFLAGS) $(EXTRA_LDFLAGS) $(MKSQUASHFS_OBJS) $(LIBS) -o $@

mksquashfs.o: Makefile mksquashfs.c squashfs_fs.h squashfs_swap.h mksquashfs.h \
	sort.h pseudo.h compressor.h xattr.h action.h error.h \
	info.h caches-queues-lists.h read_fs.h restore.h process_fragments.h 

read_fs.o: read_fs.c squashfs_fs.h squashfs_swap.h compressor.h xattr.h \
	error.h mksquashfs.h

sort.o: sort.c squashfs_fs.h mksquashfs.h sort.h error.h

swap.o: swap.c

pseudo.o: pseudo.c pseudo.h error.h

compressor.o: Makefile compressor.c compressor.h squashfs_fs.h

xattr.o: xattr.c squashfs_fs.h squashfs_swap.h mksquashfs.h xattr.h error.h

read_xattrs.o: read_xattrs.c squashfs_fs.h squashfs_swap.h xattr.h error.h

action.o: action.c squashfs_fs.h mksquashfs.h action.h error.h

read_file.o: read_file.c error.h

info.o: info.c squashfs_fs.h mksquashfs.h error.h caches-queues-lists.h

restore.o: restore.c caches-queues-lists.h squashfs_fs.h mksquashfs.h error.h \
	info.h

process_fragments.o: process_fragments.c process_fragments.h

caches-queues-lists.o: caches-queues-lists.c error.h caches-queues-lists.h

lz4_wrapper.o: lz4_wrapper.c squashfs_fs.h lz4_wrapper.h compressor.h

.PHONY: clean
clean:
	-rm -f *.o mksquashfs

.PHONY: install
install: mksquashfs
	mkdir -p $(INSTALL_DIR)
	cp mksquashfs $(INSTALL_DIR)
