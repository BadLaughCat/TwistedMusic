cmake_minimum_required(VERSION 3.14)

project(TwistedMusic VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(BML REQUIRED)

add_library(TwistedMusic SHARED src/TwistedMusic.cpp)

target_include_directories(TwistedMusic PRIVATE ${BML_INCLUDE_DIRS})
target_link_libraries(TwistedMusic PRIVATE BML)

set_target_properties(TwistedMusic PROPERTIES
    OUTPUT_NAME "TwistedMusic"
)