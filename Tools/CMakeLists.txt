cmake_minimum_required(VERSION 3.5)
project(SharedTools LANGUAGES CXX)

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} STATIC ${CPP_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} BEFORE PUBLIC ${RapidJSON_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC)

