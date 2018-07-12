#
# anki_build_data_list.cmake
#
# Helper macros to integrate cmake + metabuild.py
#
macro(anki_build_data_list target_name srclist_dir)

set(DATA_LIST_FILE "${srclist_dir}/${target_name}.data.lst")

if (EXISTS ${DATA_LIST_FILE})
  file(STRINGS ${DATA_LIST_FILE} DATA)
else()
  message(FATAL_ERROR ${DATA_LIST_FILE} " not found. Did you forget to add your BUILD.in to build-victor.sh?")
endif()

set(_ab_PLATFORM_DATA "")

if (ANDROID)
  set(_ab_datafile "${srclist_dir}/${target_name}_android.data.lst")
elseif (MACOSX)
  set(_ab_datafile "${srclist_dir}/${target_name}_mac.data.lst")
elseif (VICOS)
  set(_ab_datafile "${srclist_dir}/${target_name}_vicos.data.lst")
else()
  set(_ab_datafile "")
endif()

if (EXISTS ${_ab_datafile})
  file(STRINGS ${_ab_datafile} _ab_PLATFORM_DATA)
endif()

endmacro()

macro(anki_build_absolute_data_list target_name srclist_dir)
  anki_build_data_list(${target_name} ${srclist_dir})
  # transform data files into absolute paths
  set(__temp_datalist "")
  foreach(i ${DATA})
    set(__temp_datai "${CMAKE_CURRENT_SOURCE_DIR}/${i}")
  list(APPEND __temp_datalist ${__temp_datai})
  endforeach(i)
  set(DATA ${__temp_datalist})
endmacro()
