#
#	gr-mcs_judgement - A GNU Radio modulation scheme jugement tool
#	Copyright (C) 2017 Bowring Chan <bowringchan@gmail.com>. All Rights Reserved.
#	Copyright (C) 2015 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
#	Copyright (C) 2012  Nicholas Tomlinson
#	
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#	
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#	
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>. 
#

VERSION = 20170302
CXXFLAGS ?= -O3 -march=native -fomit-frame-pointer
CXXFLAGS +=-DVERSION="\"gr-mcs_judgement $(VERSION)\"" -Wall
LDLIBS = -lgnuradio-filter -lgnuradio-blocks -lgnuradio-pmt -lfftw3 -lgnuradio-runtime -lgnuradio-osmosdr -lboost_system

PREFIX ?= /usr
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
MANDIR ?= $(PREFIX)/share/man

all: gr-mcs_judgement

gr-mcs_judgement: *.cpp *.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) *.cpp -o gr-mcs_judgement $(LDLIBS) $(LDFLAGS)
clean:
	rm -f gr-mcs_judgement

install: gr-mcs_judgement
	@install -v -d "$(DESTDIR)$(BINDIR)" && install -s -m 0755 -v gr-mcs_judgement "$(DESTDIR)$(BINDIR)/gr-mcs_judgement"
