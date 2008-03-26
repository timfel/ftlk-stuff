#
# "$Id: flphoto.spec 436 2006-12-06 01:14:16Z mike $"
#
#   RPM "spec" file for flPhoto.
#
#   Copyright 2002-2006 by Michael Sweet
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#

Summary: flPhoto
Name: flphoto
Version: 1.3.1
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source: http://prdownloads.sourceforge.net/fltk/flphoto-%{version}-source.tar.gz
Url: http://www.easysw.com/~mike/flPhoto
Packager: Anonymous <anonymous@foo.com>
Vendor: Michael Sweet

# Use buildroot so as not to disturb the version already installed
BuildRoot: /var/tmp/%{name}-root

%description
flPhoto is a basic image management and display program based on
the FLTK <http://www.fltk.org> toolkit and is provided under the
terms of the GNU General Public License.  It can read, display,
print, and export many image file formats and supports EXIF
information provided by digital cameras.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" LDFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr

# If we got this far, all prerequisite libraries must be here.
make

%install
# Make sure the RPM_BUILD_ROOT directory exists.
rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/etc/X11/applnk/Graphics/*
/usr/bin/*
%dir /usr/share/doc/flphoto
/usr/share/doc/flphoto/*
/usr/share/locale/*
/usr/share/man/*
/usr/share/mimelnk/application/*
/usr/share/pixmaps/*

#
# End of "$Id: flphoto.spec 436 2006-12-06 01:14:16Z mike $".
#
