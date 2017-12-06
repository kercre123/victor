set(TENSORFLOW_DIR ${CORETECH_EXTERNAL_DIR}/../../../coretech-external-local/tensorflow-lite)

if (ANDROID)
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/android)
else()
  set(TENSORFLOW_LIB_PATH ${TENSORFLOW_DIR}/lib/mac)
endif()

set(TENSORFLOW_INCLUDE_PATHS
    ${TENSORFLOW_DIR}/include/tensorflow
    ${TENSORFLOW_DIR}/include/tensorflow-lite
    ${TENSORFLOW_DIR}/include/flatbuffers)

set(TENSORFLOW_LIBS libtensorflow-lite)

add_library(libtensorflow-lite STATIC IMPORTED)

set_target_properties(libtensorflow-lite PROPERTIES
      IMPORTED_LOCATION
      ${TENSORFLOW_LIB_PATH}/libtensorflow-lite.a
      INTERFACE_INCLUDE_DIRECTORIES
      "${TENSORFLOW_INCLUDE_PATH}")



