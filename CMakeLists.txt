##################################
# Basic CMake Configuration
##################################
cmake_minimum_required (VERSION 2.8)
set (CMAKE_COLOR_MAKEFILE TRUE)
set (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
cmake_policy (SET CMP0003 NEW)
cmake_policy (SET CMP0005 NEW)

# Use relative paths
# This is mostly to reduce path size for command-line limits on windows
if (WIN32)
  # This supposedly breaks Xcode projects so don't enable on Apple builds
  set (CMAKE_USE_RELATIVE_PATHS true)
  set (CMAKE_SUPPRESS_REGENERATION true)
endif ()

project (QuesoGLC) # CXX and C are default languages.
set (QUESOGLC_VERSION 0.7.9)
add_definitions ("-DQUESOGLC_VERSION=\"${QUESOGLC_VERSION}\"")

include (FindPkgConfig)
include (CheckCSourceRuns)
include (CheckFunctionExists)

#####################################################################
# Set up the basic build environment
#####################################################################

if (CMAKE_BUILD_TYPE STREQUAL "")
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set (CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

if (MSVC)
  if (CMAKE_BUILD_TOOL STREQUAL "nmake")
    # set variable to state that we are using nmake makefiles
  set (NMAKE TRUE)
  endif ()
endif ()

if (MINGW)
  add_definitions (-D_WIN32_WINNT=0x0500) # enable Win 2k / Win XP APIs on mingw
endif ()

if (UNIX AND NOT WIN32)
  string (REGEX MATCH [fF][rR][eE]{2}[bB][sS][dD] IS_FREEBSD ${CMAKE_SYSTEM_NAME})
  if (NOT IS_FREEBSD)
    add_definitions (-D_XOPEN_SOURCE=600)
  endif ()
endif ()

# Set compiler specific build flags
if (CMAKE_COMPILER_IS_GNUCXX)
  # Set our own default flags at first run.
  if (NOT CONFIGURED)

    if (ENABLE_STRICT_COMPILATION)
      set (STRICT_FLAGS "-ansi -Wextra -pedantic -Werror -Wno-strict-aliasing")
    else (ENABLE_STRICT_COMPILATION)
      set (STRICT_FLAGS "")
    endif ()

    # GCC >= 4.0
    exec_program (
      ${CMAKE_C_COMPILER}
      ARGS ${CMAKE_C_COMPILER_ARG1} -dumpversion
      OUTPUT_VARIABLE GCC_VERSION
    )
    if (GCC_VERSION MATCHES "4\\.[0-9]\\.[0-9]")
      set (HIDE_SYMBOLS_FLAGS "-fvisibility=hidden" CACHE INTERNAL "")
    else ()
      set (HIDE_SYMBOLS_FLAGS "" CACHE INTERNAL "")
    endif ()

    # CMAKE_C_FLAGS gets appended to the other C flags
    set(CMAKE_C_FLAGS "-Wall $ENV{CFLAGS}"
        CACHE STRING "Flags used by the C compiler during normal builds." FORCE)

    set(CMAKE_C_FLAGS_DEBUG "-g -DDDEBUG"
        CACHE STRING "Flags used by the C compiler during debug builds." FORCE)
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG"
        CACHE STRING "Flags used by the C compiler during release builds." FORCE)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG"
        CACHE STRING "Flags used by the C compiler during release builds." FORCE)
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG"
        CACHE STRING "Flags used by the C compiler during release builds." FORCE)

    # CMAKE_CXX_FLAGS gets appended to the other CXX flags
    set(CMAKE_CXX_FLAGS "-Wall ${STRICT_FLAGS} $ENV{CXXFLAGS}"
        CACHE STRING "Flags used by the CXX compiler during normal builds." FORCE)

    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG"
        CACHE STRING "Flags used by the CXX compiler during debug builds." FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG"
        CACHE STRING "Flags used by the CXX compiler during release builds." FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG"
        CACHE STRING "Flags used by the CXX compiler during release builds." FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG"
        CACHE STRING "Flags used by the CXX compiler during release builds." FORCE)

  endif (NOT CONFIGURED)
endif ()

#######################################################################
# Dependencies
#######################################################################

find_package (OpenGL REQUIRED)
find_package (Freetype REQUIRED)
find_package (PythonInterp REQUIRED)

set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads REQUIRED)

pkg_check_modules (GLEW glew)
pkg_check_modules (FRIBIDI fribidi)
pkg_check_modules (FONTCONFIG fontconfig REQUIRED)

find_package (GLUT) # for examples and tests

