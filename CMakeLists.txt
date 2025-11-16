cmake_minimum_required(VERSION 3.10)
project(MultiApps)

option(BUILD_HTTP_SERVER "Build Http Server" ON)

if(BUILD_HTTP_SERVER)
    message(STATUS "Building Http Server ...")
    include(${CMAKE_SOURCE_DIR}/HttpServerCMakeLists.txt)
endif()