#include "Renderer.h"
#include "../ecs/ECSWorld.h"
#include "../ecs/Components.h"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace Henky3D {

static std::string GetExecutableDirectory() {
    std::string exePath;
    
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        exePath = std::string(path, len);
    }
#elif defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        exePath = std::string(path);
    }
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        exePath = std::string(path);
    }
#endif
    
    if (!exePath.empty()) {
        return std::filesystem::path(exePath).parent_path().string();
    }
    return "";
}

static std::string GetShaderPath(const char* filename) {
    namespace fs = std::filesystem;
    
    // Priority 1: HENKY_ASSET_DIR environment variable
    if (const char* assetDir = std::getenv("HENKY_ASSET_DIR")) {
        fs::path path = fs::path(assetDir) / "shaders" / filename;
        if (fs::exists(path)) {
            return path.string();
        }
    }
    
    // Priority 2: Relative to executable directory
    std::string exeDir = GetExecutableDirectory();
    if (!exeDir.empty()) {
        // Try shaders/ subdirectory relative to exe
        fs::path path = fs::path(exeDir) / "shaders" / filename;
        if (fs::exists(path)) {
            return path.string();
        }
        
        // Try ../shaders/ (one level up from exe)
        path = fs::path(exeDir) / ".." / "shaders" / filename;
        if (fs::exists(path)) {
            try {
                return fs::canonical(path).string();
            } catch (...) {
                // If canonicalization fails, use the path as-is
                return path.string();
            }
        }
    }
    
    // Priority 3: Fallback to original relative path from build directory
    fs::path fallbackPath = fs::path("../../../shaders/") / filename;
    if (fs::exists(fallbackPath)) {
        return fallbackPath.string();
    }
    
    // If nothing worked, return the relative path and let it fail with a clear error
    return std::string("../../../shaders/") + filename;
}

Renderer::Renderer(GraphicsDevice* device) 
    : m_Device(device), m_DepthPrepassEnabled(true), m_ShadowsEnabled(true),
      m_ForwardProgram(0), m_DepthPrepassProgram(0), m_ShadowProgram(0),
      m_CubeVAO(0), m_CubeVBO(0), m_CubeIBO(0), m_IndexCount(0),
      m_PerFrameUBO(0), m_PerDrawUBO(0) {
    
    m_AssetRegistry = std::make_unique<AssetRegistry>(device);
    m_ShadowMap = std::make_unique<ShadowMap>(device, 2048);
    
    // Initialize default textures
    m_AssetRegistry->InitializeDefaults();
    
    CreateShaderPrograms();
    CreateCubeGeometry();
    
    // Create uniform buffers
    glGenBuffers(1, &m_PerFrameUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_PerFrameUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PerFrameConstants), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_PerFrameUBO);
    
    glGenBuffers(1, &m_PerDrawUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_PerDrawUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PerDrawConstants), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_PerDrawUBO);
    
    std::cout << "Renderer initialized with OpenGL" << std::endl;
}

Renderer::~Renderer() {
    if (m_CubeVAO) glDeleteVertexArrays(1, &m_CubeVAO);
    if (m_CubeVBO) glDeleteBuffers(1, &m_CubeVBO);
    if (m_CubeIBO) glDeleteBuffers(1, &m_CubeIBO);
    if (m_PerFrameUBO) glDeleteBuffers(1, &m_PerFrameUBO);
    if (m_PerDrawUBO) glDeleteBuffers(1, &m_PerDrawUBO);
    if (m_ForwardProgram) glDeleteProgram(m_ForwardProgram);
    if (m_DepthPrepassProgram) glDeleteProgram(m_DepthPrepassProgram);
    if (m_ShadowProgram) glDeleteProgram(m_ShadowProgram);
}

std::string Renderer::LoadShaderSource(const char* filename) {
    std::string path = GetShaderPath(filename);
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    
    std::stringstream buffer;
    std::string line;
    while (std::getline(file, line)) {
        // Simple #include handling for Common.glsl
        if (line.find("#include") != std::string::npos) {
            size_t start = line.find('"');
            size_t end = line.rfind('"');
            if (start != std::string::npos && end != std::string::npos && start < end) {
                std::string includeName = line.substr(start + 1, end - start - 1);
                std::string includePath = GetShaderPath(includeName.c_str());
                std::ifstream includeFile(includePath);
                if (includeFile.is_open()) {
                    std::string includeLine;
                    while (std::getline(includeFile, includeLine)) {
                        buffer << includeLine << "\n";
                    }
                    continue;
                }
            }
        }
        buffer << line << "\n";
    }
    return buffer.str();
}

GLuint Renderer::LoadAndCompileShader(const char* filename, GLenum shaderType) {
    std::string source = LoadShaderSource(filename);
    const char* sourceCStr = source.c_str();
    
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    
    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::string shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        throw std::runtime_error("Shader compilation failed (" + std::string(filename) + "): " + infoLog);
    }
    
    return shader;
}

