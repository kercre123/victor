# products-cozmo/unity/CSharpBinding/CMakeLists.txt

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

FILE(GLOB PRIVATE_FILES
  "src/*.cpp"
  "src/*.h")
FILE(GLOB IOS_FILES
    "src/ios/*.cpp"
    "src/ios/*.h")

if(ANKI_IOS_BUILD)
  add_library(CSharpBinding STATIC
    ${PRIVATE_FILES} 
    ${IOS_FILES}
  )
else(ANKI_IOS_BUILD)
  add_library(CSharpBinding SHARED
    ${PRIVATE_FILES} 
  )
endif(ANKI_IOS_BUILD)

target_link_libraries(CSharpBinding
  Cozmo_Basestation 
  CoreTech_Messaging_Basestation
  CoreTech_Common_Basestation 
)
