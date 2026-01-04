#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

# Matches Corrade requirement, see its root CMakeLists for more information.
cmake_minimum_required(VERSION 3.5...3.10)

# CMake 3.12+ uses the policy max version specified in
# cmake_minimum_required(), meaning that with ...3.10, everything until CMP0071
# gets set to NEW implicitly. We however want to keep compatibility with
# versions before 3.12, so the NEW policies are still being hand-picked. Also
# don't want to do a blanket cmake_policy(VERSION) because that may break
# behavior for existing projects that rely on the OLD behavior.

# Don't restrict INTERPROCEDURAL_OPTIMIZATION only for icc on Linux
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()
# If CMAKE_AUTOMOC is set, all uses of corrade_add_resource() would otherwise
# complain on 3.10 that AUTOMOC is not processing GENERATED files
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()
# Allow <PackageName>_ROOT to be used on 3.12+ to point to per-package install
# locations that find_package(PackageName) subsequently picks up
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
# Allow also <PACKAGENAME>_ROOT (i.e., uppercase), on 3.27+
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
# Superprojects can use just set(MAGNUM_WITH_BLAH ON) without FORCE CACHE on
# 3.13+
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

project(MagnumBindings CXX)

# Use folders for nice tree in Visual Studio and XCode
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})
find_package(Magnum REQUIRED)

include(CMakeDependentOption)

# Options that used to be unprefixed. New options shouldn't be added to this
# list.
set(_MAGNUMBINDINGS_DEPRECATED_UNPREFIXED_OPTIONS
    WITH_PYTHON
    BUILD_TESTS)
