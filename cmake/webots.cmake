set(WEBOTS_HOME "/Applications/Webots.app")

set(WEBOTS_INCLUDE_PATHS
   "${WEBOTS_HOME}/include"
   "${WEBOTS_HOME}/include/ode"
   "${WEBOTS_HOME}/include/controller/cpp"
)

if (MACOSX)
    set(WEBOTS_LIB_PATH "${WEBOTS_HOME}/lib")
endif()

set(WEBOTS_LIBS
    Controller
    CppController
)

set(WEBOTS_SIM_LIBS
    ode
)

set(WEBOTS_LIB_TARGETS
    "${WEBOTS_LIBS}"
    "${WEBOTS_SIM_LIBS}"
)

foreach(LIB ${WEBOTS_LIB_TARGETS})
    if (NOT TARGET ${LIB})
        add_library(${LIB} SHARED IMPORTED)
        set_target_properties(${LIB} PROPERTIES
            IMPORTED_LOCATION
            "${WEBOTS_LIB_PATH}/lib${LIB}.dylib"
            INTERFACE_INCLUDE_DIRECTORIES
            "${WEBOTS_INCLUDE_PATHS}")
    endif()
endforeach()


#
# webots plugins lib targets
#

if (NOT TARGET webots_plugin_physics)
    add_library(webots_plugin_physics
        ${WEBOTS_HOME}/resources/projects/plugins/physics/physics.c
    )

    target_include_directories(webots_plugin_physics
    PUBLIC
        $<BUILD_INTERFACE:${WEBOTS_HOME}/include>
        $<BUILD_INTERFACE:${WEBOTS_HOME}/include/ode>
    )

    anki_build_target_license(webots_plugin_physics "ANKI")
endif()
list(APPEND WEBOTS_PLUGINS webots_plugin_physics)
