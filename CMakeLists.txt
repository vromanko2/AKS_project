cmake_minimum_required(VERSION 3.13)
project(untitled)

set(CMAKE_CXX_STANDARD 14)


set(Gumbo_LIBRARY_DIR "/usr/local/opt/gumbo-parser/lib")
set(Gumbo_INCLUDE_DIR "/usr/local/opt/gumbo-parser/include")

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)


add_executable (WebCrawler main.cpp)
include_directories(${CURL_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${Gumboe_INCLUDE_DIR})
LINK_DIRECTORIES(${Gumbo_LIBRARY_DIR})

target_link_libraries(WebCrawler ${CURL_LIBRARIES})
target_link_libraries(WebCrawler "/usr/local/opt/gumbo-parser/lib/libgumbo.dylib")