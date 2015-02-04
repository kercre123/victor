# products-cozmo/unity/CSharpBinding/CMakeLists.txt

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

# root src dir
file(GLOB SOURCE_FILES_ROOT "*.cpp")
file(GLOB HEADER_FILES_ROOT "*.h")

set(PRIVATE_HEADER_FILES
  ${HEADER_FILES_ROOT}
)

set(SOURCE_FILES
  ${SOURCE_FILES_ROOT}
)

# Enable ARC
add_compile_options(-fobjc-arc)
set(CMAKE_CXX_FLAGS “${CMAKE_CXX_FLAGS} -v -stdlib=libstdc++”)

set(PUBLIC_HEADER_FILES
  ${PUBLIC_HEADER_FILES_ROOT}
)

add_library(CSharpBinding
  ${SOURCE_FILES} 
  ${PRIVATE_HEADER_FILES}
  ${PUBLIC_HEADER_FILES}
)

target_link_libraries(CSharpBinding
  Cozmo_Basestation 
  CoreTech_Messaging_Basestation
  CoreTech_Common_Basestation 
)

