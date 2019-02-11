if (VICOS)
  file(GLOB GPIO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
  file(GLOB GPIO_INCS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

  add_library(gpio STATIC
    ${GPIO_SRCS}
    ${GPIO_INCS}
  )

  target_include_directories(gpio 
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  )

  set_target_properties(gpio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}"
  )

  set_target_properties(gpio PROPERTIES LINKER_LANGUAGE C)

  anki_build_strip(TARGET gpio)

endif()

