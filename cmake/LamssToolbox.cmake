# LamssToolbox.cmake - helpers shared by every lamss3 toolbox
#
#   lamss_toolbox(NAME <tb> [REQUIRES <tb>...])
#   lamss_add_library(<name> SOURCES <files>... [LINK_LIBRARIES <libs>...])
#   lamss_add_proto(<name> PROTOS <files>... [LINK_LIBRARIES <libs>...])
#   lamss_add_app(<name> SOURCES <files>... [PROTOS <files>...] [LINK_LIBRARIES <libs>...] [NO_GOBY])
#   lamss_add_python(<package_dir>)
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

function(_lamss_find_goby)
  if(NOT TARGET goby)
    find_package(goby 3.6 REQUIRED)
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
    if(_dir AND NOT _dir MATCHES "NOTFOUND$" AND NOT _dir MATCHES "^\\$<")
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
  cmake_parse_arguments(_A "" "" "SOURCES;LINK_LIBRARIES" ${ARGN})
  set(_target lamss_${LAMSS_TOOLBOX}_${name})
  add_library(${_target} SHARED ${_A_SOURCES})
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
  install(TARGETS ${_target} EXPORT lamss_${LAMSS_TOOLBOX}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  set_property(GLOBAL APPEND PROPERTY LAMSS_${LAMSS_TOOLBOX}_EXPORTED ${_target})
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
  cmake_parse_arguments(_A "NO_GOBY" "" "SOURCES;PROTOS;LINK_LIBRARIES" ${ARGN})
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
    "find_dependency(goby 3.6)\n"
    "get_filename_component(_lamss_prefix \"\${CMAKE_CURRENT_LIST_DIR}/${_up}\" ABSOLUTE)\n"
    "set_property(GLOBAL APPEND PROPERTY LAMSS_PROTO_ROOTS \"\${_lamss_prefix}/${CMAKE_INSTALL_INCLUDEDIR}\")\n")
  foreach(_req IN LISTS _requires)
    file(APPEND "${_config}" "find_dependency(lamss_${_req} ${LAMSS_VERSION})\n")
  endforeach()
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
