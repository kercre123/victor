add_library(libCaptureImages
  captureImages.h
  captureImages.cpp  
)

add_executable(run_captureImages
  captureImages.h
  run_captureImages.cpp 
)

target_link_libraries(run_captureImages
  ${ALL_EMBEDDED_LIBRARIES}
  libCaptureImages
)

set_target_properties(libCaptureImages PROPERTIES
  FOLDER "Tools"
)

set_target_properties(run_captureImages PROPERTIES
  FOLDER "Tools"
)

#set(MEX_LINK_LIBRARIES 
#  CoreTech_Common_Robot 
#  CoreTech_Vision_Robot 
#  ${ALL_EMBEDDED_LIBRARIES}
#  SDL2-2.0
#  ${EXTRA_LIBS}
#  libShowVoxels
#)
#
#build_mex(mexCaptureImages.cpp)