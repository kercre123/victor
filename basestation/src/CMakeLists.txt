# products-cozmo/basestation/src/CMakeLists.txt

file(GLOB_RECURSE SOURCE_FILES
    anki/*.cpp)
file(GLOB_RECURSE PRIVATE_HEADER_FILES
    anki/*.h
    anki/*.def)
file(GLOB_RECURSE PUBLIC_HEADER_FILES
    ../include/*.h
    ../include/*.def)

# BLEManager subdirectory (not on iOS)
file(GLOB_RECURSE SOURCE_FILES_BLEMGR "BLEManager/*.m*")
file(GLOB_RECURSE HEADER_FILES_BLEMGR "BLEManager/*.h")
#if(NOT ANKI_IOS_BUILD)
#  set(SOURCE_FILES ${SOURCE_FILES} ${SOURCE_FILES_BLEMGR})
#  set(PRIVATE_HEADER_FILES ${PRIVATE_HEADER_FILES} ${HEADER_FILES_BLEMGR})
#endif(ANKI_IOS_BUILD)

# Enable ARC
#add_compile_options(-fobjc-arc)

include_directories(.)

####################
# Begin FacioMetric Stuff

file(GLOB RESOURCE_FILES
  ${FACIO_METRIC_DIR}/CommonFiles/ResourceFiles/*.bin
  ${FACIO_METRIC_DIR}/Anki/ResourceFiles/*.gns
)

set_source_files_properties(${RESOURCE_FILES}
  PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} 
  -finalize 
  -prefinalized-library 
  ${FACIO_METRIC_DIR}/Anki/Library/liblibintraface_core122.a 
  -prefinalized-library 
  ${FACIO_METRIC_DIR}/Anki/Library/liblibintraface_license_anki.a 
  -prefinalized-library 
  ${FACIO_METRIC_DIR}/Anki/Library/liblibintraface_emo122.a 
  -finalized-product 
  "${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PATH}""
)

# End FacioMetric Stuff
####################

add_library(Cozmo_Basestation
  ${SOURCE_FILES} 
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
  ${RESOURCE_FILES}
)

if(NOT ANKI_IOS_BUILD)
  # Grouping source files
  source_group(BLEManager FILES ${SOURCE_FILES_BLEMGR} ${HEADER_FILES_BLEMGR})
endif()

target_link_libraries(Cozmo_Basestation 
  CoreTech_Common_Basestation 
  CoreTech_Messaging_Basestation 
  CoreTech_Vision_Basestation 
  CoreTech_Planning_Basestation
  CoreTech_Vision_Robot  # TODO: Remove once we do basestation implementation of vision
  CoreTech_Common_Robot  # TODO: Remove once we do basestation implementation of vision
  
  Clad_Support_Cpp
  util
  ${ZLIB_LIBRARY}

)

add_dependencies(Cozmo_Basestation
  Cozmo_Engine_Clad)
