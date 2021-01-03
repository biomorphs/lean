cmake_minimum_required(VERSION 3.18)

project(Lean
  VERSION 1.0
  DESCRIPTION "Lean SDL/GL/C++/LUA Engine"
  LANGUAGES CXX)
  
set(SDL2Libs external/SDL2-2.0.12/lib/x64/SDL2 
			 external/SDL2-2.0.12/lib/x64/SDL2main)

add_library(Core STATIC)
target_sources(Core PRIVATE Source/core/file_io.h)
target_sources(Core PRIVATE Source/core/log.h)
target_sources(Core PRIVATE Source/core/semaphore.h)
target_sources(Core PRIVATE Source/core/thread.h)
target_sources(Core PRIVATE Source/core/time.h)
target_sources(Core PRIVATE Source/core/file_io.cpp)
target_sources(Core PRIVATE Source/core/log.cpp)
target_sources(Core PRIVATE Source/core/semaphore.cpp)
target_sources(Core PRIVATE Source/core/thread.cpp)
target_sources(Core PRIVATE Source/core/time.cpp)
target_sources(Core PRIVATE Source/core/profiler.h)
target_sources(Core PRIVATE Source/core/string_hashing.h)
target_include_directories(Core PRIVATE Source)
target_include_directories(Core PRIVATE external/SDL2-2.0.12/include/)

add_library(Engine STATIC)
target_sources(Engine PRIVATE Source/engine/platform.h)
target_sources(Engine PRIVATE Source/engine/platform.cpp)
target_sources(Engine PRIVATE Source/engine/system.h)
target_sources(Engine PRIVATE Source/engine/system_enumerator.h)
target_sources(Engine PRIVATE Source/engine/system_manager.cpp)
target_sources(Engine PRIVATE Source/engine/system_manager.h)
target_sources(Engine PRIVATE Source/engine/engine_startup.cpp)
target_sources(Engine PRIVATE Source/engine/engine_startup.h)
target_sources(Engine PRIVATE Source/engine/event_system.cpp)
target_sources(Engine PRIVATE Source/engine/event_system.h)



target_include_directories(Engine PRIVATE Source)
target_include_directories(Engine PRIVATE external/SDL2-2.0.12/include/)
target_link_libraries(Engine PRIVATE Core)

add_executable(Lean Source/main/main.cpp)
target_include_directories(Lean PRIVATE Source)
target_link_libraries(Lean PRIVATE Core)
target_link_libraries(Lean PRIVATE Engine)
target_include_directories(Lean PRIVATE Source/Engine)
target_link_libraries(Lean PRIVATE ../external/SDL2-2.0.12/lib/x64/SDL2 ../external/SDL2-2.0.12/lib/x64/SDL2main)