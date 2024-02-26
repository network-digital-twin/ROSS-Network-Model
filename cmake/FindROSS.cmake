# - Try to find libROSS
# Once done this will define
#  ROSS_FOUND - System has libROSS
#  ROSS_INCLUDE_DIRS - The libROSS include directories
#  ROSS_LIBRARIES - The libraries needed to use libROSS


FIND_PATH(WITH_ROSS_PREFIX
    NAMES include/ross.h
    HINTS "../build-ross"
)

FIND_LIBRARY(ROSS_LIBRARIES
    NAMES ROSS
    HINTS ${WITH_ROSS_PREFIX}/lib
)

FIND_PATH(ROSS_INCLUDE_DIRS
    NAMES ross.h
    HINTS ${WITH_ROSS_PREFIX}/include
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ROSS DEFAULT_MSG
    ROSS_LIBRARIES
    ROSS_INCLUDE_DIRS
)

# Hide these vars from cmake GUI
MARK_AS_ADVANCED(
	ROSS_LIBRARIES
	ROSS_INCLUDE_DIRS
)
