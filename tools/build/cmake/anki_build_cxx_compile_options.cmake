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
# Use -fdiagnostics-absolute-paths to enable IDEs like Visual Studio Code
# that don't know the source directory used for each compilation.
#

set(ANKI_BUILD_CXX_COMPILE_OPTIONS
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-Os>
  $<$<BOOL:${MACOSX}>:-fobjc-arc>
  $<$<BOOL:${IOS}>:-fobjc-arc>
  $<$<AND:$<CXX_COMPILER_ID:AppleClang>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},9.0>>:-fdiagnostics-absolute-paths>
  $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},5.0>>:-fdiagnostics-absolute-paths>
  -fdiagnostics-show-category=name
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
  -Wunused-variable
  -Wno-unused-command-line-argument
)
