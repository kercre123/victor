#
# Declare compile definitions shared by all Anki C++ projects
#

set(ANKI_BUILD_CXX_COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Debug>:_LIBCPP_DEBUG=0>
  $<$<CONFIG:Release>:NDEBUG>
)

