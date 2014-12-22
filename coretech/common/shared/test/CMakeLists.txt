# coretech/common/shared/test/CMakeLists.txt

file(GLOB SOURCE_FILES *.cpp ../src/*.cpp)

file(GLOB HEADER_FILES 
	*.h 
	../src/*.h 
	${CORETECH_COMMON_INCLUDE_DIR}/shared/*.h
)


# CoreTech_Common Shared Tests
add_executable(run_coreTechCommonSharedTests 
	run_coreTechCommonSharedTests.cpp 
	testVelocityProfileGenerator.cpp
	${SOURCE_FILES}
	${HEADER_FILES}
)
	
target_link_libraries(run_coreTechCommonSharedTests gtest)

if(${ANKICORETECH_EMBEDDED_USE_MATLAB} OR ${ANKICORETECH_USE_MATLAB})
  target_link_libraries(run_coreTechCommonSharedTests
    ${MATLAB_ENG_LIBRARY}
    ${MATLAB_MX_LIBRARY}
    ${MATLAB_MEX_LIBRARY}
  )
endif()

set_target_properties(run_coreTechCommonSharedTests PROPERTIES
  FOLDER "Unit Tests"
)