GLuint Renderer::CreateShaderProgram(const char* vsFile, const char* fsFile) {
    GLuint vs = LoadAndCompileShader(vsFile, GL_VERTEX_SHADER);
    GLuint fs = LoadAndCompileShader(fsFile, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    
    // Check link status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        throw std::runtime_error("Shader program linking failed: " + std::string(infoLog));
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    return program;
}

void Renderer::CreateShaderPrograms() {
    m_ForwardProgram = CreateShaderProgram("Forward.vs.glsl", "Forward.ps.glsl");
    m_DepthPrepassProgram = CreateShaderProgram("DepthPrepass.vs.glsl", "DepthPrepass.ps.glsl");
    m_ShadowProgram = CreateShaderProgram("Shadow.vs.glsl", "Shadow.ps.glsl");
    
    std::cout << "Shader programs created successfully" << std::endl;
}

void Renderer::CreateCubeGeometry() {
    // Cube vertices with normals and colors
    Vertex vertices[] = {
        // Front face (red-ish)
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.3f, 0.3f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.3f, 0.3f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.3f, 0.3f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.3f, 0.3f, 1.0f}},
        
        // Back face (green-ish)
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.3f, 1.0f, 0.3f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.3f, 1.0f, 0.3f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.3f, 1.0f, 0.3f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.3f, 1.0f, 0.3f, 1.0f}},
        
        // Top face (blue-ish)
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.3f, 1.0f, 1.0f}},
        
        // Bottom face (yellow-ish)
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.3f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.3f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.3f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.3f, 1.0f}},
        
        // Right face (magenta-ish)
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.3f, 1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.3f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.3f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.3f, 1.0f, 1.0f}},
        
        // Left face (cyan-ish)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.3f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.3f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.3f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.3f, 1.0f, 1.0f, 1.0f}},
    };
    
    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };
    
    m_IndexCount = 36;
    
    // Create VAO
    glGenVertexArrays(1, &m_CubeVAO);
    glBindVertexArray(m_CubeVAO);
    
    // Create VBO
    glGenBuffers(1, &m_CubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(2);
    
    // Create IBO
    glGenBuffers(1, &m_CubeIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_CubeIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0);
    
    std::cout << "Cube geometry created" << std::endl;
}

void Renderer::BeginFrame() {
    m_Stats = RenderStats();
}

void Renderer::SetPerFrameConstants(const PerFrameConstants& constants) {
    m_PerFrameConstants = constants;
    
    // Update UBO
    glBindBuffer(GL_UNIFORM_BUFFER, m_PerFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameConstants), &constants);
}

void Renderer::DrawCube(const glm::mat4& worldMatrix, const glm::vec4& color) {
    PerDrawConstants perDraw;
    perDraw.WorldMatrix = worldMatrix;
    perDraw.MaterialIndex = 0;
    
    glBindBuffer(GL_UNIFORM_BUFFER, m_PerDrawUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerDrawConstants), &perDraw);
    
    glBindVertexArray(m_CubeVAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    
    m_Stats.DrawCount++;
    m_Stats.TriangleCount += m_IndexCount / 3;
}

void Renderer::RenderShadowPass(ECSWorld* world) {
    if (!m_ShadowsEnabled || !m_ShadowMap) {
        return;
    }
    
    // Bind shadow framebuffer
    m_ShadowMap->BeginShadowPass();
    
    // Use shadow shader program
    glUseProgram(m_ShadowProgram);
    
    // Render all renderable entities
    auto& registry = world->GetRegistry();
    auto view = registry.view<Transform, Renderable>();
    
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& renderable = view.get<Renderable>(entity);
        
        if (!renderable.Visible) {
            m_Stats.CulledCount++;
            continue;
        }
        
        // Update per-draw constants
        PerDrawConstants perDraw;
        perDraw.WorldMatrix = transform.GetWorldMatrix();
        perDraw.MaterialIndex = 0;
        
        glBindBuffer(GL_UNIFORM_BUFFER, m_PerDrawUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerDrawConstants), &perDraw);
        
        // Draw
        glBindVertexArray(m_CubeVAO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
        
        m_Stats.DrawCount++;
        m_Stats.TriangleCount += m_IndexCount / 3;
    }
    
    m_ShadowMap->EndShadowPass();
}

void Renderer::RenderScene(ECSWorld* world, bool enableDepthPrepass, bool enableShadows) {
    auto& registry = world->GetRegistry();
    
    // Depth prepass (optional)
    if (enableDepthPrepass) {
        glUseProgram(m_DepthPrepassProgram);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        
        auto view = registry.view<Transform, Renderable>();
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            auto& renderable = view.get<Renderable>(entity);
            
            if (!renderable.Visible) continue;
            
            PerDrawConstants perDraw;
            perDraw.WorldMatrix = transform.GetWorldMatrix();
            perDraw.MaterialIndex = 0;
            
            glBindBuffer(GL_UNIFORM_BUFFER, m_PerDrawUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerDrawConstants), &perDraw);
            
            glBindVertexArray(m_CubeVAO);
            glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
        }
        
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_EQUAL);
    }
    
    // Forward pass
    glUseProgram(m_ForwardProgram);
    
    // Bind shadow map if shadows are enabled
    if (enableShadows && m_ShadowMap) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMap->GetDepthTexture());
        GLint shadowMapLoc = glGetUniformLocation(m_ForwardProgram, "uShadowMap");
        if (shadowMapLoc >= 0) {
            glUniform1i(shadowMapLoc, 0);
        }
    }
    
    auto view = registry.view<Transform, Renderable>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& renderable = view.get<Renderable>(entity);
        
        if (!renderable.Visible) {
            m_Stats.CulledCount++;
            continue;
        }
        
        // Update per-draw constants
        PerDrawConstants perDraw;
        perDraw.WorldMatrix = transform.GetWorldMatrix();
        perDraw.MaterialIndex = 0;
        
        glBindBuffer(GL_UNIFORM_BUFFER, m_PerDrawUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerDrawConstants), &perDraw);
        
        // Draw
        glBindVertexArray(m_CubeVAO);
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
        
        m_Stats.DrawCount++;
        m_Stats.TriangleCount += m_IndexCount / 3;
    }
    
    if (enableDepthPrepass) {
        glDepthFunc(GL_LESS);
    }
}

} // namespace Henky3D
