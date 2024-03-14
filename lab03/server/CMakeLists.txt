cmake_minimum_required(VERSION 3.1)

project(Lab03Server VERSION 1.0 LANGUAGES CXX)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.74.0 COMPONENTS REQUIRED) 

add_executable(server src/main.cpp)

include_directories(${Boost_INCLUDE_DIRS}) 
target_link_libraries(server ${Boost_LIBRARIES})
