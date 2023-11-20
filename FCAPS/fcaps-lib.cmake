
include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)

add_subdirectory(FCAPS/src/fcaps/storages/)
add_subdirectory(FCAPS/src/fcaps/SharedModulesLib/)
add_subdirectory(FCAPS/src/fcaps/StdFCA/)
add_subdirectory(FCAPS/src/fcaps/PS-Modules/)
add_subdirectory(FCAPS/src/fcaps/SofiaModules/)
add_subdirectory(FCAPS/src/fcaps/ParallelPatternEnumeratorModules/)

