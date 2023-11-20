cmake_minimum_required(VERSION 3.5)
project(StdFCALib LANGUAGES CXX)

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)
include_directories(${CMAKE_SOURCE_DIR}/Sofia-PS/inc)

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} SHARED ${CPP_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/FCAPS/src)
target_link_libraries(${PROJECT_NAME} PUBLIC SharedTools storages SharedModulesLib)



