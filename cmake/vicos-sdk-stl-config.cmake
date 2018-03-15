# Copy shared STL files to Android Studio output directory so they can be
# packaged in the APK.
# Usage:
#
#   find_package(ndk-stl REQUIRED)
#
# or
#
#   find_package(ndk-stl REQUIRED PATHS ".")

function(configure_shared_stl lib_path so_lib)
  message(STATUS "Configuring STL ${so_base} for vicos")
  configure_file(
    "${VICOS_SDK}/${lib_path}/libc++.so.1"
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${so_lib}" 
    COPYONLY)
endfunction()

configure_shared_stl("sysroot/usr/lib" "libc++.so.1")
configure_shared_stl("sysroot/usr/lib" "libatomic.so.1")
