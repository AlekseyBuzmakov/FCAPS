cmake_minimum_required(VERSION 3.5)
project(SharedModulesLib LANGUAGES CXX)

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)

FILE(GLOB CPP_FILES "${PROJECT_SOURCE_DIR}/*.cpp")
LIST(REMOVE_ITEM CPP_FILES "${PROJECT_SOURCE_DIR}/BlockBitSetDescriptor.cpp")

add_library(${PROJECT_NAME} STATIC ${CPP_FILES})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/FCAPS/src)
target_link_libraries(${PROJECT_NAME} PUBLIC SharedTools)



