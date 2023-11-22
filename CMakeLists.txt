cmake_minimum_required(VERSION 3.7)
project(Sofia LANGUAGES CXX)

set(CMAKE_DEBUG_POSTFIX d)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3")

include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")

find_package(Boost COMPONENTS system filesystem REQUIRED)
#find_package(RapidJSON REQUIRED NO_DEFAULT_PATH)

add_subdirectory(Tools/)

include("${CMAKE_SOURCE_DIR}/FCAPS/fcaps-lib.cmake")

include_directories(BEFORE ${RapidJSON_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/FCAPS/include)
link_directories(BEFORE ${PROJECT_NAME} ${Boost_LIBRARY_DIRS} ${rapidjson_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} Sofia-PS/src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE Sofia-PS/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC SharedTools ${Boost_LIBRARIES})

