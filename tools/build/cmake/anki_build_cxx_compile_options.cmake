#
# Declare compile options shared by all Anki C++ projects
#
# Warning flags can be suppressed for specific targets with
#   target_compile_options(name
#     -Wno-error=blah
#   )
# where needed.
#

set(ANKI_BUILD_CXX_COMPILE_OPTIONS
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-Os>
  -fsigned-char
  -g
  -Wall
  -Wconditional-uninitialized
  -Werror
  -Wformat
  -Wformat-security
  -Wheader-guard
  -Winit-self
  -Woverloaded-virtual
  -Wshorten-64-to-32
  -Wundef
)

