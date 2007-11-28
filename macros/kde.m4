dnl  
dnl    Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
dnl  
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 3 of the License, or
dnl  (at your option) any later version.
dnl  
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


AC_DEFUN([GNASH_PATH_KDE],
[
  dnl Lool for the header
  AC_ARG_WITH(kde_incl, AC_HELP_STRING([--with-kde-incl], [directory where kde headers are]), with_kde_incl=${withval})
  AC_CACHE_VAL(ac_cv_path_kde_incl,[
    if test x"${with_kde_incl}" != x ; then
      if test -f ${with_kde_incl}/kapp.h ; then
        ac_cv_path_kde_incl="-I`(cd ${with_kde_incl}; pwd)`"
      else
        AC_MSG_ERROR([${with_kde_incl} directory doesn't contain any headers])
      fi
    fi
  ])

  kde_prefix="/usr"
  has_kde=no
  if test x"${ac_cv_path_kde_incl}" = x; then
    AC_PATH_PROG(KDE_CONFIG, kde-config, ,[${pathlist}])
    if test "x$KDE_CONFIG" != "x" ; then
      if test "x$KDE_CFLAGS" = "x" ; then
        kde_prefix=`$KDE_CONFIG --prefix`

        if test -f ${kde_prefix}/include/kapp.h ; then
          ac_cv_path_kde_incl="-I${kde_prefix}/include -I${kde_prefix}/include/kio"
        else
	  if test -f ${kde_prefix}/include/kde/kapp.h; then
            ac_cv_path_kde_incl="-I${kde_prefix}/include/kde -I${kde_prefix}/include/kde/kio"
	  fi
        fi

      fi
    else
      AC_MSG_RESULT(no)
    fi
  fi
  AC_MSG_CHECKING([for kde header])

  dnl incllist is inherited from configure.ac.
  if test x"${ac_cv_path_kde_incl}" = x ; then
    for i in $incllist; do
      if test -f $i/kde/kapp.h; then
        ac_cv_path_kde_incl="-I$i/kde"
        kde_prefix=`dirname $i`
        break
      fi
    done
  fi

  if test x"${ac_cv_path_kde_incl}" = x; then
    AC_MSG_RESULT(no)
    if test x${cross_compiling} = xno; then
      AC_CHECK_HEADERS(kde/kapp.h, [ac_cv_path_kde_incl=""])
    fi
  else
    AC_MSG_RESULT(${ac_cv_path_kde_incl})
  fi

  if test x"${ac_cv_path_kde_incl}" != x ; then
    KDE_CFLAGS="${ac_cv_path_kde_incl}"
  else
    KDE_CFLAGS=""
  fi

dnl   # KDE_LIBS =  -lkdeui -lkdecore -lkdeprint -L/usr/lib/qt-3.3/lib -lqt-mt
  dnl Look for the libraries
  AC_ARG_WITH(kde_lib, AC_HELP_STRING([--with-kde-lib], [directory where kde libraries are]), with_kde_lib=${withval})
  AC_CACHE_VAL(ac_cv_path_kde_lib, [
    if test x"${with_kde_lib}" != x ; then
      if test `ls -C1 ${with_kde_lib}/libkdeui.* | wc -l` -gt 0; then
	      ac_cv_path_kde_lib="-L`(cd ${with_kde_lib}; pwd)`"
      else
	      AC_MSG_ERROR([${with_kde_lib} directory doesn't contain kde libraries.])
      fi
    fi
  ])

  if test x"${ac_cv_path_kde_lib}" = x; then
    AC_MSG_CHECKING([for kdeui library])
    topdir=""
    newlist="${kde_prefix}/lib ${libslist}"
    for i in $newlist ; do
      if test -f $i/libkdeui.a -o -f $i/libkdeui.${shlibext} ; then
        topdir=$i
        AC_MSG_RESULT(${topdir}/libkdeui)
	      if test ! x"$i" = x"/usr/lib" -a ! x"$i" = x"/usr/lib64"; then
	        ac_cv_path_kde_lib="-L$i -lkdeui"
       	  break
        else
	        ac_cv_path_kde_lib="-lkdeui"
	        break
        fi
      fi
    done
  fi

  dnl Look for the kdecore library, which is required
  if test x"${ac_cv_path_kde_lib}" != x; then
    AC_MSG_CHECKING([for kdecore library])
    if test `ls -C1 ${topdir}/libkdecore.* | wc -l` -gt 0; then
      ac_cv_path_kde_lib="${ac_cv_path_kde_lib} -lkdecore"
      AC_MSG_RESULT(${topdir}/libkdecore)
    else
      AC_MSG_RESULT(no)
    fi

    dnl Look for the kdeprint library, which is required
    AC_MSG_CHECKING([for kdeprint library])
    if test x"${libkdeprint}" = x; then
      if test -f ${topdir}/libkdeprint.a -o -f ${topdir}/libkdeprint.${shlibext}; then
        ac_cv_path_kde_lib="${ac_cv_path_kde_lib} -lkdeprint"
        AC_MSG_RESULT(${topdir}/libkdeprint)
      else
        AC_MSG_RESULT(no)
      fi
    else
      AC_MSG_RESULT(${libkdeprint})
      ac_cv_path_kde_lib="${ac_cv_path_kde_lib} ${libkdeprint}"
    fi	
  fi                            dnl end of all optional library tests

  if test x"${ac_cv_path_kde_lib}" != x; then
    KDE_LIBS="${ac_cv_path_kde_lib}"
    AC_DEFINE(HAVE_KDE,1,[Have KDE installed])
    has_kde=yes
  else
    KDE_LIBS=""
    has_kde=no
  fi

  KLASH_PLUGIN=
  

  AC_SUBST(KLASH_PLUGIN)
  AC_SUBST(KDE_CFLAGS)  
  AC_SUBST(KDE_LIBS)
])

# Local Variables:
# c-basic-offset: 2
# tab-width: 2
# indent-tabs-mode: nil
# End:
