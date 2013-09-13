# products-cozmo/src/CMakeLists.txt

file(GLOB COZMO_FILES "*.cpp")

include_directories(${PROJECT_SOURCE_DIR}/../coretech-vision/include)

add_library(Cozmo ${COZMO_FILES})
