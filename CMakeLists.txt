cmake_minimum_required(VERSION 3.15)
project(FFExample)
add_executable(FFE FFExample.c)
target_link_libraries(FFE avformat avcodec avutil)
