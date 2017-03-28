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

AM_CFLAGS = -Wall -Wextra -g -pedantic $(DEPS_CFLAGS)

core_headers :=
core_headers +=

core_src :=
core_src +=

cladder_SOURCES = $(core_headers) $(core_src)
bin_PROGRAMS   = cladder

dist_man_MANS  =
dist_man_MANS += man/cladder.8

EXTRA_DIST = Changelog README.md COPYING

man/%.1: man/%.1.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.5: man/%.5.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.7: man/%.7.pod
	./man/build $< @PACKAGE_VERSION@ > $@
man/%.8: man/%.8.pod
	./man/build $< @PACKAGE_VERSION@ > $@
