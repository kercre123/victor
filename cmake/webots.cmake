set(WEBOTS_INCLUDE_PATHS
   "/Applications/Webots.app/include"
   "/Applications/Webots.app/include/ode"
   "/Applications/Webots.app/include/controller/cpp"
)

if (MACOSX)
    set(WEBOTS_LIB_PATH "/Applications/Webots.app/lib")
endif()

set(WEBOTS_LIBS
    Controller
    CppController
)

foreach(LIB ${WEBOTS_LIBS})
    add_library(${LIB} SHARED IMPORTED)
    set_target_properties(${LIB} PROPERTIES
        IMPORTED_LOCATION
        "${WEBOTS_LIB_PATH}/lib${LIB}.dylib"
        INTERFACE_INCLUDE_DIRECTORIES
        "${WEBOTS_INCLUDE_PATHS}")

endforeach()
