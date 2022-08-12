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

set(PhysXLibPathRelease ${CMAKE_CURRENT_SOURCE_DIR}/../PhysX/physx/bin/win.x86_64.vc142.mt/checked/)
set(PhysXLibPathDebug ${CMAKE_CURRENT_SOURCE_DIR}/../PhysX/physx/bin/win.x86_64.vc142.mt/debug/)

set(CommonCompilerOptions /WX /MP)

add_library(Core STATIC)
set(CORE_SOURCES
	source/core/base64.h
	source/core/base64.cpp
	source/core/file_io.h
	source/core/log.h
	source/core/semaphore.h
	source/core/thread.h
	source/core/time.h
	source/core/file_io.cpp
	source/core/log.cpp
	source/core/semaphore.cpp
	source/core/thread.cpp
	source/core/time.cpp
	source/core/profiler.h
	source/core/string_hashing.h
	source/core/thread_pool.h
	source/core/thread_pool.cpp
	source/core/glm_headers.h
	source/core/timer.h
	source/core/timer.cpp
	source/core/mutex.h
	source/core/mutex.cpp
	source/core/random.h
	source/core/random.cpp)
target_sources(Core PRIVATE ${CORE_SOURCES})
target_include_directories(Core PRIVATE ${CommonIncludePaths})
target_compile_options(Core PRIVATE ${CommonCompilerOptions})

add_library(Render STATIC)
set(RENDER_SOURCES
	source/render/camera.h
	source/render/camera.inl
	source/render/fence.h
	source/render/device.cpp
	source/render/device.h
	source/render/frame_buffer.cpp
	source/render/frame_buffer.h
	source/render/material.cpp
	source/render/material.h
	source/render/mesh.cpp
	source/render/mesh.h
	source/render/mesh_builder.cpp
	source/render/mesh_builder.h
	source/render/render_buffer.cpp
	source/render/render_buffer.h
	source/render/render_pass.cpp
	source/render/render_pass.h
	source/render/shader_binary.cpp
	source/render/shader_binary.h
	source/render/shader_program.cpp
	source/render/shader_program.h
	source/render/texture.cpp
	source/render/texture.h
	source/render/texture_source.h
	source/render/texture_source.inl
	source/render/uniform_buffer.cpp
	source/render/uniform_buffer.h
	source/render/utils.h
	source/render/vertex_array.cpp
	source/render/vertex_array.h
	source/render/window.cpp
	source/render/window.h
	source/render/render_target_blitter.cpp
	source/render/render_target_blitter.h)
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
	source/engine/new_render.h
	source/engine/new_render.cpp
	source/engine/material_helpers.h
	source/engine/material_helpers.cpp
	source/engine/physics_handle.h
	source/engine/physics_handle.cpp
	source/engine/physics_system.h
	source/engine/physics_system.cpp
	source/engine/character_controller_system.h
	source/engine/character_controller_system.cpp
	source/engine/intersection_tests.h
	source/engine/intersection_tests.cpp
	source/engine/raycast_system.h
	source/engine/raycast_system.cpp
	source/engine/tag.h
	source/engine/tag.cpp
	source/engine/platform.h
	source/engine/platform.cpp
	source/engine/system.h
	source/engine/system_manager.cpp
	source/engine/system_manager.h
	source/engine/engine_startup.cpp
	source/engine/engine_startup.h
	source/engine/event_system.cpp
	source/engine/event_system.h
	source/engine/job.h
	source/engine/job.cpp
	source/engine/job_queue.h
	source/engine/job_queue.cpp
	source/engine/job_system.h
	source/engine/job_system.cpp
	source/engine/controller_state.h
	source/engine/input_system.cpp
	source/engine/input_system.h
	source/engine/keyboard_state.h
	source/engine/mouse_state.h
	source/engine/render_system.cpp
	source/engine/render_system.h
	source/engine/script_system.cpp
	source/engine/script_system.h
	source/engine/debug_gui_menubar.h
	source/engine/debug_gui_menubar.cpp
	source/engine/debug_gui_system.h
	source/engine/debug_gui_system.cpp
	source/engine/graph_data_buffer.h
	source/engine/graph_data_buffer.cpp
	source/engine/imgui_sdl_gl3_render.h
	source/engine/imgui_sdl_gl3_render.cpp
	source/engine/debug_camera.h
	source/engine/debug_camera.cpp
	source/engine/arcball_camera.h
	source/engine/arcball_camera.cpp
	source/engine/model_manager.h
	source/engine/model_manager.cpp
	source/engine/texture_manager.h
	source/engine/texture_manager.cpp
	source/engine/shader_manager.h
	source/engine/shader_manager.cpp
	source/engine/platform.cpp
	source/engine/stb_image.cpp
	source/engine/stb_image.h
	source/engine/model.h
	source/engine/model.cpp
	source/engine/model_asset.h
	source/engine/model_asset.cpp
	source/engine/renderer.h
	source/engine/renderer.cpp
	source/engine/light.h
	source/engine/mesh_instance.h
	source/engine/debug_render.cpp
	source/engine/debug_render.h
	source/engine/serialisation.inl
	source/engine/serialisation.h
	source/engine/file_picker_dialog.cpp
	source/engine/file_picker_dialog.h
	source/engine/frustum.h
	source/engine/sdf_mesh_octree.h
	source/engine/sdf_mesh_octree.cpp
	source/engine/sdf_mesh_builder.h
	source/engine/sdf_mesh_builder.cpp
	source/engine/sdf.h
	source/engine/sdf.cpp
	source/engine/sdf_mesh_system.h
	source/engine/sdf_mesh_system.cpp
	source/engine/camera_system.h
	source/engine/camera_system.cpp
	source/engine/graphics_system.h
	source/engine/graphics_system.cpp
	source/engine/components/component_script.h
	source/engine/components/component_script.cpp
	source/engine/components/component_camera.h
	source/engine/components/component_camera.cpp
	source/engine/components/component_sdf_mesh.h
	source/engine/components/component_sdf_mesh.cpp
	source/engine/components/component_sdf_model.h
	source/engine/components/component_sdf_model.cpp
	source/engine/components/component_model.h
	source/engine/components/component_model.cpp
	source/engine/components/component_model_part_materials.h
	source/engine/components/component_model_part_materials.cpp
	source/engine/components/component_transform.h
	source/engine/components/component_transform.cpp
	source/engine/components/component_light.h
	source/engine/components/component_light.cpp
	source/engine/components/component_tags.h
	source/engine/components/component_tags.cpp
	source/engine/components/component_physics.h
	source/engine/components/component_physics.cpp
	source/engine/components/component_material.h
	source/engine/components/component_material.cpp
	source/engine/components/component_environment_settings.h
	source/engine/components/component_environment_settings.cpp
	source/engine/components/component_character_controller.h
	source/engine/components/component_character_controller.cpp
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
	source/entity/entity_handle.h
	source/entity/entity_handle.cpp
	source/entity/world.h
	source/entity/world.cpp
	source/entity/entity_system.h
	source/entity/entity_system.cpp
	source/entity/component.h
	source/entity/component_storage.h
	source/entity/component_storage.inl
	source/entity/component_handle.h
	source/entity/component_handle.inl
	source/entity/component_inspector.h
	source/entity/basic_inspector.h
	source/entity/basic_inspector.cpp
)
target_sources(Entity PRIVATE ${ENTITY_SOURCES})
target_include_directories(Entity PRIVATE ${CommonIncludePaths})
target_include_directories(Entity PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Entity PRIVATE ${JSONIncludePaths})

