# FindSentry.cmake
# Created: 2024-12-26 by ARYPROGRAMMER
#
# This module finds the Sentry Native SDK and its dependencies.
# It supports Windows, Linux, and macOS platforms.
#
# The following variables are set:
#   SENTRY_FOUND        - True if Sentry was found
#   SENTRY_INCLUDE_DIR  - The Sentry include directory
#   SENTRY_LIBRARY      - The Sentry library to link against
#   SENTRY_VERSION      - The version of Sentry found

# Platform-specific search paths
if(UNIX AND NOT APPLE)
    # Linux paths
    set(_SENTRY_SEARCH_PATHS
        /usr/local
        /usr
        /opt/local
        /opt
    )
    
    set(_SENTRY_INCLUDE_SUFFIXES
        include
        include/sentry
    )
    
    set(_SENTRY_LIB_SUFFIXES
        lib
        lib64
        lib/x86_64-linux-gnu
    )
    
    set(_SENTRY_NAMES libsentry.so sentry)
    
elseif(APPLE)
    # macOS paths
    set(_SENTRY_SEARCH_PATHS
        /usr/local
        /opt/homebrew
        /opt/local
    )
    
    set(_SENTRY_INCLUDE_SUFFIXES
        include
        include/sentry
    )
    
    set(_SENTRY_LIB_SUFFIXES
        lib
    )
    
    set(_SENTRY_NAMES libsentry.dylib sentry)
    
else()
    # Windows paths
    set(_SENTRY_SEARCH_PATHS
        "C:/Program Files/Sentry"
        "C:/Program Files (x86)/Sentry"
        "${CMAKE_SOURCE_DIR}/3rdparty/sentry"
    )
    
    set(_SENTRY_INCLUDE_SUFFIXES
        include
        include/sentry
    )
    
    set(_SENTRY_LIB_SUFFIXES
        lib
        lib/x64
    )
    
    set(_SENTRY_NAMES sentry.lib libsentry.lib)
endif()

# Find include directory
find_path(SENTRY_INCLUDE_DIR
    NAMES sentry.h
    PATHS ${_SENTRY_SEARCH_PATHS}
    PATH_SUFFIXES ${_SENTRY_INCLUDE_SUFFIXES}
)

# Find library
find_library(SENTRY_LIBRARY
    NAMES ${_SENTRY_NAMES}
    PATHS ${_SENTRY_SEARCH_PATHS}
    PATH_SUFFIXES ${_SENTRY_LIB_SUFFIXES}
)

# Get version if possible
if(SENTRY_INCLUDE_DIR)
    file(STRINGS "${SENTRY_INCLUDE_DIR}/sentry.h" _sentry_version_line REGEX "^#define SENTRY_SDK_VERSION \"[0-9.]+\"$")
    if(_sentry_version_line)
        string(REGEX REPLACE "^#define SENTRY_SDK_VERSION \"([0-9.]+)\"$" "\\1" SENTRY_VERSION "${_sentry_version_line}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sentry
    REQUIRED_VARS 
        SENTRY_LIBRARY 
        SENTRY_INCLUDE_DIR
    VERSION_VAR
        SENTRY_VERSION
    FAIL_MESSAGE
        "Could NOT find Sentry. Please install Sentry SDK or set SENTRY_ROOT to the installation prefix."
)

if(SENTRY_FOUND AND NOT TARGET sentry::sentry)
    add_library(sentry::sentry UNKNOWN IMPORTED)
    set_target_properties(sentry::sentry PROPERTIES
        IMPORTED_LOCATION "${SENTRY_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SENTRY_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SENTRY_INCLUDE_DIR SENTRY_LIBRARY)