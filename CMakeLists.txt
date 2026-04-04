cmake_minimum_required(VERSION 3.10)
project(web_tinytcad_service)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src/webserver)
include_directories(src/tinytcad) 

find_library(MYSQL_LIB mysqlclient REQUIRED)
# find_path(MYSQL_INCLUDE_DIR mysql.h REQUIRED)
find_path(MYSQL_INCLUDE_DIR 
    NAMES mysql.h 
    PATH_SUFFIXES mysql  # 关键：告诉CMake去/usr/include/mysql/子目录找
    REQUIRED
)
include_directories(${MYSQL_INCLUDE_DIR})

file(GLOB WEB_SRC src/webserver/*.cpp)

add_executable(web_tinytcad_service ${WEB_SRC})

find_package(Threads REQUIRED)
# find_package(MySQL REQUIRED)
target_link_libraries(web_tinytcad_service PRIVATE Threads::Threads ${MYSQL_LIB})