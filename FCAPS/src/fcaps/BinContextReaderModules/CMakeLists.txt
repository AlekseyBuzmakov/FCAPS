cmake_minimum_required(VERSION 3.5)
project(BinContextReaderModules LANGUAGES CXX)

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)
include_directories(${CMAKE_SOURCE_DIR}/Sofia-PS/inc)

FILE(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR} *.cpp)

add_library(${PROJECT_NAME} SHARED ${CPP_FILES})
target_include_directories(${PROJECT_NAME} BEFORE PUBLIC ${RapidJSON_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/FCAPS/src)
target_link_libraries(${PROJECT_NAME} PUBLIC SharedTools SharedModulesLib)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/modules/)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/modules/${targetfile}
)