# If during the first run (i.e., when the variable isn't in cache yet), check
# if any of the prefixed options are already set. If so, we assume the user is
# already switched to the prefixed options and won't accept the deprecated
# unprefixed options for backwards compatibility. This way it's possible for
# projects to reuse these variables for other purposes without affecting
# Corrade in any way.
if(NOT DEFINED _MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
    foreach(option ${_MAGNUMBINDINGS_DEPRECATED_UNPREFIXED_OPTIONS})
        if(DEFINED MAGNUM_${option})
            set(_MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS OFF CACHE INTERNAL "")
            break()
        endif()
    endforeach()
endif()

# Libraries to build
option(MAGNUM_WITH_PYTHON "Build Python bindings" OFF)
# There's no global MAGNUM_BUILD_PLUGINS_STATIC option (only exposed as
# per-plugin define), so not checking it.
if(MAGNUM_WITH_PYTHON AND MAGNUM_BUILD_STATIC)
    set(MAGNUM_PYTHON_BINDINGS_STATIC_PLUGINS "" CACHE STRING "Static plugins to link to Magnum Python bindings")
endif()

option(MAGNUM_BUILD_TESTS "Build unit tests" OFF)

if(MAGNUM_WITH_PYTHON AND MAGNUM_BUILD_STATIC AND UNIX)
    # See src/python/corrade/__init__.py.in and the corresponding CMakeLists
    # for details
    option(MAGNUM_BUILD_PYTHON_BINDINGS_RTLD_GLOBAL "Build Python bindings linking to static libraries with RTLD_GLOBAL enabled for loading" ON)
endif()

# Python since version 3.8 doesn't take dependency DLLs implicitly from PATH
# anymore, and one has to either place them right next to the Python modules,
# into system locations, or add extra paths via os.add_dll_directory().
#
# Be nice to the user and implicitly populate that with Corrade and Magnum DLL
# directories, if found, on the first run. Anything else (such as SDL or GLFW
# DLL path) has to be specified by the user.
if(MAGNUM_WITH_PYTHON AND WIN32)
    # Do this lookup only on the first ever run. But add the option always, so
    # in case it's passed via command line in the initial CMake run, it
    # actually becomes a cached option with a help text and not just a loose
    # variable.
    if(NOT MAGNUM_PYTHON_BINDINGS_DLL_PATH)
        set(dll_path )
        foreach(dll_variable CORRADE_UTILITY_DLL_DEBUG CORRADE_UTILITY_DLL_RELEASE MAGNUM_DLL_DEBUG MAGNUM_DLL_RELEASE)
            if(${dll_variable})
                get_filename_component(dll_directory ${${dll_variable}} DIRECTORY)
                list(APPEND dll_path ${dll_directory})
            endif()
        endforeach()
        list(REMOVE_DUPLICATES dll_path)

        # If no DLLs were found, it means Corrade and Magnum is static and thus no
        # DLL directories may need to be passed
        if(dll_path)
            message(STATUS "Autodetected ${dll_path} as directories to pass to Python for finding Corrade, Magnum and other dependency DLLs. Update the MAGNUM_PYTHON_BINDINGS_DLL_PATH variable if needed.")
        endif()
    endif()

    # This variable is then used in src/python/corrade/__init__.py.in
    set(MAGNUM_PYTHON_BINDINGS_DLL_PATH "${dll_path}" CACHE STRING "Semicolon-separated directories where to look for Corrade, Magnum and other dependency DLLs")
endif()

# Backwards compatibility for unprefixed CMake options. If the user isn't
# explicitly using prefixed options in the first run already, accept the
# unprefixed options, and remember this decision for subsequent runs
if(NOT DEFINED _MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS)
    set(_MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS ON CACHE INTERNAL "")
endif()
# If the user wasn't explicitly using prefixed options in the first run and the
# MAGNUM_BUILD_DEPRECATED option is not currently disabled (which can get
# changed subsequently), accept the unprefixed options and print a warning if
# they're different from the prefixed ones.
if(_MAGNUMBINDINGS_ACCEPT_DEPRECATED_UNPREFIXED_OPTIONS AND MAGNUM_BUILD_DEPRECATED)
    set(_MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION )
    foreach(option ${_MAGNUMBINDINGS_DEPRECATED_UNPREFIXED_OPTIONS})
        if(DEFINED ${option})
            # CMake has no comparison of boolean values (EQUAL returns false if
            # comparing ON and 1 or OFF and FALSE, STREQUAL also), so we have
            # to do it this way. Also warn only on the first encountered
            # variable so people can fix it, reconfigure and go to the next one
            # that warns.
            if((${option} AND NOT MAGNUM_${option}) OR
               (NOT ${option} AND MAGNUM_${option}) AND NOT _MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION)
                set(_MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION ${option})
            endif()
            set(MAGNUM_${option} ${${option}})
            # If variables specified on the command line don't match any
            # options, they're kept in cache but set as UNINITIALIZED, meaning
            # they don't appear in cmake-gui or ccmake, so there's no way to
            # fix the warning apart from hand-enditing the CMakeCache.txt or
            # recreating the build dir. Update their cached type to be BOOL to
            # make them appear.
            set(${option} ${${option}} CACHE BOOL "Deprecated, use MAGNUM_${option} instead" FORCE)
        endif()
    endforeach()

    if(_MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION)
        message(DEPRECATION "Unprefixed options such as ${_MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION} are deprecated, use MAGNUM_${_MAGNUMBINDINGS_WARN_DEPRECATED_UNPREFIXED_OPTION} instead. Delete the unprefixed variable from CMake cache or set both to the same value to silence this warning.")
    endif()
endif()

if(MAGNUM_BUILD_TESTS)
    find_package(Corrade REQUIRED TestSuite)
    if(CORRADE_TARGET_IOS)
        set(CORRADE_TESTSUITE_BUNDLE_IDENTIFIER_PREFIX "cz.mosra.magnum-bindings")
    endif()
    enable_testing()

    # If CORRADE_TESTSUITE_TEST_TARGET is set, tests aren't built by default
    # (in the ALL target) but instead set as dependencies of a target named
    # after the value of CORRADE_TESTSUITE_TEST_TARGET. This is a copy of
    # what's done in corrade_add_test(), because we also build various test
    # libraries and plugins in addition to the test executables.
    if(CORRADE_TESTSUITE_TEST_TARGET)
        if(NOT TARGET ${CORRADE_TESTSUITE_TEST_TARGET})
            add_custom_target(${CORRADE_TESTSUITE_TEST_TARGET})
        endif()
        set(EXCLUDE_FROM_ALL_IF_TEST_TARGET EXCLUDE_FROM_ALL)
    endif()
endif()

set(MAGNUMBINDINGS_CMAKE_MODULE_INSTALL_DIR share/cmake/MagnumBindings)

# Library version. MAGNUMBINDINGS_VERSION_YEAR/MONTH is used in
# src/Magnum/CMakeLists.txt to generate the versionBindings.h header.
set(MAGNUMBINDINGS_VERSION_YEAR 2020)
set(MAGNUMBINDINGS_VERSION_MONTH 6)

# A single output location. After a decade of saying NO THIS IS A NON-SOLUTION
# TO A NON-PROBLEM I reconsidered my views and enabled this, because:
#
# - On Windows (which don't have RPATH), this makes test execution finally
#   possible without having to install all the stuff first (including the
#   test-only libs, which is ugh).
# - With CMake subprojects, this makes it finally possible to use dynamic
#   plugins directly from the build dir (again without installing anything) ---
#   all plugins are put into the same place, so PluginManager has a single
#   place to look into; and thanks to the dynamic libraries being there as
#   well, this location can be automagically detected as relative to
#   Utility::Path::libraryLocation().
# - Thanks to the $<CONFIG> being part of the output path, you are always sure
#   you never accidentally mix up debug/release libraries when switching
#   CMAKE_BUILD_TYPE in an existing build dir.
#
# The runtime location is set to CMAKE_BINARY_DIR and not PROJECT_BINARY_DIR
# because have one runtime location per CMake subproject would not solve much
# either. If the user already provides CMAKE_RUNTIME_OUTPUT_DIRECTORY (even
# empty), it's respected and nothing is being done.
#
# Explicitly using a generator expression to ensure plugins are added to e.g.
# <CONFIG>/lib/magnum/importers/ instead of lib/magnum/importers/<CONFIG>. Also
# adding this to cache, making superprojects pick that up implicitly as well,
# without forcing them to explicitly mirror this setting.
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib CACHE PATH "" FORCE)
    # There should be no need for the "90% use case" user to adjust these, so
    # don't show them in the default view
    mark_as_advanced(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        CMAKE_LIBRARY_OUTPUT_DIRECTORY
        CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
endif()

add_subdirectory(modules)
add_subdirectory(src)
