cmake_minimum_required(VERSION 3.13)
project(untitled)

set(CMAKE_CXX_STANDARD 14)


set(Gumbo_LIBRARY_DIR "/usr/local/opt/gumbo-parser/lib")
set(Gumbo_INCLUDE_DIR "/usr/local/opt/gumbo-parser/include")

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)


find_package(Boost COMPONENTS filesystem REQUIRED)


add_executable (WebCrawler main.cpp MainTask/main_task.h MainTask/main_task.cpp hrefStruct/href_t.h ThreadsafeQueue/threadsafe_queue.h Configuration/configuration.h TimeMeasurement/time_measurement.h TimeMeasurement/time_measurement.cpp)
include_directories(${CURL_INCLUDE_DIR})

INCLUDE_DIRECTORIES(${Gumboe_INCLUDE_DIR})
LINK_DIRECTORIES(${Gumbo_LIBRARY_DIR})

IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF (Boost_FOUND)

target_link_libraries(WebCrawler ${CURL_LIBRARIES})
target_link_libraries(WebCrawler "/usr/local/opt/gumbo-parser/lib/libgumbo.dylib")

target_link_libraries(WebCrawler ${Boost_LIBRARIES})