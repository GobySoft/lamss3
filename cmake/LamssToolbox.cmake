# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

# LamssToolbox.cmake - helpers shared by every lamss3 toolbox
#
#   lamss_toolbox(NAME <tb> [REQUIRES <tb>...])
#   lamss_add_library(<name> SOURCES <files>... [PROTOS <files>...] [LINK_LIBRARIES <libs>...]
#                     [NO_EXPORT])
#   lamss_add_proto(<name> PROTOS <files>... [LINK_LIBRARIES <libs>...])
#   lamss_add_app(<name> SOURCES <files>... [PROTOS <files>...] [LINK_LIBRARIES <libs>...] [NO_GOBY] [MOOS])
#   lamss_find_moos()
#   lamss_find_ivp(<lib>...)
#   lamss_export_code(<cmake code>...)
#   lamss_add_python(<package_dir>)
#   lamss_add_pytest(<name> <path> [PYTHONPATH <dirs>...])
#   lamss_add_mission_test(NAME <name> CRUISE <cruise> [VARIANT <variant>] TEST <file>
#                          [SET <key=value>...] [WAIT <seconds>] [TIMEOUT <seconds>])
#   lamss_add_test(<name> SOURCES <files>... [LINK_LIBRARIES <libs>...])
#   lamss_add_dccl_test(<name> SOURCES <files>... [LINK_LIBRARIES <libs>...])
#   lamss_toolbox_export()
#
# See core/src/doc/plan/claude/02-repository-layout-and-build.md.

include_guard(GLOBAL)

set(LAMSS_VERSION 3.0.0)
set(LAMSS_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(_lamss_find_root out_var start_dir)
  set(_dir "${start_dir}")
  while(TRUE)
    if(IS_DIRECTORY "${_dir}/.lamss")
      set(${out_var} "${_dir}" PARENT_SCOPE)
      return()
    endif()
    get_filename_component(_parent "${_dir}" DIRECTORY)
    if(_parent STREQUAL _dir)
      set(${out_var} "" PARENT_SCOPE)
      return()
    endif()
    set(_dir "${_parent}")
  endwhile()
endfunction()

if(NOT DEFINED LAMSS_ROOT)
  _lamss_find_root(LAMSS_ROOT "${CMAKE_SOURCE_DIR}")
endif()

if(LAMSS_ROOT)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${LAMSS_ROOT}/build/usr" CACHE PATH "Install prefix" FORCE)
  endif()
  # standalone toolbox builds find their upstream toolboxes here
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}")
endif()

find_program(LAMSS_PROTOC_GEN_DCCL protoc-gen-dccl)

# core installs DCCL_IDS.md beside this file for toolboxes built outside a lamss3 checkout
set(LAMSS_DCCL_IDS_MD "${LAMSS_CMAKE_DIR}/../core/src/lib/protobuf/lamss/DCCL_IDS.md")
if(NOT EXISTS "${LAMSS_DCCL_IDS_MD}")
  set(LAMSS_DCCL_IDS_MD "${LAMSS_CMAKE_DIR}/DCCL_IDS.md")
endif()

function(_lamss_joint_build out_var)
  get_property(_set GLOBAL PROPERTY LAMSS_ACTIVE_TOOLBOXES SET)
  set(${out_var} ${_set} PARENT_SCOPE)
endfunction()

function(_lamss_check_requires tb)
  _lamss_joint_build(_joint)
  get_property(_active GLOBAL PROPERTY LAMSS_ACTIVE_TOOLBOXES)
  foreach(_req IN LISTS ARGN)
    if(_joint)
      if(NOT _req IN_LIST _active)
        message(FATAL_ERROR
          "toolbox '${tb}' requires '${_req}', which is not active; "
          "run `lamss toolbox activate ${_req}`")
      endif()
    else()
      find_package(lamss_${_req} CONFIG QUIET)
      if(NOT lamss_${_req}_FOUND)
        message(FATAL_ERROR
          "standalone build of toolbox '${tb}' requires toolbox '${_req}' to be installed "
          "(searched CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}); build and install '${_req}' first")
      endif()
    endif()
  endforeach()
