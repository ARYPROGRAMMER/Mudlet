# Copyright (C) 2024 by ARYPROGRAMMER
# Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com

if(USE_CRASH_REPORTING)
    # Add Sentry as a third-party dependency
    add_subdirectory(sentry)
    
    # Configure Sentry build options
    set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "Build Sentry as a static library")
    set(SENTRY_BUILD_TESTS OFF CACHE BOOL "Don't build Sentry tests")
    set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "Don't build Sentry examples")
    
    # Platform specific configurations
    if(WIN32)
        set(SENTRY_TRANSPORT_WINHTTP ON CACHE BOOL "Use WinHTTP transport on Windows")
    else()
        set(SENTRY_TRANSPORT_CURL ON CACHE BOOL "Use CURL transport on Unix systems")
    endif()
endif()