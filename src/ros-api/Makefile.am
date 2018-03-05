AUTOMAKE_OPTIONS = foreign no-dependencies

AM_CPPFLAGS = -DCONFDIR='$(sysconfdir)'

if COMPILER_IS_GCC
AM_CFLAGS = -Wall -Werror
if BUILD_WITH_DEBUG
AM_CFLAGS += -Wextra -g -O0
endif
endif

include_HEADERS = ../routeros_api.h ../routeros_version.h

bin_PROGRAMS = ros-api

ros_api_SOURCES = ros-api.c
ros_api_LDADD = librouteros.la
