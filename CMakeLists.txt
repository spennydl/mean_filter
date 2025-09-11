cmake_minimum_required(VERSION 3.18)
project(
  mean_filter 
  VERSION 0.0.1
  DESCRIPTION "Implement mean filtering efficiently"
  LANGUAGES CXX C)

add_subdirectory(vendor)

file(GLOB MEAN_FILTER_SRCS "src/*.cpp")

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Werror)
add_compile_options(-fdiagnostics-color)

add_executable(mean_filter ${MEAN_FILTER_SRCS})
target_link_libraries(mean_filter LINK_PRIVATE SDL3::SDL3 stb_image m)

