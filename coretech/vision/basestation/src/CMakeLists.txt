# coretech/vision/basestation/src/CMakeLists.txt

# Vision Library


file(GLOB SOURCE_FILES "*.cpp")

file(GLOB HEADER_FILES "*.h")

add_library(CoreTech_Vision_Basestation 
  ${SOURCE_FILES} 
  ${HEADER_FILES}
)

#target_link_libraries(CoreTech_Vision)


