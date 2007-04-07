%define version 20070328
Name:           gnash
Version:        cvs%{version}
Release:        1%{?dist}
Summary:        GNU flash movie player

Group:          Applications/Multimedia
Vendor:		Gnash Project
Packager:	Rob Savoye <rob@welcomehome.org>
License:        GPL
URL:            http://www.gnu.org/software/gnash/
Source0:        http://www.gnu.org/software/gnash/releases/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-%{_target_cpu}

#AutoReqProv: no

BuildRequires:  libxml2-devel libpng-devel libjpeg-devel libogg-devel
BuildRequires:  gtk2-devel libX11-devel agg-devel
BuildRequires:  boost-devel curl-devel libXt-devel
# the opengl devel packages are required by gtkglext-devel
# monolithic Xorg
#BuildRequires:  xorg-x11-devel
# modular Xorg 
#BuildRequires:  libGLU-devel libGL-devel
#BuildRequires:  gtkglext-devel
BuildRequires:  mysql-devel mysqlclient14-devel
BuildRequires:  SDL-devel
BuildRequires:  kdelibs-devel
BuildRequires:  docbook2X
BuildRequires:  gstreamer >= 0.10
# BuildRequires:  scrollkeeper

#Requires(post): scrollkeeper
#Requires(postun): scrollkeeper
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
#Requires(post): /sbin/install-info
#Requires(preun): /sbin/install-info

%description
Gnash is a GNU Flash movie player that supports many SWF v7 features.

%package plugin
Summary:   Web-client flash movie player plugin 
Requires:  %{name} = %{version}-%{release}
Requires:  xulrunner
Group:     Applications/Internet

%description plugin
The gnash flash movie player plugin for firefox or mozilla.

%package klash
Summary:   Konqueror flash movie player plugin
Requires:  %{name} = %{version}-%{release}
Group:     Applications/Multimedia

%description klash
The gnash flash movie player plugin for Konqueror.

%package cygnal
Summary:   Streaming media server
Requires:  %{name} = %{version}-%{release}
Group:     Applications/Multimedia

%description cygnal
Cygnal is a streaming media server that's Flash aware.

%prep
%setup -q

%build

[ -n "$QTDIR" ] || . %{_sysconfdir}/profile.d/qt.sh

# handle cross building rpms. This gets messy when building for two
# archtectures with the same CPU type, like x86-Linux -> OLPC. We have
# to do this because an OLPC requires RPMs to install software, but
# doesn't have the resources to do native builds. So this hack lets us
# build RPM packages on one host for the OLPC, or other RPM based
# embedded distributions.
%if %{_target_cpu} != %{_build_arch}
%define cross_compile 1
%else
%define cross_compile 0
%endif
%{?do_cross_compile:%define cross_compile 1}

# FIXME: this ia a bad hack! Although all this does work correctly and
# build an RPM, it's set for an geode-olpc, so the actual hardware
# won't let us install it.
%define cross_compile 1
%define olpc 1

# Build rpms for an ARM based processor, in our case the Nokia 770/800
# tablet. 
%ifarch arm
RPM_TARGET=%{_target}
%endif
# Build rpms for an OLPC, which although it's geode based, our
# toolchain treat this as a stock i386. Our toolchain has geode
# specific optimizations added, which was properly handled by setting
# the vendor field of the config triplet to olpc. Since rpm has no
# concept of a vendor other than Redhat, we force it to use the proper
# config triplet so configure uses the correct cross compiler.
%if %{olpc}
%define _target_platform %{_build_cpu}-%{_build_os}-linux
RPM_TARGET=i386-olpc-linux
%endif

%if %{cross_compile}
# cross building an RPM. This works as long as you have a good cross
# compiler installed. We currently do want to cross compile the
# Mozilla plugin, but not the Konqueror one till we make KDE work
# better than it does now.
  CROSS_OPTS="--build=%{_host} --host=$RPM_TARGET --target=$RPM_TARGET"
  RENDERER="--enable-renderer=agg"		# could be opengl
  %ifarch arm
    SOUND="--disable-media --disable-plugin --disable-klash"
  %else
    SOUND="--enable-media=gst"			# could also be sdl
  %endif
# The OLPC is a weird case, it's basically an i386-linux toolchain
# targeted towards Fedora Core 6. The machine itself is too limited to
# build RPMs on, so we do it this way.
  %if olpc
    CROSS_OPTS="$CROSS_OPTS --disable-klash --disable-menus"
    SOUND="--enable-media=mad --disable-static"
    RENDERER="$RENDERER --with-pixelformat=RGB565"
  %endif
%else
# Native RPM build
  CROSS_OPTS="--enable-ghelp --enable-docbook"
  SOUND="--enable-media=ffmpeg"
  RENDERER=""
%endif

# The default options for the configure aren't suitable for
# cross configuring, so we force them to be what we know is correct.
# export CONFIG_SHELL="sh -x"
# sh -x ./configure \
%if %{cross_compile}
%configure --disable-static --with-plugindir=%{_libdir}/mozilla/plugins \
	$CROSS_OPTS \
	$SOUND \
	$RENDERER \
	--disable-dependency-tracking --disable-rpath \
	--with-plugindir=%{_libdir}/mozilla/plugins
 
make %{?_smp_mflags} CXXFLAGS="-g" dumpconfig all
%else
./configure \
	--with-qtdir=$QTDIR \
	$CROSS_OPTS \
	$SOUND \
	$RENDERER \
	--disable-static \
	--disable-dependency-tracking \
	--disable-rpath \
	--enable-extensions \
        --prefix=%{_prefix} \
	--with-plugindir=%{_libdir}/mozilla/plugins

