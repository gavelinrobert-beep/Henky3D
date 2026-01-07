set(ENGINE_SOURCES
    core/Window.cpp
    core/Window.h
    core/Timer.h
    input/Input.cpp
    input/Input.h
    graphics/GraphicsDevice.cpp
    graphics/GraphicsDevice.h
    graphics/Renderer.cpp
    graphics/Renderer.h
    ecs/Components.h
    ecs/ECSWorld.h
)

add_library(Henky3DEngine STATIC ${ENGINE_SOURCES})

target_include_directories(Henky3DEngine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Henky3DEngine PUBLIC
    entt
    imgui
    d3d12.lib
    dxgi.lib
    dxcompiler.lib
)

target_compile_features(Henky3DEngine PUBLIC cxx_std_20)
