cmake_minimum_required(VERSION 3.18)

add_library(Core STATIC)
target_sources(Core PRIVATE "atomics.h" "atomics.cpp")