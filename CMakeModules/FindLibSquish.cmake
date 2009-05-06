# Locate libsquish
# This module defines
# LIBSQUISH_LIBRARY
# LIBSQUISH_FOUND, if false, do not try to link to libsquish
# LIBSQUISH_INCLUDE_DIR, where to find the headers
#
# $GDALDIR is an environment variable that would
# correspond to the ./configure --prefix=$LIBSQUISH_DIR
# used in building libsquish.
#
# Created by Eric Wing. I'm not a libsquish user, but OpenSceneGraph uses it
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real libsquish users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems 
# to happen).

# This makes the presumption that you are include libsquish.h like
# #include "libsquish.h"

FIND_PATH(LIBSQUISH_INCLUDE_DIR libsquish.h
  $ENV{LIBSQUISH_DIR}
  NO_DEFAULT_PATH
    PATH_SUFFIXES include
)

FIND_PATH(LIBSQUISH_INCLUDE_DIR quish.h
    PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES include
)

FIND_PATH(LIBSQUISH_INCLUDE_DIR squish.h
  PATHS
  ~/Library/Frameworks/libsquish.framework/Headers
  /Library/Frameworks/libsquish.framework/Headers
  /usr/local/include
  /usr/include
  /sw/include # Fink
  /opt/local/include # DarwinPorts
  /opt/csw/include # Blastwave
  /opt/include
  c:/Program Files/FWTools2.1.0/include
)

FIND_LIBRARY(LIBSQUISH_LIBRARY
  NAMES squish libsquish
  PATHS
  $ENV{LIBSQUISH_DIR}
  NO_DEFAULT_PATH
  PATH_SUFFIXES lib64 lib
)

FIND_LIBRARY(LIBSQUISH_LIBRARY
  NAMES squish libsquish
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
    /usr/freeware
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;LIBSQUISH_ROOT]/lib
  PATH_SUFFIXES lib64 lib
)

SET(LIBSQUISH_FOUND "NO")
IF(LIBSQUISH_LIBRARY AND LIBSQUISH_INCLUDE_DIR)
  SET(LIBSQUISH_FOUND "YES")
ENDIF(LIBSQUISH_LIBRARY AND LIBSQUISH_INCLUDE_DIR)