endfunction()

macro(lamss_toolbox)
  cmake_parse_arguments(_LAMSS_TB "" "NAME" "REQUIRES" ${ARGN})
  if(NOT _LAMSS_TB_NAME)
    message(FATAL_ERROR "lamss_toolbox: NAME is required")
  endif()
  set(LAMSS_TOOLBOX ${_LAMSS_TB_NAME})
  set_property(GLOBAL PROPERTY LAMSS_${LAMSS_TOOLBOX}_REQUIRES ${_LAMSS_TB_REQUIRES})
  _lamss_check_requires(${LAMSS_TOOLBOX} ${_LAMSS_TB_REQUIRES})

  include(GNUInstallDirs)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  add_compile_options(-Wall -Wextra)
  option(LAMSS_WERROR "Treat compiler warnings as errors" OFF)
  if(LAMSS_WERROR)
    add_compile_options(-Werror)
  endif()

  # relocatable within a prefix; other prefixes (e.g. a submodule toolbox installed elsewhere) via link path
  set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

  option(BUILD_TESTING "Build the tests" ON)
  enable_testing()

  if(NOT TARGET lamss_${LAMSS_TOOLBOX}_all)
    add_custom_target(lamss_${LAMSS_TOOLBOX}_all)
  endif()
endmacro()

function(_lamss_require_toolbox fn)
  if(NOT LAMSS_TOOLBOX)
    message(FATAL_ERROR "${fn} called before lamss_toolbox()")
  endif()
endfunction()

# goby-config.cmake prints its libraries on every call and ignores QUIET, and lamss3 finds Goby in
# each directory that uses it (its imported targets are directory-scoped), so report it once.
function(_lamss_find_goby_package name)
  if(NOT CMAKE_MESSAGE_LOG_LEVEL)
    set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
  endif()
  find_package(${name} ${ARGN})
  get_property(_reported GLOBAL PROPERTY LAMSS_GOBY_REPORTED)
  if(NOT _reported AND ${name}_FOUND)
    set_property(GLOBAL PROPERTY LAMSS_GOBY_REPORTED TRUE)
    unset(CMAKE_MESSAGE_LOG_LEVEL)
    message(STATUS "Found Goby ${${name}_VERSION}: ${GOBY_INCLUDE_DIR}")
  endif()
endfunction()

function(_lamss_find_goby)
  if(NOT TARGET goby)
    _lamss_find_goby_package(goby 3.6 REQUIRED)
  endif()
endfunction()

# Defines MOOS::MOOS (MOOS 10) and goby_moos (Goby3's MOOS library, with GobyMOOSApp) in the
# calling directory. MOOS comes first: goby_moos's package config refers to MOOS::MOOS without
# finding it, and only reads its components from GOBY_FIND_COMPONENTS, hence upper-case GOBY.
function(lamss_find_moos)
  if(NOT TARGET MOOS::MOOS)
    find_package(MOOS 10 REQUIRED)
  endif()
  if(NOT TARGET goby_moos)
    _lamss_find_goby_package(GOBY 3.6 REQUIRED COMPONENTS moos)
  endif()
  lamss_export_code("lamss_find_moos()")
endfunction()

# MOOS-IvP installs static libraries and headers but no CMake package: each <lib> becomes IvP::<lib>.
function(lamss_find_ivp)
  find_path(LAMSS_IVP_INCLUDE_DIR MBUtils.h PATH_SUFFIXES ivp REQUIRED)
  foreach(_lib IN LISTS ARGN)
    if(TARGET IvP::${_lib})
      continue()
    endif()
    find_library(LAMSS_IVP_${_lib}_LIBRARY NAMES ${_lib}
      HINTS "${LAMSS_IVP_INCLUDE_DIR}/../../lib" REQUIRED)
    add_library(IvP::${_lib} STATIC IMPORTED GLOBAL)
    set_target_properties(IvP::${_lib} PROPERTIES
      IMPORTED_LOCATION "${LAMSS_IVP_${_lib}_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LAMSS_IVP_INCLUDE_DIR}")
  endforeach()
  foreach(_lib IN LISTS ARGN)
    lamss_export_code("lamss_find_ivp(${_lib})")
  endforeach()
