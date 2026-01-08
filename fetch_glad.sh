#!/bin/bash
cd external
rm -rf glad
mkdir -p glad
cd glad

# Create directory structure  
mkdir -p include/glad include/KHR src

# Download KHR platform header
curl -o include/KHR/khrplatform.h https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api/KHR/khrplatform.h

# Download pre-generated GLAD files for GL 4.6 Core
curl -o include/glad/glad.h https://raw.githubusercontent.com/Dav1dde/glad/glad2/example/c/gl-latest/include/glad/glad.h
curl -o src/glad.c https://raw.githubusercontent.com/Dav1dde/glad/glad2/example/c/gl-latest/src/glad.c

echo "GLAD files downloaded successfully!"
