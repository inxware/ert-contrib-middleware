## Process this file with automake to produce Makefile.in
# Copyright (C) 2008, 2009  Simon Josefsson.
#
# This file is part of GNU Libidn.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXTRA_DIST = cyclo-$(PACKAGE).html

cyclo-$(PACKAGE).html:
	$(PMCCABE) ${top_srcdir}/lib/*.[ch] \
		| sort -nr \
		| $(AWK) -f ${top_srcdir}/build-aux/pmccabe2html \
			-v lang=html -v name="$(PACKAGE_NAME)" \
			-v vcurl="http://git.savannah.gnu.org/gitweb/?p=$(PACKAGE).git;a=blob;f=%FILENAME%;hb=HEAD" \
			-v url="http://www.gnu.org/software/$(PACKAGE)/" \
                       -v css=${top_srcdir}/build-aux/pmccabe.css \
			> $@-tmp
	mv $@-tmp $@