endfunction()

# Adds CMake code to the toolbox's installed package config, run before its targets are imported,
# so that the targets its exported libraries link (MOOS::MOOS, IvP::*, ...) exist downstream.
# lamss_find_moos() and lamss_find_ivp() add themselves.
function(lamss_export_code)
  if(LAMSS_TOOLBOX)
    set_property(GLOBAL APPEND PROPERTY LAMSS_${LAMSS_TOOLBOX}_EXPORT_CODE ${ARGN})
  endif()
endfunction()

# Runs protoc (and the DCCL plugin when present) on PROTOS, keeping each file's path relative to
# CMAKE_CURRENT_SOURCE_DIR, so `lamss/<tb>/foo.proto` generates `<out_dir>/lamss/<tb>/foo.pb.h`.
function(_lamss_generate_protos out_var out_dir)
  _lamss_find_goby()
  find_path(LAMSS_DCCL_PROTO_DIR dccl/option_extensions.proto)
  get_target_property(_goby_inc goby INTERFACE_INCLUDE_DIRECTORIES)
  get_property(_tb_roots GLOBAL PROPERTY LAMSS_PROTO_ROOTS)

  set(_import_flags)
  foreach(_dir
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${_tb_roots}
      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
      ${_goby_inc}
      ${LAMSS_DCCL_PROTO_DIR})
    # an installed include directory only exists once an upstream toolbox has been installed
    if(_dir AND NOT _dir MATCHES "^\\$<" AND IS_DIRECTORY "${_dir}")
      list(APPEND _import_flags "-I${_dir}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _import_flags)

  set(_plugin_flags)
  if(LAMSS_PROTOC_GEN_DCCL)
    set(_plugin_flags "--plugin=protoc-gen-dccl=${LAMSS_PROTOC_GEN_DCCL}" "--dccl_out=${out_dir}")
  endif()

  file(MAKE_DIRECTORY "${out_dir}")
  set(_generated)
  foreach(_proto IN LISTS ARGN)
    get_filename_component(_abs "${_proto}" ABSOLUTE)
    file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_abs}")
    if(_rel MATCHES "^\\.\\.")
      message(FATAL_ERROR "${_proto} is outside ${CMAKE_CURRENT_SOURCE_DIR}, the proto import root")
    endif()
    string(REGEX REPLACE "\\.proto$" "" _rel_we "${_rel}")
    set(_h "${out_dir}/${_rel_we}.pb.h")
    set(_cc "${out_dir}/${_rel_we}.pb.cc")
    add_custom_command(
      OUTPUT "${_h}" "${_cc}"
      COMMAND protobuf::protoc "--cpp_out=${out_dir}" ${_plugin_flags} ${_import_flags} "${_abs}"
      DEPENDS "${_abs}"
      COMMENT "Generating C++ from ${_rel}"
      VERBATIM)
    list(APPEND _generated "${_h}" "${_cc}")
  endforeach()
  set(${out_var} "${_generated}" PARENT_SCOPE)
endfunction()

