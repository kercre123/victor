set(SOURCE_FILES_SHOW_VOXELS
  showVoxels.cpp
)

add_executable(showVoxels ${SOURCE_FILES_SHOW_VOXELS})

INCLUDE_DIRECTORIES ( /System/Library/Frameworks )

FIND_LIBRARY(COCOA_LIBRARY Cocoa)
FIND_LIBRARY(GLUT_LIBRARY GLUT )
FIND_LIBRARY(OpenGL_LIBRARY OpenGL )

MARK_AS_ADVANCED (COCOA_LIBRARY
                 GLUT_LIBRARY
                 OpenGL_LIBRARY)
                 
SET(EXTRA_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})

target_link_libraries(showVoxels
  CoreTech_Vision_Robot
  CoreTech_Common_Robot
  ${ALL_EMBEDDED_LIBRARIES}
  SDL2-2.0
  ${EXTRA_LIBS}
)

set_target_properties(showVoxels PROPERTIES
  FOLDER "Tools"
)