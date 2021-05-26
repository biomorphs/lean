cmake_minimum_required(VERSION 3.18)

project(Lean
  VERSION 1.0
  DESCRIPTION "C++/Lua engine"
  LANGUAGES CXX)
  
set(CMAKE_CXX_STANDARD 17)
  
find_package(OpenGL REQUIRED)

set(UseLuaJIT 1)

set(CommonIncludePaths 
	Source
	external/SDL2-2.0.12/include/
	external/glm/include/
	external/glew-2.1.0/include/
	external/fork-awesome/
	external/Optick_1.3.1/include
	external/robin-hood-hashing/src/include/
)

if(UseLuaJIT)
	set(LuaAndSolIncludePaths
		../luajit/src/
		external/sol2-2.20.6/
	)
else()
	set(LuaAndSolIncludePaths
		external/lua-5.3.5_Win64_vc16_lib/include/
		external/sol2-2.20.6/
	)
endif()

set(JSONIncludePaths
	external/json-3.9.1/include/
)

set(PysXIncludePaths
	../physx/physx/include/
	../physx/pxshared/include/
)

set(CommonCompilerOptions /WX /MP)

add_library(Core STATIC)
set(CORE_SOURCES
	Source/core/file_io.h
	Source/core/log.h
	Source/core/semaphore.h
	Source/core/thread.h
	Source/core/time.h
	Source/core/file_io.cpp
	Source/core/log.cpp
	Source/core/semaphore.cpp
	Source/core/thread.cpp
	Source/core/time.cpp
	Source/core/profiler.h
	Source/core/string_hashing.h
	Source/core/thread_pool.h
	Source/core/thread_pool.cpp
	Source/core/glm_headers.h
	Source/core/timer.h
	Source/core/timer.cpp
	Source/core/mutex.h
	Source/core/mutex.cpp
	Source/core/random.h
	Source/core/random.cpp)
target_sources(Core PRIVATE ${CORE_SOURCES})
target_include_directories(Core PRIVATE ${CommonIncludePaths})
target_compile_options(Core PRIVATE ${CommonCompilerOptions})

add_library(Render STATIC)
set(RENDER_SOURCES
	Source/render/camera.h
	Source/render/camera.inl
	Source/render/device.cpp
	Source/render/device.h
	Source/render/frame_buffer.cpp
	Source/render/frame_buffer.h
	Source/render/material.cpp
	Source/render/material.h
	Source/render/mesh.cpp
	Source/render/mesh.h
	Source/render/mesh_builder.cpp
	Source/render/mesh_builder.h
	Source/render/render_buffer.cpp
	Source/render/render_buffer.h
	Source/render/render_pass.cpp
	Source/render/render_pass.h
	Source/render/shader_binary.cpp
	Source/render/shader_binary.h
	Source/render/shader_program.cpp
	Source/render/shader_program.h
	Source/render/texture.cpp
	Source/render/texture.h
	Source/render/texture_source.h
	Source/render/texture_source.inl
	Source/render/uniform_buffer.cpp
	Source/render/uniform_buffer.h
	Source/render/utils.h
	Source/render/vertex_array.cpp
	Source/render/vertex_array.h
	Source/render/window.cpp
	Source/render/window.h
	Source/render/render_target_blitter.cpp
	Source/render/render_target_blitter.h)
target_sources(Render PRIVATE ${RENDER_SOURCES})
target_include_directories(Render PRIVATE ${CommonIncludePaths})
target_compile_options(Render PRIVATE ${CommonCompilerOptions})

add_library(ImguiLib STATIC)
set(IMGUILIB_SOURCES
	external/imgui-docking/imgui/imgui.h
	external/imgui-docking/imgui/imgui.cpp
	external/imgui-docking/imgui/imgui_demo.cpp
	external/imgui-docking/imgui/imgui_draw.cpp
	external/imgui-docking/imgui/imgui_tables.cpp
	external/imgui-docking/imgui/imgui_widgets.cpp
	external/imgui-docking/imgui/backends/imgui_impl_opengl3.cpp
	external/imgui-docking/imgui/backends/imgui_impl_sdl.cpp)
