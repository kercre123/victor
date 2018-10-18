#
# platform/config/CMakeLists.txt
#
# cmake rules to determine which files get deployed to robot under /anki/{bin,etc}
#
# Build options
#
# Configure with -DANKI_RESOURCE_SHIPPING=1 to deploy shipping config files.
# Configure with -DANKI_RESOURCE_SHIPPING=0 to deploy development config files.
# Default is ANKI_RESOURCE_SHIPPING=0.
#
# Configure with -DANKI_USER_DEV=1 to deploy the userdev version of config files
# Default is ANKI_USER_DEV=0
#
# Configure with -DANKI_BETA=1 to deploy the beta version of config files
# Default is ANKI_BETA=0
#
# Note use of add_dependencies() to serialize copy operations. This is done to prevent random build failures
# that occur when parallel processes attempt to create the same directory structure.
#
include(anki_build_copy_assets)

anki_build_copy_assets(
    TARGET platform_config_bin
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}"
)

anki_build_copy_assets(
    TARGET platform_config_etc
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}"
)

#
# platform_config_etc_development, platform_config_etc_shipping
# platform_config_bin_development, (none for shipping)
# Choose extra config files from shipping or development,
# then copy them into etc
#
if (ANKI_RESOURCE_SHIPPING)
  set(platform_config_etc_extra platform_config_etc_shipping)
else()
  set(platform_config_etc_extra platform_config_etc_development)
  set(platform_config_bin_extra platform_config_bin_development)
endif()

# If this is the special "userdev" build then use its etc files
if (ANKI_USER_DEV)
  set(platform_config_etc_extra platform_config_etc_userdev)
endif()

# If this is the special "beta" build then use its etc files
if (ANKI_BETA)
  set(platform_config_etc_extra platform_config_etc_beta)
endif()

anki_build_copy_assets(
  TARGET ${platform_config_etc_extra}
  SRCLIST_DIR ${ANKI_SRCLIST_DIR}
  OUTPUT_DIR "${CMAKE_BINARY_DIR}"
)

if (NOT ANKI_RESOURCE_SHIPPING)
  anki_build_copy_assets(
    TARGET ${platform_config_bin_extra}
    SRCLIST_DIR ${ANKI_SRCLIST_DIR}
    OUTPUT_DIR "${CMAKE_BINARY_DIR}"
  )
endif()

add_dependencies(platform_config_etc ${platform_config_etc_extra})
if (NOT ANKI_RESOURCE_SHIPPING)
  add_dependencies(platform_config_bin ${platform_config_bin_extra})
endif()
