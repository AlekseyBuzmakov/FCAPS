# https://www.jibbow.com/posts/rapidjson-cmake/
# Download RapidJSON

include(FetchContent)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
set(RAPIDJSON_BUILD_TESTS OFF)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF)

FetchContent_Declare(
  RapidJSON
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
  GIT_TAG origin/master
  GIT_SUBMODULES_RECURSE true
  CMAKE_ARGS
	-DRAPIDJSON_BUILD_TESTS=OFF
	-DRAPIDJSON_BUILD_DOC=OFF
	-DRAPIDJSON_BUILD_EXAMPLES=OFF
  OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(RapidJSON)
