set(OPENCV_VERSION 3.1.0)

if(NOT USE_TENSORFLOW AND NOT USE_TENSORFLOW_LITE)
  set(OPENCV_VERSION dev) # the dnn module exists in OpenCV 3.3.0, mobile net arch supported in latest master (dev) 
endif()

set(OPENCV_DIR opencv-${OPENCV_VERSION})                                                                        

if (ANDROID)
  #set(OPENCV_DIR opencv-android)
  set(OPENCV_3RDPARTY_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/opencv-android-${OPENCV_VERSION}/o4a/3rdparty/lib/armeabi-v7a)
  set(OPENCV_LIB_DIR ${CORETECH_EXTERNAL_DIR}/build/opencv-android-${OPENCV_VERSION}/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a)
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
    videoio)

if(NOT USE_TENSORFLOW AND NOT USE_TENSORFLOW_LITE)
  list(APPEND OPENCV_LIBS dnn)
endif()

set(OPENCV_INCLUDE_PATHS ${OPENCV2_INCLUDE_PATH})

set(LIB_EXT a)
set(LIB_TYPE STATIC)
if(ANDROID AND OPENCV_VERSION EQUAL "3.1.0")
  set(LIB_EXT so)
  set(LIB_TYPE SHARED)
endif()

# Add the include directory for each OpenCV module:
foreach(OPENCV_MODULE ${OPENCV_LIBS})
  add_library(${OPENCV_MODULE} ${LIB_TYPE} IMPORTED)
  set(MODULE_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/${OPENCV_DIR}/modules/${OPENCV_MODULE}/include")
  set(include_paths
        ${MODULE_INCLUDE_PATH}
        ${OPENCV2_INCLUDE_PATH})

  set_target_properties(${OPENCV_MODULE} PROPERTIES
        IMPORTED_LOCATION
        ${OPENCV_LIB_DIR}/libopencv_${OPENCV_MODULE}.${LIB_EXT}
        INTERFACE_INCLUDE_DIRECTORIES
        "${include_paths}")

  list(APPEND OPENCV_INCLUDE_PATHS ${MODULE_INCLUDE_PATH})
endforeach()

add_library(opencv_interface INTERFACE IMPORTED)
set_target_properties(opencv_interface PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES
    "${OPENCV_INCLUDE_PATHS}"
)

if (ANDROID)
  set(OPENCV_EXTERNAL_LIBS     
      libpng
      libtiff
  )
else()
  set(OPENCV_EXTERNAL_LIBS
      IlmImf
      libjasper
      libpng
      libtiff
      zlib
  )
endif()

# 3.3.0 and "dev" versions have additional external libs
if(NOT OPENCV_VERSION EQUAL "3.1.0")
  if (ANDROID)
    list(APPEND OPENCV_EXTERNAL_LIBS cpufeatures libjpeg libprotobuf)
  else()
    list(APPEND OPENCV_EXTERNAL_LIBS libjpeg ippicv ippiw ittnotify libwebp libprotobuf)
  endif()
endif()

foreach(LIB ${OPENCV_EXTERNAL_LIBS})
    add_library(${LIB} STATIC IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        ${OPENCV_3RDPARTY_LIB_DIR}/lib${LIB}.a)
endforeach()

message(STATUS "including OpenCV: v${OPENCV_VERSION}, [Modules: ${OPENCV_LIBS}], [3rdParty: ${OPENCV_EXTERNAL_LIBS}]")

list(APPEND OPENCV_LIBS ${OPENCV_EXTERNAL_LIBS})

# We used jpeg-turbo with 3.1.0
if(OPENCV_VERSION EQUAL "3.1.0")
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
endif()


if (MACOSX)
  # Add Frameworks
  find_library(ACCELERATE Accelerate)
  find_library(APPKIT AppKit)
  find_library(OPENCL OpenCL)                                                               
  list(APPEND OPENCV_LIBS ${ACCELERATE} ${APPKIT} ${OPENCL})
endif()

# On Android, with opencv-3.1, we need to copy shared libs to our library output folder
macro(copy_opencv_android_libs)
if (ANDROID)
    if (TARGET copy_opencv_libs)
        return()
    endif()
    if(OPENCV_VERSION EQUAL "3.1.0")
      add_library(tbb SHARED IMPORTED)
      set(include_paths
          ${CORETECH_EXTERNAL_DIR}/build/opencv-android-${OPENCV_VERSION}/OpenCV-android-sdk/sdk/native/jni/include/opencv2/${OPENCV_MODULE}
          ${OPENCV_INCLUDE_PREFIX}
          ${OPENCV2_INCLUDE_PATH})
      set_target_properties(tbb PROPERTIES
          IMPORTED_LOCATION
          ${CORETECH_EXTERNAL_DIR}/build/opencv-android-${OPENCV_VERSION}/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a/libtbb.so)
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
    endif()
    add_custom_target(copy_opencv_libs ALL DEPENDS ${OUTPUT_FILES})
endif()
endmacro()