add_library(Creatures STATIC)
set(CREATURES_SOURCES
	source/playground/creatures/creature_system.h
	source/playground/creatures/creature_system.cpp
	source/playground/creatures/component_creature.h
	source/playground/creatures/component_creature.cpp
	source/playground/creatures/behaviour_library.h
	source/playground/creatures/behaviour_library.cpp
	source/playground/creatures/blackboard.h
	source/playground/creatures/blackboard.cpp
)
target_sources(Creatures PRIVATE ${CREATURES_SOURCES})
target_include_directories(Creatures PRIVATE ${CommonIncludePaths})
target_include_directories(Creatures PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Creatures PRIVATE ${JSONIncludePaths})
target_compile_options(Creatures PRIVATE ${CommonCompilerOptions})

add_library(Survivors STATIC)
set(SURVIVORS_SOURCES
	source/survivors/entity_grid.h
	source/survivors/entity_grid.cpp
	source/survivors/survivors_main.h
	source/survivors/survivors_main.cpp
	source/survivors/player_component.h
	source/survivors/player_component.cpp
	source/survivors/monster_component.h
	source/survivors/monster_component.cpp
	source/survivors/explosion_component.h
	source/survivors/explosion_component.cpp
	source/survivors/world_tile_system.h
	source/survivors/world_tile_system.cpp
	source/survivors/world_tile_component.h
	source/survivors/world_tile_component.cpp
)
target_sources(Survivors PRIVATE ${SURVIVORS_SOURCES})
target_include_directories(Survivors PRIVATE ${CommonIncludePaths})
target_include_directories(Survivors PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Survivors PRIVATE ${JSONIncludePaths})
target_compile_options(Survivors PRIVATE ${CommonCompilerOptions})

