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


AC_PREREQ(2.59)
AC_INIT([cladder], [0.0.1], [bugs@danmolik.com], , [https://github.com/GrayTShirt/cladder/])

AC_CONFIG_SRCDIR([include/cladder.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build])
AC_CONFIG_HEADERS(config.h)

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_INSTALL

AC_HAVE_LIBRARY(pthread, , AC_MSG_ERROR(Missing pthread library))
AC_HAVE_LIBRARY(m, ,       AC_MSG_ERROR(Missing math library))
AC_HAVE_LIBRARY(lz4, ,     AC_MSG_ERROR(Missing lz4 compressor library))
AC_HAVE_LIBRARY(mount, ,   AC_MSG_ERROR(Missing mount library))

AC_C_INLINE

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
