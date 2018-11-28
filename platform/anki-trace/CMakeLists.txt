if(USE_ANKITRACE AND VICOS)
  set(SRC
    anki_ust.c)

  add_library(ankitrace ${SRC})


  target_include_directories(ankitrace
    PUBLIC
    $<BUILD_INTERFACE:${LTTNGUST_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    )

  target_link_libraries(ankitrace
    LINK_PUBLIC
    dl
    ${LTTNGUST_LIBRARIES}
    )
endif()
