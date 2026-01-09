# External dependencies
include(FetchContent)

# GLAD - OpenGL Function Loader
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c
)
target_include_directories(glad PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

# EnTT - Header-only ECS library
set(ENTT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/entt)
if (NOT EXISTS "${ENTT_DIR}/single_include/entt/entt.hpp")
    message(STATUS "EnTT sources not found in external/entt; fetching from upstream")
    FetchContent_Declare(
        entt_content
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.2
        GIT_SHALLOW TRUE
    )
    FetchContent_Populate(entt_content)
    set(ENTT_DIR ${entt_content_SOURCE_DIR})
endif()

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${ENTT_DIR}/single_include)

# ImGui - Immediate mode GUI
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# Fetch ImGui if it isn't vendored in the repository
if (NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(STATUS "ImGui sources not found in external/imgui; fetching from upstream")
    FetchContent_Declare(
        imgui_content
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.5
        GIT_SHALLOW TRUE
    )
    FetchContent_Populate(imgui_content)
    set(IMGUI_DIR ${imgui_content_SOURCE_DIR})
endif()

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
