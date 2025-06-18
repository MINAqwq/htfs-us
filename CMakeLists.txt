cmake_minimum_required(VERSION 3.10)

project(HTFS-Userspace C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-fplan9-extensions -fms-extensions)

include_directories("./")

add_library(htfs SHARED
	libhtfs/bptree.c
	libhtfs/ctx.c
	libhtfs/entry.c
	libhtfs/helper.c
	libhtfs/map.c
)

add_executable(htfsinfo
	htfsinfo.c
)

add_executable(mkhtfs
	mkhtfs.c
)

target_link_libraries(htfs PRIVATE blake3)
target_link_libraries(htfsinfo PRIVATE htfs)
target_link_libraries(mkhtfs PRIVATE htfs)
