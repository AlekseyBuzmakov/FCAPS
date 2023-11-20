
include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)

add_subdirectory(FCAPS/src/fcaps/storages/)
add_subdirectory(FCAPS/src/fcaps/SharedModulesLib/)
add_subdirectory(FCAPS/src/fcaps/StdFCA/)
add_subdirectory(FCAPS/src/fcaps/PS-Modules/)
add_subdirectory(FCAPS/src/fcaps/SofiaModules/)
add_subdirectory(FCAPS/src/fcaps/ParallelPatternEnumeratorModules/)
add_subdirectory(FCAPS/src/fcaps/StabilityEstimatorContextProcessor/)
add_subdirectory(FCAPS/src/fcaps/Filters/)
add_subdirectory(FCAPS/src/fcaps/OptimisticEstimatorModules/)
add_subdirectory(FCAPS/src/fcaps/WestfallYoungOptimisticEstimatorModules/)
add_subdirectory(FCAPS/src/fcaps/ComputationProcedureModules/)
add_subdirectory(FCAPS/src/fcaps/LocalProjectionChainModules/)
add_subdirectory(FCAPS/src/fcaps/ContextAttributesModules/)
add_subdirectory(FCAPS/src/fcaps/BinContextReaderModules/)

