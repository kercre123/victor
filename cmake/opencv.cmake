set(OPENCV_VERSION 3.3.0)

set(OPENCV_DIR opencv-${OPENCV_VERSION})                                                                        

if (ANDROID)
  #set(OPENCV_DIR opencv-android)
  set(OPENCV_3RDPARTY_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/opencv-android/o4a/3rdparty/lib/armeabi-v7a)
  set(OPENCV_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a)
  set(OPENCV2_INCLUDE_PATH ${CORETECH_EXTERNAL_DIR}/build/opencv-android) 
else()
  #set(OPENCV_DIR opencv-${OPENCV_VERSION})                                                                        
  set(OPENCV_3RDPARTY_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/${OPENCV_DIR}/3rdparty/lib/Release)
  set(OPENCV_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/${OPENCV_DIR}/lib/Release)
  set(OPENCV2_INCLUDE_PATH ${CORETECH_EXTERNAL_DIR}/build/opencv-${OPENCV_VERSION})
endif()

                                                                                                    
set(OPENCV_LIBS
    calib3d
    features2d
    imgproc 
    core
    highgui
    objdetect
    video
    flann
    imgcodecs
    ml
    videoio
    dnn)

set(OPENCV_MODULES_DIR ${CORETECH_EXTERNAL_DIR}/OPENCV_DIR/modules)

set(OPENCV_INCLUDE_PATHS ${OPENCV2_INCLUDE_PATH})

# Add the include directory for each OpenCV module:
foreach(OPENCV_MODULE ${OPENCV_LIBS})
  message(STATUS "opencv.cmake add_library: " ${OPENCV_MODULE})
  add_library(${OPENCV_MODULE} STATIC IMPORTED)
  set(MODULE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/modules/${OPENCV_MODULE}/include")
  set(include_paths
        ${MODULE_INCLUDE_PATH}
        ${OPENCV2_INCLUDE_PATH})

  set_target_properties(${OPENCV_MODULE} PROPERTIES
        IMPORTED_LOCATION
        ${OPENCV_LIB_DIR}/libopencv_${OPENCV_MODULE}.a
        INTERFACE_INCLUDE_DIRECTORIES
        "${include_paths}")

  list(APPEND OPENCV_INCLUDE_PATHS ${MODULE_INCLUDE_PATH})
endforeach()

# TODO: Remove this 
if(0)
    if (ANDROID)
      message(STATUS "opencv.cmake add_library: " ${OPENCV_MODULE})
      add_library(${OPENCV_MODULE} STATIC IMPORTED)
      set(MODULE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/jni/include/opencv2/${OPENCV_MODULE}")
      set(include_paths
          ${MODULE_INCLUDE_PATH}
          ${OPENCV_INCLUDE_PREFIX}
          ${OPENCV2_INCLUDE_PATH})
      set_target_properties(${OPENCV_MODULE} PROPERTIES
          IMPORTED_LOCATION
          ${CORETECH_EXTERNAL_DIR}/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a/libopencv_${OPENCV_MODULE}.a
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
endif()

add_library(opencv_interface INTERFACE IMPORTED)
set_target_properties(opencv_interface PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES
    "${OPENCV_INCLUDE_PATHS}"
)

if (ANDROID)
  set(OPENCV_EXTERNAL_LIBS     
      cpufeatures
      libjpeg
      libpng
      libtiff
      libprotobuf
  )
else()
  set(OPENCV_EXTERNAL_LIBS
      IlmImf
      libjasper
      libjpeg
      libpng
      libtiff
      zlib
      libprotobuf
      libwebp
      ippicv
      ipp_iw
      ittnotify
  )
endif()

foreach(LIB ${OPENCV_EXTERNAL_LIBS})
    add_library(${LIB} STATIC IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        ${OPENCV_3RDPARTY_LIB_DIR}/lib${LIB}.a)
endforeach()

list(APPEND OPENCV_LIBS ${OPENCV_EXTERNAL_LIBS})

if (MACOSX)
  # Add Frameworks
  find_library(ACCELERATE Accelerate)
  find_library(APPKIT AppKit)
  find_library(OPENCL OpenCL)                                                               
  list(APPEND OPENCV_LIBS ${ACCELERATE} ${APPKIT} ${OPENCL})
endif()

#if (ANDROID)
#  add_library(libjpeg STATIC IMPORTED)
#  set_target_properties(libjpeg PROPERTIES IMPORTED_LOCATION
#    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/android_armv7_libs/libjpeg.so)
#  add_library(libturbojpeg STATIC IMPORTED)
#  set_target_properties(libturbojpeg PROPERTIES IMPORTED_LOCATION
#    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/android_armv7_libs/libturbojpeg.so)
#elseif (MACOSX)
#  add_library(libjpeg STATIC IMPORTED)
#  set_target_properties(libjpeg PROPERTIES IMPORTED_LOCATION
#    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/mac_libs/libjpeg.a)
#  add_library(libturbojpeg STATIC IMPORTED)
#  set_target_properties(libturbojpeg PROPERTIES IMPORTED_LOCATION
#    ${CORETECH_EXTERNAL_DIR}/libjpeg-turbo/mac_libs/libturbojpeg.a)
#  list(APPEND OPENCV_LIBS libturbojpeg)
#endif()

