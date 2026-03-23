cmake_minimum_required(VERSION 3.10)
project(web_tinytcad_service)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/webserver)
include_directories(src/tinytcad) 

file(GLOB WEB_SRC src/webserver/*.cpp)

add_executable(web_tinytcad_service ${WEB_SRC})

find_package(Threads REQUIRED)
target_link_libraries(web_tinytcad_service PRIVATE Threads::Threads)