function(lamss_add_library name)
  _lamss_require_toolbox(lamss_add_library)
  cmake_parse_arguments(_A "NO_EXPORT" "" "SOURCES;PROTOS;LINK_LIBRARIES" ${ARGN})
  set(_target lamss_${LAMSS_TOOLBOX}_${name})
  set(_srcs ${_A_SOURCES})
  if(_A_PROTOS)
    _lamss_generate_protos(_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}" ${_A_PROTOS})
    list(APPEND _srcs ${_proto_srcs})
  endif()
  add_library(${_target} SHARED ${_srcs})
  if(_A_PROTOS)
    target_include_directories(${_target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
  add_library(lamss::${LAMSS_TOOLBOX}_${name} ALIAS ${_target})
  string(REPLACE "." ";" _v "${LAMSS_VERSION}")
  list(GET _v 0 _major)
  set_target_properties(${_target} PROPERTIES
    EXPORT_NAME ${LAMSS_TOOLBOX}_${name}
    VERSION ${LAMSS_VERSION}
    SOVERSION ${_major})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(${_target} PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()
  target_include_directories(${_target} INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(${_target} PUBLIC ${_A_LINK_LIBRARIES})
  # NO_EXPORT: only ever loaded with dlopen, so nothing downstream links it
  if(_A_NO_EXPORT)
    install(TARGETS ${_target} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  else()
    install(TARGETS ${_target} EXPORT lamss_${LAMSS_TOOLBOX}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    set_property(GLOBAL APPEND PROPERTY LAMSS_${LAMSS_TOOLBOX}_EXPORTED ${_target})
  endif()
  add_dependencies(lamss_${LAMSS_TOOLBOX}_all ${_target})
endfunction()

# Call from the proto import root, e.g. src/lib/protobuf, with PROTOS under lamss/<tb>/.
function(lamss_add_proto name)
  _lamss_require_toolbox(lamss_add_proto)
  cmake_parse_arguments(_A "" "" "PROTOS;LINK_LIBRARIES" ${ARGN})
  set_property(GLOBAL APPEND PROPERTY LAMSS_PROTO_ROOTS "${CMAKE_CURRENT_SOURCE_DIR}")
  set(_out "${CMAKE_CURRENT_BINARY_DIR}/generated")
  _lamss_generate_protos(_srcs "${_out}" ${_A_PROTOS})
  lamss_add_library(${name} SOURCES ${_srcs} LINK_LIBRARIES goby ${_A_LINK_LIBRARIES})
  target_include_directories(lamss_${LAMSS_TOOLBOX}_${name} PUBLIC $<BUILD_INTERFACE:${_out}>)
  foreach(_proto IN LISTS _A_PROTOS)
    get_filename_component(_rel_dir "${_proto}" DIRECTORY)
    string(REGEX REPLACE "\\.proto$" ".pb.h" _rel_h "${_proto}")
    install(FILES "${_proto}" "${_out}/${_rel_h}"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${_rel_dir})
  endforeach()
endfunction()

function(lamss_add_app name)
  _lamss_require_toolbox(lamss_add_app)
  cmake_parse_arguments(_A "NO_GOBY;MOOS" "" "SOURCES;PROTOS;LINK_LIBRARIES" ${ARGN})
  set(_srcs ${_A_SOURCES})
  if(_A_PROTOS)
    _lamss_generate_protos(_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}" ${_A_PROTOS})
    list(APPEND _srcs ${_proto_srcs})
  endif()
  add_executable(${name} ${_srcs})
  target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  if(NOT _A_NO_GOBY)
    _lamss_find_goby()
    target_link_libraries(${name} PRIVATE goby)
  endif()
  if(_A_MOOS)
    lamss_find_moos()
    if(_A_NO_GOBY)
      target_link_libraries(${name} PRIVATE MOOS::MOOS)
    else()
      target_link_libraries(${name} PRIVATE goby_moos)
    endif()
  endif()
  target_link_libraries(${name} PRIVATE ${_A_LINK_LIBRARIES})
  install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  add_dependencies(lamss_${LAMSS_TOOLBOX}_all ${name})
endfunction()

function(lamss_add_python package_dir)
  _lamss_require_toolbox(lamss_add_python)
  install(DIRECTORY "${package_dir}"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/python3/dist-packages
    PATTERN "__pycache__" EXCLUDE)
endfunction()

# Registers `python3 -m pytest <path>` as a ctest, run against the source tree (PYTHONPATH) so no
# install is needed. Skipped with a note when pytest is not installed.
function(lamss_add_pytest name path)
  _lamss_require_toolbox(lamss_add_pytest)
  cmake_parse_arguments(_A "" "" "PYTHONPATH" ${ARGN})
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  if(NOT DEFINED LAMSS_HAVE_PYTEST)
    execute_process(COMMAND ${Python3_EXECUTABLE} -c "import pytest"
      RESULT_VARIABLE _status OUTPUT_QUIET ERROR_QUIET)
    if(_status EQUAL 0)
      set(LAMSS_HAVE_PYTEST TRUE CACHE INTERNAL "")
    else()
      set(LAMSS_HAVE_PYTEST FALSE CACHE INTERNAL "")
    endif()
  endif()
  if(NOT LAMSS_HAVE_PYTEST)
    message(STATUS "${name}: not registered, pytest is not installed (python3-pytest)")
    return()
  endif()
  get_filename_component(_path "${path}" ABSOLUTE)
  string(REPLACE ";" ":" _pythonpath "${_A_PYTHONPATH}")
  add_test(NAME ${name}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${_pythonpath} PYTHONDONTWRITEBYTECODE=1
      ${Python3_EXECUTABLE} -m pytest -q -p no:cacheprovider "${_path}")
endfunction()

# Runs `lamss mission test` on one goby test file. The mission runs from the install prefix, so
# run it after installing (`lamss build --test` does). The instance is named after the test, so
# tests of one cruise can run in parallel; ports are picked free unless SET gives port_base.
function(lamss_add_mission_test)
  _lamss_require_toolbox(lamss_add_mission_test)
  cmake_parse_arguments(_A "" "NAME;CRUISE;VARIANT;TEST;WAIT;TIMEOUT" "SET" ${ARGN})
  foreach(_required NAME CRUISE TEST)
    if(NOT _A_${_required})
      message(FATAL_ERROR "lamss_add_mission_test: ${_required} is required")
    endif()
  endforeach()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  if(NOT GOBY_EXECUTABLE)
    _lamss_find_goby_package(goby 3.6 QUIET)
  endif()
  if(GOBY_EXECUTABLE)
    set(_goby "${GOBY_EXECUTABLE}")
  else()
    find_program(LAMSS_GOBY_TOOL goby REQUIRED)
    set(_goby "${LAMSS_GOBY_TOOL}")
  endif()
  # the source tree's lamss.mission when core is part of this build, else the installed one
  if(IS_DIRECTORY "${LAMSS_CMAKE_DIR}/../core/src/python/lamss/mission")
    get_filename_component(_pythonpath "${LAMSS_CMAKE_DIR}/../core/src/python" ABSOLUTE)
  else()
    set(_pythonpath "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/python3/dist-packages")
  endif()
  get_filename_component(_test "${_A_TEST}" ABSOLUTE)
  if(NOT _A_WAIT)
    set(_A_WAIT 60)
  endif()
  if(NOT _A_TIMEOUT)
    math(EXPR _A_TIMEOUT "${_A_WAIT} + 240")
  endif()
  set(_args ${_A_CRUISE} ${_A_VARIANT} --instance ${_A_NAME} --test-file ${_test} --wait ${_A_WAIT}
    --goby ${_goby})
  foreach(_set IN LISTS _A_SET)
    list(APPEND _args --set ${_set})
  endforeach()
  add_test(NAME ${_A_NAME}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${_pythonpath} PYTHONDONTWRITEBYTECODE=1
      LAMSS_ROOT=${LAMSS_ROOT} LAMSS_PREFIX=${CMAKE_INSTALL_PREFIX}
      ${Python3_EXECUTABLE} -m lamss.mission test ${_args})
  set_tests_properties(${_A_NAME} PROPERTIES TIMEOUT ${_A_TIMEOUT})
endfunction()

# Test executables are part of lamss_<tb>_all so `lamss build --toolbox <tb> --test` builds them.
function(lamss_add_test name)
  _lamss_require_toolbox(lamss_add_test)
  cmake_parse_arguments(_A "" "" "SOURCES;LINK_LIBRARIES" ${ARGN})
  add_executable(${name} ${_A_SOURCES})
  target_link_libraries(${name} PRIVATE ${_A_LINK_LIBRARIES})
  add_test(NAME ${name} COMMAND ${name})
  add_dependencies(lamss_${LAMSS_TOOLBOX}_all ${name})
endfunction()

# Includes lamss_dccl_test.h, for checking a toolbox's DCCL messages against DCCL_IDS.md.
function(lamss_add_dccl_test name)
  lamss_add_test(${name} ${ARGN})
  target_include_directories(${name} PRIVATE "${LAMSS_CMAKE_DIR}")
  target_compile_definitions(${name} PRIVATE LAMSS_DCCL_IDS_MD="${LAMSS_DCCL_IDS_MD}")
endfunction()

function(lamss_toolbox_export)
  _lamss_require_toolbox(lamss_toolbox_export)
  set(_tb ${LAMSS_TOOLBOX})
  set(_dest ${CMAKE_INSTALL_LIBDIR}/cmake/lamss_${_tb})
  get_property(_requires GLOBAL PROPERTY LAMSS_${_tb}_REQUIRES)
  get_property(_exported GLOBAL PROPERTY LAMSS_${_tb}_EXPORTED)

  file(RELATIVE_PATH _up "/${_dest}" "/")
  set(_config "${CMAKE_CURRENT_BINARY_DIR}/lamss_${_tb}Config.cmake")
  file(WRITE "${_config}"
    "include(CMakeFindDependencyMacro)\n"
    "set(_lamss_log_level \"\${CMAKE_MESSAGE_LOG_LEVEL}\")\n"
    "if(NOT CMAKE_MESSAGE_LOG_LEVEL)\n"
    "  set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)\n"
    "endif()\n"
    "find_dependency(goby 3.6)\n"
    "set(CMAKE_MESSAGE_LOG_LEVEL \"\${_lamss_log_level}\")\n"
    "unset(_lamss_log_level)\n"
    "get_filename_component(_lamss_prefix \"\${CMAKE_CURRENT_LIST_DIR}/${_up}\" ABSOLUTE)\n"
    "set_property(GLOBAL APPEND PROPERTY LAMSS_PROTO_ROOTS \"\${_lamss_prefix}/${CMAKE_INSTALL_INCLUDEDIR}\")\n")
  foreach(_req IN LISTS _requires)
    file(APPEND "${_config}" "find_dependency(lamss_${_req} ${LAMSS_VERSION})\n")
  endforeach()
  get_property(_code GLOBAL PROPERTY LAMSS_${_tb}_EXPORT_CODE)
  if(_code)
    list(REMOVE_DUPLICATES _code)
    file(APPEND "${_config}"
      "if(NOT COMMAND lamss_export_code)\n"
      "  include(\"\${CMAKE_CURRENT_LIST_DIR}/../lamss_core/LamssToolbox.cmake\")\n"
      "endif()\n")
    foreach(_line IN LISTS _code)
      file(APPEND "${_config}" "${_line}\n")
    endforeach()
  endif()
  file(APPEND "${_config}"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/lamss_${_tb}Targets.cmake\" OPTIONAL)\n"
    "list(APPEND CMAKE_MODULE_PATH \"\${CMAKE_CURRENT_LIST_DIR}\")\n")

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/lamss_${_tb}ConfigVersion.cmake"
    VERSION ${LAMSS_VERSION} COMPATIBILITY SameMinorVersion)
  install(FILES "${_config}" "${CMAKE_CURRENT_BINARY_DIR}/lamss_${_tb}ConfigVersion.cmake"
    DESTINATION ${_dest})
  if(_exported)
    install(EXPORT lamss_${_tb}Targets NAMESPACE lamss:: DESTINATION ${_dest})
  endif()
endfunction()
