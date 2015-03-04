# products-cozmo/unity/CSharpBinding/CMakeLists.txt

add_definitions(-DCOZMO_BASESTATION)
add_definitions(-DCORETECH_BASESTATION)

if(ANKI_IOS_BUILD)
  # root src dir
  FILE(GLOB SOURCE_FILES_ROOT "csharp-binding.cpp")
  FILE(GLOB HEADER_FILES_ROOT "csharp-binding.h")

  set(PRIVATE_HEADER_FILES
    ${HEADER_FILES_ROOT}
  )

  set(SOURCE_FILES
    ${SOURCE_FILES_ROOT}
  )

  # Enable ARC
  #add_compile_options(-fobjc-arc)

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
endif(ANKI_IOS_BUILD)