# Add library and include paths from the dependencies
include_directories (
  ${OPENGL_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${FRIBIDI_INCLUDE_DIRS}
  ${FONTCONFIG_INCLUDE_DIRS}
)

if (GLUT_FOUND)
  include_directories (${GLUT_INCLUDE_DIRS})
endif ()

# TLS check TODO: put it in its own module

set (CMAKE_REQUIRED_FLAGS "-static")
CHECK_C_SOURCE_RUNS ("__thread int a; int b; int main() { return a = b; }" TLS_RUNS_STATIC)
unset (CMAKE_REQUIRED_FLAGS)
CHECK_C_SOURCE_RUNS ("__thread int a; int b; int main() { return a = b; }" TLS_RUNS_DYN)
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
CHECK_C_SOURCE_RUNS (
"#include <pthread.h>
__thread int a;
static int *a_in_other_thread;
static void *
thread_func (void *arg)
{
    a_in_other_thread = &a;
    return (void *)0;
}
int main() {
    pthread_t thread;
    void *thread_retval;
    int *a_in_main_thread;
    if (pthread_create (&thread, (pthread_attr_t *)0, thread_func, (void *)0))
        return 0;
    a_in_main_thread = &a;
    if (pthread_join (thread, &thread_retval))
        return 0;
    return (a_in_other_thread == a_in_main_thread);
}
" TLS_LIBC
)
unset (CMAKE_REQUIRED_LIBRARIES)

if (TLS_RUNS_STATIC AND TLS_RUNS_DYN AND TLS_LIBC)
  add_definitions (-DHAVE_TLS=1)
  set (HAVE_TLS TRUE)
endif ()

# Freetype caching check
set (CMAKE_REQUIRED_LIBRARIES ${FREETYPE_LIBRARIES})
CHECK_FUNCTION_EXISTS (FTC_Manager_New HAVE_FT_CACHE)
unset (CMAKE_REQUIRED_LIBRARIES)

if (HAVE_FT_CACHE)
  add_definitions (-DHAVE_FT_CACHE=1)
endif ()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Note that static libraries are treated as archive targets
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

######################################################################
# Provide user options to customise the build process
######################################################################

option (BUILD_SHARED_LIBS "Build shared libraries if true, otherwise build static libraries." TRUE)
option (BUILD_EXAMPLES "Build the demo executables." FALSE)
option (USE_SYSTEM_GLEW "Use system glew library." TRUE)
option (USE_SYSTEM_FRIBIDI "Use system freebidi library." TRUE)
option (ENABLE_MULTICONTEXT "Enable multiple opengl context support." FALSE)
option (ENABLE_STRICT_COMPILATION "Compile using strict warning flags." FALSE)

# hide advanced options
mark_as_advanced (FORCE ENABLE_STRICT_COMPILATION USE_SYSTEM_GLEW USE_SYSTEM_FRIBIDI)

###################################################################
# configure global build settings based on selected build options
###################################################################

if (ENABLE_MULTICONTEXT)
  add_definitions (-DGLEW_MX)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions (-DDEBUGMODE)
endif ()

# Pkgconfig file
set (configure_input "quesoglc.pc.  Generated from quesoglc.pc.in by cmake.")
set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix "\${prefix}")
set (libdir "\${exec_prefix}/lib")
set (includedir "\${prefix}/include")

# FIXME: hardcoded stuff below
if (THREADS_HAVE_PTHREAD_ARG)
  set (PKGCONFIG_INCLUDE ${CMAKE_THREAD_LIBS_INIT})
else ()
  set (PKGCONFIG_LIBS_PRIVATE "-lGL ${CMAKE_THREAD_LIBS_INIT}")
endif ()

set (PKGCONFIG_REQUIREMENTS ", glew")

configure_file (quesoglc.pc.in ${CMAKE_BINARY_DIR}/quesoglc.pc @ONLY)
install (FILES ${CMAKE_BINARY_DIR}/quesoglc.pc DESTINATION lib/pkgconfig)

##################################################################
# Now setup targets
##################################################################

if (NOT USE_SYSTEM_GLEW OR NOT USE_SYSTEM_FRIBIDI)
  add_subdirectory (3rdparty)
endif ()

include_directories (include)
add_subdirectory (src)

if (BUILD_EXAMPLES AND NOT GLUT_FOUND)
  message ("Cannot build examples without GLUT")
elseif (BUILD_EXAMPLES)
  add_subdirectory (examples)
endif ()

# TODO : Tests

message (STATUS "Configuration Summary")
message (STATUS "=================")

if (USE_SYSTEM_FRIBIDI)
  message (STATUS "FriBiDi: Using system library")
endif ()

if (USE_SYSTEM_GLEW AND ENABLE_MULTICONTEXT)
  message (STATUS "GLEW: Use system library with support for multiple rendering contexts")
else ()
  message (STATUS "GLEW: Use system library without support for multiple rendering contexts")
endif ()

if (${HAVE_TLS})
  message (STATUS "Support for ELF TLS: Enabled")
else ()
  message (STATUS "Support for ELF TLS: Disabled")
endif ()

if (BUILD_EXAMPLES)
  message (STATUS "Build examples and tests: Yes")
else ()
  message (STATUS "Build examples and tests: No")
endif ()

## Other stuff
set (CONFIGURED YES CACHE INTERNAL "")

# vim: ts=2:sw=2
