#
# Copyright 2017 Dan Molik <dan@danmolik.com>
#
# This file is part of Cladder
#
# Cladder is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Cladderis distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Cladder.  If not, see <http://www.gnu.org/licenses/>.
#


AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS = -I build

AM_CFLAGS = -Wall -Wextra -g -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
	-D_LARGEFILE_SOURCE \
	-DLZ4_SUPPORT -DXATTR_SUPPORT -DXATTR_DEFAULT $(DEPS_CFLAGS)
AM_CPPFLAGS  = -I$(top_srcdir)/src/deps/lz4 -I$(top_srcdir)/src/deps/squashfs -I$(top_srcdir)/src/
AM_CPPFLAGS += -I$(top_srcdir)/include -I$(top_srcdir)/src/deps

core_headers :=
core_headers += include/cladder.h

lz4_headers = src/deps/lz4/lz4.h src/deps/lz4/lz4opt.h src/deps/lz4/lz4hc.h
lz4_src     = src/deps/lz4/lz4.c
lz4hc_src   = $(lz4_src) src/deps/lz4/lz4hc.c

squashfs_src  = src/deps/squashfs/squashfs_swap.h src/deps/squashfs/squashfs_fs.h src/deps/squashfs/swap.c
squashfs_src += src/deps/squashfs/caches-queues-lists.h src/deps/squashfs/caches-queues-lists.c
squashfs_src += src/deps/squashfs/error.h src/deps/squashfs/lz4_wrapper.h src/deps/squashfs/mksquashfs.c
squashfs_src += src/deps/squashfs/process_fragments.c src/deps/squashfs/pseudo.c  src/deps/squashfs/read_file.c  src/deps/squashfs/read_fs.h
squashfs_src += src/deps/squashfs/sort.c  src/deps/squashfs/squashfs_compat.h src/deps/squashfs/compressor.c

squashfs_src += src/deps/squashfs/mksquashfs.h src/deps/squashfs/action.h src/deps/squashfs/action.c  src/deps/squashfs/compressor.h
squashfs_src += src/deps/squashfs/lz4_wrapper.c src/deps/squashfs/process_fragments.h
squashfs_src += src/deps/squashfs/pseudo.h  src/deps/squashfs/read_fs.c src/deps/squashfs/read_xattrs.c
squashfs_src += src/deps/squashfs/sort.h  src/deps/squashfs/xattr.h src/deps/squashfs/xattr.c

core_src :=
core_src += src/net/namespace.h src/net/namespace.h
core_src += src/net/fw.h        src/net/fw.h
core_src += src/net/nl.h        src/net/nl.c
core_src += src/net/cl.h        src/net/cl.c
core_src += src/main.c


include_HEADERS = include/cladder.h $(lz4_headers)
cladder_SOURCES = $(core_headers) $(lz4hc_src) $(squashfs_src) $(core_src)
bin_PROGRAMS    = cladder

dist_man_MANS  =
dist_man_MANS += man/cladder.8

check_SCRIPTS = t/01-full.sh
TESTS = $(check_SCRIPTS)
dist_check_SCRIPTS=$(check_SCRIPTS) t/run t/lib.sh

EXTRA_DIST = Changelog README.md COPYING

man/%.1: man/%.1.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.5: man/%.5.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.7: man/%.7.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.8: man/%.8.pod
	./man/build $< @PACKAGE_VERSION@ > $@
