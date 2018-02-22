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
AM_CPPFLAGS = -I$(top_srcdir)/src/lz4 -I$(top_srcdir)/src

core_headers :=
core_headers += src/cladder.h

lz4_headers = src/lz4/lz4.h src/lz4/lz4opt.h src/lz4/lz4hc.h
lz4_src     = src/lz4/lz4.c
lz4hc_src   = $(lz4_src) src/lz4/lz4hc.c

squashfs_src  = src/squashfs/squashfs_swap.h src/squashfs/squashfs_fs.h src/squashfs/swap.c
squashfs_src += src/squashfs/caches-queues-lists.h src/squashfs/caches-queues-lists.c
squashfs_src += src/squashfs/error.h src/squashfs/lz4_wrapper.h src/squashfs/mksquashfs.c
squashfs_src += src/squashfs/process_fragments.c src/squashfs/pseudo.c  src/squashfs/read_file.c  src/squashfs/read_fs.h
squashfs_src += src/squashfs/sort.c  src/squashfs/squashfs_compat.h src/squashfs/compressor.c

squashfs_src += src/squashfs/mksquashfs.h src/squashfs/action.h src/squashfs/action.c  src/squashfs/compressor.h
squashfs_src += src/squashfs/lz4_wrapper.c src/squashfs/process_fragments.h
squashfs_src += src/squashfs/pseudo.h  src/squashfs/read_fs.c src/squashfs/read_xattrs.c
squashfs_src += src/squashfs/sort.h  src/squashfs/xattr.h src/squashfs/xattr.c

core_src :=
core_src += src/net/namespace.h src/net/namespace.c
core_src += src/net/fw.h        src/net/fw.c
core_src += src/net/nl.h        src/net/nl.c
core_src += src/net/cl.h        src/net/cl.c
core_src += src/main.c


include_HEADERS = src/cladder.h $(lz4_headers)
cladder_SOURCES = $(core_headers) $(lz4hc_src) $(squashfs_src) $(core_src)
bin_PROGRAMS    = cladder

dist_man_MANS  =
dist_man_MANS += man/cladder.8

check_SCRIPTS = t/01-full.sh
TESTS = $(check_SCRIPTS)
dist_check_SCRIPTS=$(check_SCRIPTS) t/run t/lib.sh

EXTRA_DIST = Changelog README.md COPYING mkcontainer

man/%.1: man/%.1.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.5: man/%.5.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.7: man/%.7.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.8: man/%.8.pod
	./man/build $< @PACKAGE_VERSION@ > $@
