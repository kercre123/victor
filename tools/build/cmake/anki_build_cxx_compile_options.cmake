#
# Declare compile options shared by all Anki C++ projects
#
# Warning flags can be suppressed for specific targets with
#   target_compile_options(name
#     -Wno-error=blah
#   )
# where needed.
#
# Use -fdiagnostics-show-category to identify flags associated with
# each warning.
#
# Use -fdiagnostics-absolute-paths to enable IDEs like Visual Code
# that don't know the source directory used for each compilation.
#

set(ANKI_BUILD_CXX_COMPILE_OPTIONS
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-Os>
  $<$<BOOL:${MACOSX}>:-fobjc-arc>
  $<$<BOOL:${IOS}>:-fobjc-arc>
  -fdiagnostics-show-category=name
  -fdiagnostics-absolute-paths
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

