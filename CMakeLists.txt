cmake_minimum_required(VERSION 3.7)
project(Sofia LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3")

include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(RapidJSON REQUIRED)

add_subdirectory(Tools/)

include("${CMAKE_SOURCE_DIR}/FCAPS/fcaps-lib.cmake")

include_directories(${CMAKE_SOURCE_DIR}/FCAPS/include)
link_directories(${PROJECT_NAME} ${Boost_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} Sofia-PS/src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE Sofia-PS/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC SharedTools ${Boost_LIBRARIES})

