cmake_minimum_required(VERSION 3.10)

project(HTFS-Userspace C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-fplan9-extensions)

include_directories("./")

add_library(htfs SHARED
	libhtfs/ctx.c
	libhtfs/map.c
)

add_executable(htfsinfo
	htfsinfo.c
)

add_executable(mkhtfs
	mkhtfs.c
)

target_link_libraries(htfsinfo PRIVATE htfs)
target_link_libraries(mkhtfs PRIVATE htfs)
