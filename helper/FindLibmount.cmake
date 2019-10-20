find_package(PkgConfig)
pkg_check_modules(PC_MOUNT QUIET mount)

find_path(MOUNT_INCLUDE_DIRS
          NAMES libmount.h
          HINTS ${PC_MOUNT_INCLUDEDIR}
          PATH_SUFFIXES libmount)

find_library(MOUNT_LIBRARIES NAMES mount HINTS ${PC_MOUNT_LIBDIR})
