AC_PREREQ([2.63])
AC_INIT([FinTP_base],[0.0.1])
AC_CONFIG_SRCDIR([src/DllMain.cpp])
AC_CONFIG_MACRO_DIR([../fintp_utils/m4])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AM_INIT_AUTOMAKE

AC_CHECK_HEADERS([malloc.h stdlib.h string.h sys/param.h sys/timeb.h unistd.h], [], [Cannot build])

AC_CHECK_FUNCS([ftime gethostbyaddr gethostbyname gethostname memset strerror strtol strtoul uname], [], [Cannot build])

ENVIRONMENT

AX_PTHREAD(, [AC_MSG_ERROR([Cannot build without pthread]) ])

AX_BOOST_BASE([1.48.0], , [AC_MSG_ERROR([Cannot build without Boost])])

AX_BOOST_REGEX

AX_LIB_XERCES([2.8.0])

if test $HAVE_XERCES != yes; then
	AC_MSG_ERROR([Cannot build without Xerces])
fi

AX_LIB_XALAN([1.10.0])

if test $HAVE_XALAN != yes; then
	AC_MSG_ERROR([Cannot build without Xalan])
fi

AX_CHECK_OPENSSL([],[AC_MSG_ERROR([Cannot build without OpenSSL])])

LIBCURL_CHECK_CONFIG([yes], [7.13.0], [], [AC_MSG_ERROR(Cannot build without Curl)])

if test $libcurl_protocol_HTTPS != yes; then
	AC_MSG_WARN([libcurl doesn't support HTTPS protocol])
fi

if test $libcurl_feature_SSL != yes; then
	AC_MSG_WARN([libcurl doesn't support SSL feature])
fi

ZIPARCHIVE

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

