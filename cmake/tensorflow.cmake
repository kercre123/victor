set(TENSORFLOW_DIR ${CORETECH_EXTERNAL_DIR}/../../../coretech-external-local/tensorflow)

set(TENSORFLOW_INCLUDE_DIR ${TENSORFLOW_DIR}/include)

# For enabling Ahead-of-Time (AOT) compilation 
add_definitions(
  -DTENSORFLOW_USE_AOT=0
)

set(include_paths
    ${TENSORFLOW_INCLUDE_DIR}/protobuf
    ${TENSORFLOW_INCLUDE_DIR}/tensorflow)

if (ANDROID)
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/android)
  set(WHOLE_ARCHIVE_FLAG "-Wl,--whole-archive")
  set(NO_WHOLE_ARCHIVE_FLAG "-Wl,--no-whole-archive")
else()
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/mac)
  set(WHOLE_ARCHIVE_FLAG "-Wl,-force_load,")
  set(NO_WHOLE_ARCHIVE_FLAG "")
endif()

set(TENSORFLOW_LIBS
    ${WHOLE_ARCHIVE_FLAG}
    libtensorflow-core
    ${NO_WHOLE_ARCHIVE_FLAG}
    libprotobuf)

add_library(libtensorflow-core STATIC IMPORTED)

set_target_properties(libtensorflow-core PROPERTIES
      IMPORTED_LOCATION
      ${TENSORFLOW_LIB_PATH}/libtensorflow-core.a
      INTERFACE_INCLUDE_DIRECTORIES
      "${include_paths}")

add_library(libprotobuf STATIC IMPORTED)

set_target_properties(libprotobuf PROPERTIES
      IMPORTED_LOCATION
      ${TENSORFLOW_LIB_PATH}/libprotobuf.a
      INTERFACE_INCLUDE_DIRECTORIES
      "${include_paths}")

if (MACOSX)
  # Add Frameworks
  find_library(ACCELERATE Accelerate)
  find_library(APPKIT AppKit)
  find_library(OPENCL OpenCL)                                                               
  list(APPEND OPENCV_LIBS ${ACCELERATE} ${APPKIT} ${OPENCL})
endif()


