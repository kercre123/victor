
#set(LUXAND_DIRECTORY "~/Documents/libraries/Luxand_FaceSDK")

set(LUXAND_DIRECTORY ${CORETECH_EXTERNAL_DIR}/Luxand_FaceSDK)
#set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS ${CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS} ${LUXAND_DIRECTORY}/bin/osx_x86_64/)

INCLUDE_DIRECTORIES(${LUXAND_DIRECTORY}/include/C)

add_library(libRecognizeFaces
  recognizeFaces.h
  recognizeFaces.cpp  
)

add_executable(run_recognizeFaces
  recognizeFaces.h
  run_recognizeFaces.cpp 
)

add_executable(run_recognizeFacesOffline
  recognizeFaces.h
  run_recognizeFacesOffline.cpp 
)

target_link_libraries(run_recognizeFaces
  ${ALL_EMBEDDED_LIBRARIES}
  CoreTech_Common_Robot 
  CoreTech_Vision_Robot
  libRecognizeFaces
  ${LUXAND_DIRECTORY}/bin/osx_x86_64/libfsdk.dylib
)

target_link_libraries(run_recognizeFacesOffline
  ${ALL_EMBEDDED_LIBRARIES}
  CoreTech_Common_Robot 
  CoreTech_Vision_Robot
  libRecognizeFaces
  ${LUXAND_DIRECTORY}/bin/osx_x86_64/libfsdk.dylib
)

set_target_properties(libRecognizeFaces PROPERTIES
  FOLDER "Tools"
)

set_target_properties(run_recognizeFaces PROPERTIES
  FOLDER "Tools"
)

set_target_properties(run_recognizeFacesOffline PROPERTIES
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
#build_mex(mexrecognizeFaces.cpp)
