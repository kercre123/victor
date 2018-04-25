cmake_minimum_required(VERSION 3.6)                                                                 

# config files that live in /anki/etc
add_subdirectory(config)

add_subdirectory(camera)

add_subdirectory(switchboard)

add_subdirectory(update-engine)

add_subdirectory(diagnostics-logger)
