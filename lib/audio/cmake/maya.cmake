set(MAYA_LIBS
  OpenMaya
  OpenMayaAnim
  OpenMayaRender
  OpenMayaUI
  Foundation
)

set(MAYA_LOCATION /Applications/Autodesk/maya2018)
set(MAYA_INCLUDE_PATH ${MAYA_LOCATION}/include)
set(MAYA_LIB_PATH ${MAYA_LOCATION}/Maya.app/Contents/MacOS)

foreach(LIB ${MAYA_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${MAYA_LIB_PATH}/lib${LIB}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES
    "${MAYA_INCLUDE_PATH}")
endforeach()

