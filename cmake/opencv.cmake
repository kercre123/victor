if (ANDROID)
  set(OPENCV_DIR opencv-android)
else()
  set(OPENCV_DIR opencv-3.1.0)                                                                        
endif()

                                                                                                    
set(OPENCV_LIBS
    core
    highgui
    imgproc
    calib3d
    objdetect
    video
    features2d
    flann
    imgcodecs
    ml
    videoio)

set(OPENCV_MODULES_DIR ${CORETECH_EXTERNAL_DIR}/opencv-3.1.0/modules)
set(EXTERNAL_BUILD_DIR ${CORETECH_EXTERNAL_DIR}/build)

if (ANDROID)
  set(OPENCV_INCLUDE_DIR "")
endif()

if (ANDROID)
  set(OPENCV_INCLUDE_PREFIX "${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/jni/include")
else()
  set(OPENCV_INCLUDE_PREFIX "")
endif()

set(OPENCV2_INCLUDE_PATH "")
if (ANDROID)
  set(OPENCV2_INCLUDE_PATH ${OPENCV_INCLUDE_PREFIX}/opencv2) 
endif()

set(OPENCV_INCLUDE_PATHS ${OPENCV_INCLUDE_PREFIX} ${OPENCV2_INCLUDE_PATH})

# Add the include directory for each OpenCV module:
foreach(OPENCV_MODULE ${OPENCV_LIBS})
    if (ANDROID)
      message(STATUS "opencv.cmake add_library: " ${OPENCV_MODULE})
      add_library(${OPENCV_MODULE} SHARED IMPORTED)
      set(MODULE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/jni/include/opencv2/${OPENCV_MODULE}")
      set(include_paths
          ${MODULE_INCLUDE_PATH}
          ${OPENCV_INCLUDE_PREFIX}
          ${OPENCV2_INCLUDE_PATH})
      set_target_properties(${OPENCV_MODULE} PROPERTIES
          IMPORTED_LOCATION
          ${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a/libopencv_${OPENCV_MODULE}.so
          INTERFACE_INCLUDE_DIRECTORIES
          "${include_paths}")
      list(APPEND OPENCV_INCLUDE_PATHS ${MODULE_INCLUDE_PATH})
    elseif (MACOSX)
      message(STATUS "opencv.cmake add_library: " ${OPENCV_MODULE})
      add_library(${OPENCV_MODULE} STATIC IMPORTED)
      set(MODULE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/modules/${OPENCV_MODULE}/include")
      set(include_paths
          ${MODULE_INCLUDE_PATH}
          ${OPENCV_INCLUDE_PREFIX}
          ${OPENCV2_INCLUDE_PATH})
      set_target_properties(${OPENCV_MODULE} PROPERTIES
          IMPORTED_LOCATION
          ${CORETECH_EXTERNAL_DIR}/build/${OPENCV_DIR}/lib/Release/libopencv_${OPENCV_MODULE}.a
          INTERFACE_INCLUDE_DIRECTORIES
          "${include_paths}")
      list(APPEND OPENCV_INCLUDE_PATHS ${MODULE_INCLUDE_PATH})
    else()
      include_directories(${OPENCV_MODULES_DIR}/${OPENCV_MODULE}/include)
    endif()
endforeach()

add_library(opencv_interface INTERFACE IMPORTED)
set_target_properties(opencv_interface PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES
    "${OPENCV_INCLUDE_PATHS}"
)

set(OPENCV_EXTERNAL_LIBS
    IlmImf
    libjasper
    libpng
    libtiff
    zlib
)

if (MACOSX)
  foreach(LIB ${OPENCV_EXTERNAL_LIBS})
      add_library(${LIB} STATIC IMPORTED)
      set_target_properties(${LIB} PROPERTIES
          IMPORTED_LOCATION
          ${CORETECH_EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/lib/Release/lib${LIB}.a)
  endforeach()
  list(APPEND OPENCV_LIBS ${OPENCV_EXTERNAL_LIBS})

  find_library(APPKIT AppKit)
  list(APPEND OPENCV_LIBS ${APPKIT})
endif()

if (ANDROID)
  add_library(libjpeg SHARED IMPORTED)
  set_target_properties(libjpeg PROPERTIES IMPORTED_LOCATION
    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/android_armv7_libs/libjpeg.so)
  add_library(libturbojpeg SHARED IMPORTED)
  set_target_properties(libturbojpeg PROPERTIES IMPORTED_LOCATION
    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/android_armv7_libs/libturbojpeg.so)
elseif (MACOSX)
  add_library(libjpeg STATIC IMPORTED)
  set_target_properties(libjpeg PROPERTIES IMPORTED_LOCATION
    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/mac_libs/libjpeg.a)
  add_library(libturbojpeg STATIC IMPORTED)
  set_target_properties(libturbojpeg PROPERTIES IMPORTED_LOCATION
    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/mac_libs/libturbojpeg.a)
  list(APPEND OPENCV_LIBS libturbojpeg)
endif()

# On Android, we need to copy shared libs to our library output folder
macro(copy_opencv_android_libs)
if (ANDROID)
    if (TARGET copy_opencv_libs)
        return()
    endif()
    add_library(tbb SHARED IMPORTED)
    set(include_paths
        ${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/jni/include/opencv2/${OPENCV_MODULE}
        ${OPENCV_INCLUDE_PREFIX}
        ${OPENCV2_INCLUDE_PATH})
    set_target_properties(tbb PROPERTIES
        IMPORTED_LOCATION
        ${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a/libtbb.so)
    set(INSTALL_LIBS
        "${OPENCV_LIBS}"
        tbb
        libturbojpeg)
    message(STATUS "opencv libs: ${INSTALL_LIBS}")
    set(OUTPUT_FILES "")
    foreach(lib ${INSTALL_LIBS})
        get_target_property(LIB_PATH ${lib} IMPORTED_LOCATION)
        get_filename_component(LIB_FILENAME ${LIB_PATH} NAME)
        set(DST_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${LIB_FILENAME}") 
        # message(STATUS "copy opencv lib: ${lib} ${LIB_PATH} -> ${DST_PATH}")
        add_custom_command(
            OUTPUT "${DST_PATH}"
            COMMAND ${CMAKE_COMMAND}
            ARGS -E copy_if_different "${LIB_PATH}" "${DST_PATH}"
            COMMENT "copy ${LIB_PATH}"
            VERBATIM
        )
        list(APPEND OUTPUT_FILES ${DST_PATH})
    endforeach() 
    add_custom_target(copy_opencv_libs ALL DEPENDS ${OUTPUT_FILES})
endif()
endmacro()
