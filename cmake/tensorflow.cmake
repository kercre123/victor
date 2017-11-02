set(TENSORFLOW_DIR ${CORETECH_EXTERNAL_DIR}/../../../coretech-external-local/tensorflow)

# For enabling Ahead-of-Time (AOT) compilation 
add_definitions(
  -DTENSORFLOW_USE_AOT=0
)

if (ANDROID)
  set(TENSORFLOW_INCLUDE_PATH ${TENSORFLOW_DIR}/include/android)
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/android)
  
  # TensorFlow uses a factory pattern that requires we forcibly include 
  # everything from the the library
  set(WHOLE_ARCHIVE_FLAG "-Wl,--whole-archive")
  set(NO_WHOLE_ARCHIVE_FLAG "-Wl,--no-whole-archive")

  # Force the linker to use the static protobuf we are specifying here, 
  # not the dynamic one available in the system libs, which is the
  # wrong version
  set(STATIC_FLAG "-Wl,-Bstatic")
  set(NO_STATIC_FLAG "-Wl,-Bdynamic")
else()
  set(TENSORFLOW_INCLUDE_PATH ${TENSORFLOW_DIR}/include/mac) 
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/mac)
  
  # TensorFlow uses a factory pattern that requires we forcibly include 
  # everything from the the library
  set(WHOLE_ARCHIVE_FLAG "-Wl,-force_load,")
  set(NO_WHOLE_ARCHIVE_FLAG "")

  set(STATIC_FLAG "")
  set(NO_STATIC_FLAG "")
endif()

set(include_paths
    ${TENSORFLOW_INCLUDE_PATH}/protobuf
    ${TENSORFLOW_INCLUDE_PATH}/tensorflow
    ${TENSORFLOW_INCLUDE_PATH}/proto)

set(TENSORFLOW_LIBS
    ${WHOLE_ARCHIVE_FLAG}
    libtensorflow-core
    ${NO_WHOLE_ARCHIVE_FLAG}
    ${STATIC_FLAG}
    libprotobuf
    ${NO_STATIC_FLAG})

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