target_sources(ImguiLib PRIVATE ${IMGUILIB_SOURCES})
target_include_directories(ImguiLib PRIVATE ${CommonIncludePaths})
target_compile_options(ImguiLib PRIVATE ${CommonCompilerOptions})

add_library(Engine STATIC)
set(ENGINE_SOURCES
	Source/engine/physics_handle.h
	Source/engine/physics_handle.cpp
	Source/engine/physics_system.h
	Source/engine/physics_system.cpp
	Source/engine/tag.h
	Source/engine/tag.cpp
	Source/engine/platform.h
	Source/engine/platform.cpp
	Source/engine/system.h
	Source/engine/system_manager.cpp
	Source/engine/system_manager.h
	Source/engine/engine_startup.cpp
	Source/engine/engine_startup.h
	Source/engine/event_system.cpp
	Source/engine/event_system.h
	Source/engine/job.h
	Source/engine/job.cpp
	Source/engine/job_queue.h
	Source/engine/job_queue.cpp
	Source/engine/job_system.h
	Source/engine/job_system.cpp
	Source/engine/controller_state.h
	Source/engine/input_system.cpp
	Source/engine/input_system.h
	Source/engine/keyboard_state.h
	Source/engine/mouse_state.h
	Source/engine/render_system.cpp
	Source/engine/render_system.h
	Source/engine/script_system.cpp
	Source/engine/script_system.h
	Source/engine/debug_gui_menubar.h
	Source/engine/debug_gui_menubar.cpp
	Source/engine/debug_gui_system.h
	Source/engine/debug_gui_system.cpp
	Source/engine/graph_data_buffer.h
	Source/engine/graph_data_buffer.cpp
	Source/engine/imgui_sdl_gl3_render.h
	Source/engine/imgui_sdl_gl3_render.cpp
	Source/engine/debug_camera.h
	Source/engine/debug_camera.cpp
	Source/engine/arcball_camera.h
	Source/engine/arcball_camera.cpp
	Source/engine/model_manager.h
	Source/engine/model_manager.cpp
	Source/engine/texture_manager.h
	Source/engine/texture_manager.cpp
	Source/engine/shader_manager.h
	Source/engine/shader_manager.cpp
	Source/engine/platform.cpp
	Source/engine/stb_image.cpp
	Source/engine/stb_image.h
	Source/engine/model.h
	Source/engine/model.cpp
	Source/engine/model_asset.h
	Source/engine/model_asset.cpp
	Source/engine/renderer.h
	Source/engine/renderer.cpp
	Source/engine/light.h
	Source/engine/mesh_instance.h
	Source/engine/material_helpers.cpp
	Source/engine/material_helpers.h
	Source/engine/debug_render.cpp
	Source/engine/debug_render.h
	Source/engine/serialisation.inl
	Source/engine/serialisation.h
	Source/engine/file_picker_dialog.cpp
	Source/engine/file_picker_dialog.h
	Source/engine/frustum.h
	Source/engine/sdf_mesh_builder.h
	Source/engine/sdf_mesh_builder.cpp
	Source/engine/sdf.h
	Source/engine/sdf.cpp
	Source/engine/graphics_system.h
	Source/engine/graphics_system.cpp
	Source/engine/components/component_sdf_model.h
	Source/engine/components/component_sdf_model.cpp
	Source/engine/components/component_model.h
	Source/engine/components/component_model.cpp
	Source/engine/components/component_transform.h
	Source/engine/components/component_transform.cpp
	Source/engine/components/component_light.h
	Source/engine/components/component_light.cpp
	Source/engine/components/component_tags.h
	Source/engine/components/component_tags.cpp
	Source/engine/components/component_physics.h
	Source/engine/components/component_physics.cpp
	Source/engine/components/component_material.h
	Source/engine/components/component_material.cpp
	)
