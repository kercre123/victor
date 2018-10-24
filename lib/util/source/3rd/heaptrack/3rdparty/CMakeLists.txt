if (ECM_FOUND)
    include(ECMEnableSanitizers)
endif()

if (NOT MACOSX AND NOT VICOS)
add_subdirectory(libbacktrace)
endif()

if (ZSTD_FOUND)
add_subdirectory(boost-zstd)
endif()
