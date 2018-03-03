#
# Declare compile definitions shared by all Anki C++ projects
#

set(ANKI_BUILD_CXX_COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

