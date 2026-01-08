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
    graphics/ConstantBuffers.h
    graphics/Material.h
    graphics/AssetRegistry.cpp
    graphics/AssetRegistry.h
    graphics/ShadowMap.cpp
    graphics/ShadowMap.h
    graphics/FrameGraph.cpp
    graphics/FrameGraph.h
    ecs/Components.h
    ecs/ECSWorld.h
    ecs/TransformSystem.cpp
    ecs/TransformSystem.h
    ecs/CullingSystem.cpp
    ecs/CullingSystem.h
)

add_library(Henky3DEngine STATIC ${ENGINE_SOURCES})

target_include_directories(Henky3DEngine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Henky3DEngine PUBLIC
    glad
    glfw
    glm::glm
    entt
    imgui
    OpenGL::GL
)

target_compile_features(Henky3DEngine PUBLIC cxx_std_20)
