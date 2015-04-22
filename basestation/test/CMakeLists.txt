# products-cozmo/test/CMakeLists.txt

file(GLOB TEST_SRC_FILES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR}/basestation/src/anki/cozmo/basestation)

add_executable(run_CozmoTests ${TEST_SRC_FILES})
  
target_link_libraries(run_CozmoTests 
   Cozmo_Basestation 
   CoreTech_Common_Basestation 
   CoreTech_Vision_Basestation 
   CoreTech_Messaging_Basestation
   ${OPENCV_LIBS} gtest jsoncpp)

if(${ANKICORETECH_USE_MATLAB})
  target_link_libraries(run_CozmoTests
    ${MATLAB_ENG_LIBRARY}
    ${MATLAB_MX_LIBRARY}
  )
endif()

set_target_properties(run_CozmoTests PROPERTIES
  FOLDER "Unit Tests"
)