add_library(Playground STATIC)
set(PLAYGROUND_SOURCES
	source/playground/playground.h
	source/playground/playground.cpp
	source/playground/scene.h
	source/playground/scene.cpp
	source/playground/scene_editor.h
	source/playground/scene_editor.cpp
	source/playground/walkable_area.h
	source/playground/walkable_area.cpp
	source/playground/walkable_system.h
	source/playground/walkable_system.cpp
)

target_sources(Playground PRIVATE ${PLAYGROUND_SOURCES})
target_include_directories(Playground PRIVATE ${CommonIncludePaths})
target_include_directories(Playground PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Playground PRIVATE ${JSONIncludePaths})
target_compile_options(Playground PRIVATE ${CommonCompilerOptions})

add_library(Editor STATIC)
set(EDITOR_SOURCES
	source/editor/editor.h
	source/editor/editor.cpp
	source/editor/editor_component_inspector.h
	source/editor/editor_component_inspector.cpp
	source/editor/transform_widget.h
	source/editor/transform_widget.cpp
	source/editor/command.h
	source/editor/command_list.cpp
	source/editor/command_list.h
	source/editor/utils.h
	source/editor/utils.cpp
	source/editor/commands/editor_close_cmd.cpp
	source/editor/commands/editor_close_cmd.h
	source/editor/commands/editor_new_scene_cmd.cpp
	source/editor/commands/editor_new_scene_cmd.h
	source/editor/commands/editor_save_scene_cmd.cpp
	source/editor/commands/editor_save_scene_cmd.h
	source/editor/commands/editor_import_scene_cmd.cpp
	source/editor/commands/editor_import_scene_cmd.h
	source/editor/commands/editor_create_entity_from_mesh_cmd.h
	source/editor/commands/editor_create_entity_from_mesh_cmd.cpp
	source/editor/commands/editor_clear_selection_cmd.h
	source/editor/commands/editor_clear_selection_cmd.cpp
	source/editor/commands/editor_select_all_cmd.h
	source/editor/commands/editor_select_all_cmd.cpp
	source/editor/commands/editor_delete_selection_cmd.h
	source/editor/commands/editor_delete_selection_cmd.cpp
	source/editor/commands/editor_clone_selection_cmd.h
	source/editor/commands/editor_clone_selection_cmd.cpp
	source/editor/commands/editor_select_entity_cmd.h
	source/editor/commands/editor_select_entity_cmd.cpp
	source/editor/commands/editor_set_value_cmd.h
	source/editor/commands/editor_set_entity_positions_cmd.h
	source/editor/commands/editor_set_entity_positions_cmd.cpp
)

target_sources(Editor PRIVATE ${EDITOR_SOURCES})
target_include_directories(Editor PRIVATE ${CommonIncludePaths})
target_include_directories(Editor PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Editor PRIVATE ${JSONIncludePaths})
target_compile_options(Editor PRIVATE ${CommonCompilerOptions})

add_executable(Lean Source/main/main.cpp)
target_include_directories(Lean PRIVATE ${CommonIncludePaths})
target_include_directories(Lean PRIVATE ${LuaAndSolIncludePaths})
target_include_directories(Lean PRIVATE ${JSONIncludePaths})
set_target_properties(Lean PROPERTIES VS_DPI_AWARE "ON")
target_link_libraries(Lean PRIVATE Core)
target_link_libraries(Lean PRIVATE Engine)
target_link_libraries(Lean PRIVATE Entity)
target_link_libraries(Lean PRIVATE Playground)
target_link_libraries(Lean PRIVATE Editor)
target_link_libraries(Lean PRIVATE Creatures)
target_link_libraries(Lean PRIVATE Survivors)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysX_64.lib)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysXCommon_64.lib)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysXCooking_64.lib)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysXFoundation_64.lib)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysXPvdSDK_static_64.lib)
target_link_libraries(Lean PRIVATE optimized ${PhysXLibPathRelease}/PhysXExtensions_static_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysX_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysXCommon_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysXCooking_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysXFoundation_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysXPvdSDK_static_64.lib)
target_link_libraries(Lean PRIVATE debug ${PhysXLibPathDebug}/PhysXExtensions_static_64.lib)
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