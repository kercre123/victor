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


FILE(GLOB SOURCE_PRE_ROOT "GeneratedMessages.c")
FILE(GLOB HEADER_PRE_ROOT "MessageDefMacros_CSharp.h")

set(SOURCE_PRE
  ${SOURCE_PRE_ROOT}
)

set(PRIVATE_HEADER_PRE)
foreach(INCLUDE ${HEADER_PRE_ROOT})
  set(PRIVATE_HEADER_PRE ${PRIVATE_HEADER_PRE} -I${INCLUDE})
endforeach()

set(PRIVATE_HEADER_PRE
  ${HEADER_PRE_ROOT}
)

get_property(INCLUDE_PRE
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY INCLUDE_DIRECTORIES
)

set(PUBLIC_HEADER_PRE)
foreach(INCLUDE ${INCLUDE_PRE})
  set(PUBLIC_HEADER_PRE ${PUBLIC_HEADER_PRE} -I${INCLUDE})
endforeach()

set(OUTPUT_PRE GeneratedMessages.cs)
set(OUTPUT_DEST ${CMAKE_CURRENT_SOURCE_DIR}/../Cozmo/Assets/Scripts/Channel/)

add_custom_command(
  OUTPUT Fake.xxx
  COMMAND "${CMAKE_C_COMPILER}" -E -P ${SOURCE_PRE} ${PUBLIC_HEADER_PRE} ${PRIVATE_HEADER_PRE} -o ${OUTPUT_PRE}
  COMMAND diff ${OUTPUT_PRE} ${OUTPUT_DEST} || cp ${OUTPUT_PRE} ${OUTPUT_DEST}
  MAIN_DEPENDENCY ${SOURCE_PRE}
  DEPENDS ${HEADER_PRE_ROOT}
  VERBATIM
)

add_custom_target(
  CSharpGeneratedMessages ALL
  DEPENDS Fake.xxx
  VERBATIM
)
