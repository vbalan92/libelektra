# avoid build artifacts in the source folder
get_filename_component (CMAKE_CURRENT_BINARY_PARENT_DIR "${CMAKE_CURRENT_BINARY_DIR}" DIRECTORY)
file (COPY "${CMAKE_CURRENT_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_PARENT_DIR}")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/specelektra.cabal.in" "${CMAKE_CURRENT_BINARY_DIR}/specelektra.cabal" @ONLY)