target_sources(Engine PRIVATE ${ENGINE_SOURCES})
target_include_directories(Engine PRIVATE ${CommonIncludePaths})
target_include_directories(Engine PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Engine PRIVATE ${JSONIncludePaths})
target_include_directories(Engine PRIVATE ${PysXIncludePaths})
target_include_directories(Engine PRIVATE external/imgui-docking/)
target_include_directories(Engine PRIVATE external/assimp-5.0.1/include/)
target_include_directories(Engine PRIVATE external/qef/)
target_link_libraries(Engine PRIVATE Core)
target_link_libraries(Engine PRIVATE Render)
target_link_libraries(Engine PRIVATE ImguiLib)
target_compile_options(Engine PRIVATE ${CommonCompilerOptions})

add_library(Entity STATIC)
set(ENTITY_SOURCES
	Source/entity/entity_handle.h
	Source/entity/world.h
	Source/entity/world.cpp
	Source/entity/entity_system.h
	Source/entity/entity_system.cpp
	Source/entity/component.h
	Source/entity/component_storage.h
	Source/entity/component_storage.inl
)
target_sources(Entity PRIVATE ${ENTITY_SOURCES})
target_include_directories(Entity PRIVATE ${CommonIncludePaths})
target_include_directories(Entity PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Entity PRIVATE ${JSONIncludePaths})

add_library(Playground STATIC)
set(PLAYGROUND_SOURCES
	Source/playground/playground.h
	Source/playground/playground.cpp
	Source/playground/scene.h
	Source/playground/scene.cpp
	Source/playground/scene_editor.h
	Source/playground/scene_editor.cpp
	Source/playground/creatures/creature_system.h
	Source/playground/creatures/creature_system.cpp
	Source/playground/creatures/component_creature.h
	Source/playground/creatures/component_creature.cpp
	Source/playground/creatures/behaviour_library.h
	Source/playground/creatures/behaviour_library.cpp
	Source/playground/creatures/blackboard.h
	Source/playground/creatures/blackboard.cpp)
target_sources(Playground PRIVATE ${PLAYGROUND_SOURCES})
target_include_directories(Playground PRIVATE ${CommonIncludePaths})
target_include_directories(Playground PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Playground PRIVATE ${JSONIncludePaths})
target_compile_options(Playground PRIVATE ${CommonCompilerOptions})

add_executable(Lean Source/main/main.cpp)
target_include_directories(Lean PRIVATE ${CommonIncludePaths})
target_include_directories(Lean PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Lean PRIVATE ${JSONIncludePaths})
set_target_properties(Lean PROPERTIES VS_DPI_AWARE "ON")
target_link_libraries(Lean PRIVATE Core)
target_link_libraries(Lean PRIVATE Engine)
target_link_libraries(Lean PRIVATE Entity)
target_link_libraries(Lean PRIVATE Playground)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysX_64.lib)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysXCommon_64.lib)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysXCooking_64.lib)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysXFoundation_64.lib)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysXPvdSDK_static_64.lib)
target_link_libraries(Lean PRIVATE optimized E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/checked/PhysXExtensions_static_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysX_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysXCommon_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysXCooking_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysXFoundation_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysXPvdSDK_static_64.lib)
target_link_libraries(Lean PRIVATE debug E:/Git/PhysX/physx/bin/win.x86_64.vc142.mt/debug/PhysXExtensions_static_64.lib)
target_link_libraries(Lean PRIVATE ../external/assimp-5.0.1/lib/release/assimp-vc142-mt)
target_link_libraries(Lean PRIVATE ../external/glew-2.1.0/lib/Release/x64/glew32)
if(UseLuaJIT)
	target_link_libraries(Lean PRIVATE ../external/luajit/luajit)
	target_link_libraries(Lean PRIVATE ../external/luajit/lua51)
else()
	target_link_libraries(Lean PRIVATE ../external/lua-5.3.5_Win64_vc16_lib/lua53)
endif()
target_link_libraries(Lean PRIVATE ../external/SDL2-2.0.12/lib/x64/SDL2 ../external/SDL2-2.0.12/lib/x64/SDL2main)
target_link_libraries(Lean PRIVATE ../external/Optick_1.3.1/lib/x64/release/OptickCore)
target_link_libraries(Lean PRIVATE opengl32)
target_compile_options(Lean PRIVATE ${CommonCompilerOptions})