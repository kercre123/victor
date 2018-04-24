#
# anki_build_source_list.cmake
#
# Helper macros to integrate cmake + metabuild.py
#
macro(anki_build_source_list target_name srclist_dir)

set(SRC_LIST_FILE "${srclist_dir}/${target_name}.srcs.lst")
set(HDR_LIST_FILE "${srclist_dir}/${target_name}.headers.lst")

if (EXISTS ${SRC_LIST_FILE})
  file(STRINGS ${SRC_LIST_FILE} SRCS)
else()
  message(FATAL_ERROR ${SRC_LIST_FILE} " not found. Did you forget to add your BUILD.in to build-victor.sh?")
endif()

set(HEADERS "")
if (EXISTS ${HDR_LIST_FILE})
  file(STRINGS ${HDR_LIST_FILE} HEADERS)
endif()

set(_ab_PLATFORM_SRCS "")
set(_ab_PLATFORM_HEADERS "")

if (ANDROID)
  set(_ab_srcfile "${srclist_dir}/${target_name}_android.srcs.lst")
  set(_ab_hdrfile "${srclist_dir}/${target_name}_android.headers.lst")
elseif (MACOSX)
  set(_ab_srcfile "${srclist_dir}/${target_name}_mac.srcs.lst")
  set(_ab_hdrfile "${srclist_dir}/${target_name}_mac.headers.lst")
elseif (VICOS)
  set(_ab_srcfile "${srclist_dir}/${target_name}_vicos.srcs.lst")
  set(_ab_hdrfile "${srclist_dir}/${target_name}_vicos.headers.lst")
else()
  set(_ab_srcfile "")
  set(_ab_hdrfile "")
endif()

if (EXISTS ${_ab_srcfile})
  file(STRINGS ${_ab_srcfile} _ab_PLATFORM_SRCS)
endif()

if (EXISTS ${_ab_hdrfile})
  file(STRINGS ${_ab_hdrfile} _ab_PLATFORM_HEADERS)
endif()

set(_ab_GO_DIR "")
if (EXISTS "${srclist_dir}/${target_name}.godir.lst")
  file(STRINGS "${srclist_dir}/${target_name}.godir.lst" _ab_GO_DIR)
endif()

endmacro()

macro(anki_build_absolute_source_list target_name srclist_dir)
  anki_build_source_list(${target_name} ${srclist_dir})
  # transform source files into absolute paths
  set(__temp_srclist "")
  foreach(i ${SRCS})
    set(__temp_srci "${CMAKE_CURRENT_SOURCE_DIR}/${i}")
  list(APPEND __temp_srclist ${__temp_srci})
  endforeach(i)
  set(SRCS ${__temp_srclist})
endmacro()
