# Required CMake Version
cmake_minimum_required(VERSION 2.8.3)

# Package name
project(royale_in_ros)

set(royale_LIBRARIES royale)

if(APPLE)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wall -Wsign-compare -Wuninitialized -Wunused -Wno-deprecated-declarations -std=c++11" CACHE STRING "" FORCE)
   SET(CMAKE_MACOSX_RPATH TRUE CACHE BOOL "")
   SET(CMAKE_BUILD_WITH_INSTALL_RPATH OFF CACHE BOOL "")
   SET(CMAKE_INSTALL_RPATH "@loader_path" CACHE STRING "")
elseif(UNIX)
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++0x -Wall -Wconversion" CACHE STRING "" FORCE)
   SET(CMAKE_BUILD_WITH_INSTALL_RPATH OFF CACHE BOOL "")
   SET(CMAKE_INSTALL_RPATH "\$ORIGIN/" CACHE STRING "")
endif()

# Find packages needed for build
find_package(catkin REQUIRED COMPONENTS
  rviz
  nodelet
  roscpp
  rostime
  sensor_msgs
  std_msgs
)

catkin_package(
)

# Include Paths and Library Paths 
include_directories(SYSTEM
  ${catkin_INCLUDE_DIRS}
)

link_directories(${catkin_LIBRARY_DIRS} "${CMAKE_CURRENT_LIST_DIR}/royale/bin")

# This setting causes Qt's "MOC" generation to happen automatically.
set(CMAKE_AUTOMOC ON)

# Include Qt
if(rviz_QT_VERSION VERSION_LESS "5")
  message(STATUS "Using Qt4 based on the rviz_QT_VERSION: ${rviz_QT_VERSION}")
  find_package(Qt4 ${rviz_QT_VERSION} EXACT REQUIRED QtCore QtGui)
  include(${QT_USE_FILE})
else()
  message(STATUS "Using Qt5 based on the rviz_QT_VERSION: ${rviz_QT_VERSION}")
  find_package(Qt5 ${rviz_QT_VERSION} EXACT REQUIRED Core Widgets)
  set(QT_LIBRARIES Qt5::Widgets)
endif()

# Define QT_NO_KEYWORDS
add_definitions(-DQT_NO_KEYWORDS)

include_directories(
  "${CMAKE_CURRENT_LIST_DIR}/inc"
  "${CMAKE_CURRENT_LIST_DIR}/royale/include"
)

set(SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/RoyaleInRos.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/RoyaleControl.cpp"
  )
set(HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/inc/RoyaleInRos.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/inc/RoyaleControl.hpp"
  )

add_library(royale_nodelet SHARED ${SOURCES} ${HEADERS})
target_link_libraries(royale_nodelet
  ${catkin_LIBRARIES}
  ${QT_LIBRARIES}
  royale
)

add_executable(royale_in_ros ${SOURCES} 
                             ${HEADERS}
                             "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
              )
target_link_libraries(royale_in_ros
  ${catkin_LIBRARIES}
  ${QT_LIBRARIES}
  royale
)

install(TARGETS royale_in_ros royale_nodelet
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
install(DIRECTORY
  launch
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
install(FILES
  nodelets.xml
  plugins.xml
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