make CXXFLAGS="-g" dumpconfig all
%endif

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm $RPM_BUILD_ROOT%{_libdir}/*.la
%if !%{cross_compile}
rm -rf $RPM_BUILD_ROOT%{_localstatedir}/scrollkeeper
rm -f $RPM_BUILD_ROOT%{_infodir}/dir
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%post 
/sbin/ldconfig
%if !%{cross_compile}
scrollkeeper-update -q -o %{_datadir}/omf/%{name} || :
/sbin/install-info --entry="* Gnash: (gnash). GNU Flash Player" %{_infodir}/%{name}.info %{_infodir}/dir || :
%endif

%preun
if [ $1 = 0 ]; then
    /sbin/install-info --delete %{_infodir}/%{name}.info %{_infodir}/dir || :
fi

%postun
/sbin/ldconfig
%if !%{cross_compile}
scrollkeeper-update -q || :
%endif

%files
%defattr(-,root,root,-)
%dump
%doc README AUTHORS COPYING NEWS 
%{_bindir}/gnash
%{_bindir}/gparser
%{_bindir}/gprocessor
%{_libdir}/libgnash*.so
%{_libdir}/mozilla/plugins/*.so
%{_libdir}/libltdl*
%{_prefix}/include/ltdl.h
%{_prefix}/share/gnash/GnashG.png
%{_prefix}/share/gnash/gnash_128_96.ico
%if !%{cross_compile}
%{_prefix}/man/man1/gnash.1*
%doc doc/C/gnash.html 
%doc %{_prefix}/share/gnash/doc/gnash/C/images
%{_datadir}/omf/gnash/gnash-C.omf
%{_infodir}/gnash.info*.gz
%{_prefix}/share/gnash/doc/gnash/C/*.xml
%endif

%files plugin
%defattr(-,root,root,-)
%{_libdir}/mozilla/plugins/libgnashplugin.so

%files klash
%defattr(-,root,root,-)
%{_bindir}/gnash
%if !%{cross_compile}
%{_libdir}/kde3/libklashpart.*
%{_datadir}/apps/klash/
%{_datadir}/services/klash_part.desktop
%endif

%files cygnal
%defattr(-,root,root,-)
%{_bindir}/cygnal

%changelog
* Sat Mar  6 2007 Rob Savoye <rob@welcomehome.org> - %{version}-%{release}
- merge in patch from John @ Redhat.

* Tue Mar 06 2007 John (J5) Palmieri <johnp@redhat.com> 0.7.2.cvs20070306-1
- update to new snapshot

* Thu Feb 28 2007 John (J5) Palmieri <johnp@redhat.com> 0.7.2.cvs20070226-3
- require xulrunner instead of webclient

* Wed Feb 28 2007 John (J5) Palmieri <johnp@redhat.com> 0.7.2.cvs20070226-2
- don't delete requires .so files

* Mon Feb 26 2007 John (J5) Palmieri <johnp@redhat.com> 0.7.2.cvs20070226-1
- cvs snapshot built for olpc

* Sat Nov  7 2006 Rob Savoye <rob@welcomehome.org> - 0.7.2-2
- update for 0.7.2 release.

* Sat Nov  6 2006 Patrice Dumas <pertusus@free.fr> 0.7.2-1
- update for 0.7.2 release.

* Thu Oct 05 2006 Christian Iseli <Christian.Iseli@licr.org> 0.7.1-9
 - rebuilt for unwind info generation, broken in gcc-4.1.1-21

* Sun Sep 24 2006 Patrice Dumas <pertusus@free.fr> 0.7.1-8
- plugin requires %%{_libdir}/mozilla/plugins. Fix (incompletly and 
  temporarily, but there is no better solution yet) #207613

* Sun Aug 27 2006 Patrice Dumas <pertusus@free.fr> - 0.7.1-7
- add defattr for klash
- add warnings in the description about stability

* Mon Aug 21 2006 Patrice Dumas <pertusus@free.fr> - 0.7.1-6
- remove superfluous buildrequires autoconf
- rename last patch to gnash-plugin-tempfile-dir.patch
- add README.fedora to plugin to explain tmpdirs

* Wed Aug 16 2006 Jens Petersen <petersen@redhat.com> - 0.7.1-5
- source qt.sh and configure --with-qtdir (Dominik Mierzejewski)
- add plugin-tempfile-dir.patch for plugin to use a safe tempdir

* Fri Jul 28 2006 Jens Petersen <petersen@redhat.com> - 0.7.1-4
- buildrequire autotools (Michael Knox)

* Fri Jun  2 2006 Patrice Dumas <pertusus@free.fr> - 0.7.1-3
- add gnash-continue_on_info_install_error.patch to avoid
- buildrequire libXmu-devel

* Wed May 17 2006 Jens Petersen <petersen@redhat.com> - 0.7.1-2
- configure with --disable-rpath
- buildrequire docbook2X
- remove devel files

* Sun May  7 2006 Jens Petersen <petersen@redhat.com> - 0.7.1-1
- update to 0.7.1 alpha release

* Sat Apr  22 2006 Rob Savoye <rob@welcomehome.org> - 0.7-1
- install the info file. Various tweaks for my system based on
Patrice's latest patch,

* Fri Feb  3 2006 Patrice Dumas <dumas@centre-cired.fr> - 0.7-0
- initial packaging

