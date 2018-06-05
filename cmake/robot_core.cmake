if (VICOS)
  file(GLOB ROBOT_CORE_SRCS ${CMAKE_SOURCE_DIR}/robot/core/src/*.c)
  file(GLOB ROBOT_CORE_INCS ${CMAKE_SOURCE_DIR}/robot/core/inc/*.h)

  set(ROBOT_CORE_LIBS
    robot_core
  )

  foreach(LIB ${ROBOT_CORE_LIBS})
    add_library(${LIB} STATIC
      ${ROBOT_CORE_SRCS}
      ${ROBOT_CORE_INCS}
    )

    target_include_directories(${LIB} 
      PRIVATE
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/robot/core/inc>
    )

    set_target_properties(${LIB} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES
      "${CMAKE_SOURCE_DIR}/robot/core/inc"
    )
    anki_build_target_license(${LIB} "ANKI")

  endforeach()
endif()
