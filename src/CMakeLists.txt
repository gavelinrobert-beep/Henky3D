add_executable(Henky3D WIN32 main.cpp)

target_link_libraries(Henky3D PRIVATE
    Henky3DEngine
)

target_compile_features(Henky3D PRIVATE cxx_std_20)
