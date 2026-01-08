#include "engine/core/Window.h"
#include "engine/core/Timer.h"
#include "engine/input/Input.h"
#include "engine/graphics/GraphicsDevice.h"
#include "engine/graphics/Renderer.h"
#include "engine/graphics/ConstantBuffers.h"
#include "engine/graphics/ShadowMap.h"
#include "engine/ecs/ECSWorld.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/TransformSystem.h"
#include "engine/ecs/CullingSystem.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <iostream>

using namespace Henky3D;

class Application {
public:
    Application() {
        m_Window = std::make_unique<Window>("Henky3D Engine", 1280, 720);
        m_Device = std::make_unique<GraphicsDevice>(m_Window.get());
        m_Renderer = std::make_unique<Renderer>(m_Device.get());
        m_ECS = std::make_unique<ECSWorld>();
        
        InitializeImGui();
        InitializeScene();
        
        Input::Initialize(m_Window->GetHandle());
        
        m_Window->SetEventCallback([this]() {
            OnResize();
        });
    }

    ~Application() {
        m_Device->WaitForGPU();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Run() {
        Timer timer;
        FPSCounter fpsCounter;
        m_TotalTime = 0.0f;

        while (m_Running) {
            if (!m_Window->ProcessMessages()) {
                m_Running = false;
                break;
            }

            m_DeltaTime = timer.GetDeltaTime();
            fpsCounter.Update(m_DeltaTime);
            m_TotalTime += m_DeltaTime;
            
            Input::Update();
            Update(m_DeltaTime);
            Render(fpsCounter);
        }
    }

private:
    void InitializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(m_Window->GetHandle(), true);
        ImGui_ImplOpenGL3_Init("#version 460 core");
    }

    void InitializeScene() {
        // Create camera entity
        auto cameraEntity = m_ECS->CreateEntity();
        auto& camera = m_ECS->AddComponent<Camera>(cameraEntity);
        camera.AspectRatio = static_cast<float>(m_Window->GetWidth()) / static_cast<float>(m_Window->GetHeight());
        m_CameraEntity = cameraEntity;

        // Create light entity
        auto lightEntity = m_ECS->CreateEntity();
        auto& light = m_ECS->AddComponent<Light>(lightEntity);
        light.LightType = Light::Type::Directional;
        light.Direction = { 0.0f, -1.0f, 0.5f };
        light.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Create a spinning cube at the center
        auto cubeEntity = m_ECS->CreateEntity();
        auto& transform = m_ECS->AddComponent<Transform>(cubeEntity);
        transform.Position = { 0.0f, 0.0f, 0.0f };
        m_ECS->AddComponent<Renderable>(cubeEntity);
        m_ECS->AddComponent<BoundingBox>(cubeEntity);

        // Create a second cube to the right
        auto cube2Entity = m_ECS->CreateEntity();
        auto& transform2 = m_ECS->AddComponent<Transform>(cube2Entity);
        transform2.Position = { 2.0f, 0.0f, 0.0f };
        auto& renderable2 = m_ECS->AddComponent<Renderable>(cube2Entity);
        renderable2.Color = { 0.3f, 1.0f, 0.3f, 1.0f };
        m_ECS->AddComponent<BoundingBox>(cube2Entity);

        // Create a third cube to the left
        auto cube3Entity = m_ECS->CreateEntity();
        auto& transform3 = m_ECS->AddComponent<Transform>(cube3Entity);
        transform3.Position = { -2.0f, 0.0f, 0.0f };
        auto& renderable3 = m_ECS->AddComponent<Renderable>(cube3Entity);
        renderable3.Color = { 0.3f, 0.3f, 1.0f, 1.0f };
        m_ECS->AddComponent<BoundingBox>(cube3Entity);
    }

    void Update(float deltaTime) {
        m_ECS->Update(deltaTime);
        UpdateCamera(deltaTime);
        UpdateScene(deltaTime);
        
        // Update transform hierarchy
        TransformSystem::UpdateTransforms(m_ECS.get());
    }

    void UpdateScene(float deltaTime) {
        // Rotate the first cube
        auto view = m_ECS->GetRegistry().view<Transform, Renderable>();
        int cubeIndex = 0;
        for (auto entity : view) {
            auto& transform = view.get<Transform>(entity);
            if (cubeIndex == 0) {
                transform.Rotation.y += deltaTime;
                transform.Rotation.x += deltaTime * 0.5f;
                transform.MarkDirty();
            }
            cubeIndex++;
        }
    }

    void UpdateCamera(float deltaTime) {
        if (!m_ECS->HasComponent<Camera>(m_CameraEntity)) return;
        
        auto& camera = m_ECS->GetComponent<Camera>(m_CameraEntity);

        // Camera fly controls
        if (m_CameraControlEnabled) {
            // Mouse look
            float deltaX = Input::GetMouseDeltaX();
            float deltaY = Input::GetMouseDeltaY();
            
            camera.Yaw += deltaX * camera.LookSpeed;
            camera.Pitch -= deltaY * camera.LookSpeed;
            
            // Clamp pitch
            const float maxPitch = glm::half_pi<float>() - 0.01f;
            if (camera.Pitch > maxPitch) camera.Pitch = maxPitch;
            if (camera.Pitch < -maxPitch) camera.Pitch = -maxPitch;

            // Movement
            float moveSpeed = camera.MoveSpeed * deltaTime;
            glm::vec3 forward = glm::vec3(
                sinf(camera.Yaw), 0.0f, cosf(camera.Yaw)
            );
            glm::vec3 right = glm::vec3(
                cosf(camera.Yaw), 0.0f, -sinf(camera.Yaw)
            );
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 pos = camera.Position;

            if (Input::IsKeyDown('W')) pos += forward * moveSpeed;
            if (Input::IsKeyDown('S')) pos -= forward * moveSpeed;
            if (Input::IsKeyDown('A')) pos -= right * moveSpeed;
            if (Input::IsKeyDown('D')) pos += right * moveSpeed;
            if (Input::IsKeyDown('E')) pos += up * moveSpeed;
            if (Input::IsKeyDown('Q')) pos -= up * moveSpeed;

            camera.Position = pos;
            camera.UpdateTargetFromAngles();
        }
    }

    void Render(const FPSCounter& fpsCounter) {
        m_Device->BeginFrame();
        m_Renderer->BeginFrame();
        
        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set viewport
        glViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());

        // Setup per-frame constants
        if (m_ECS->HasComponent<Camera>(m_CameraEntity)) {
            auto& camera = m_ECS->GetComponent<Camera>(m_CameraEntity);
            camera.AspectRatio = static_cast<float>(m_Window->GetWidth()) / static_cast<float>(m_Window->GetHeight());

            // Get directional light from scene
            glm::vec3 lightDirection = glm::vec3(0.5f, -1.0f, 0.3f);
            glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
            auto lightView = m_ECS->GetRegistry().view<Light>();
            for (auto entity : lightView) {
                auto& light = lightView.get<Light>(entity);
                if (light.LightType == Light::Type::Directional) {
                    lightDirection = light.Direction;
                    lightColor = light.Color;
                    break;
                }
            }
            
            // Compute light view-projection for shadows
            glm::vec3 sceneBoundsMin = glm::vec3(-5.0f, -5.0f, -5.0f);
            glm::vec3 sceneBoundsMax = glm::vec3(5.0f, 5.0f, 5.0f);
            glm::mat4 lightViewProj = ShadowMap::ComputeLightViewProjection(
                lightDirection, sceneBoundsMin, sceneBoundsMax);

            PerFrameConstants perFrameConstants;
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 projection = camera.GetProjectionMatrix();
            
            perFrameConstants.ViewMatrix = view;
            perFrameConstants.ProjectionMatrix = projection;
            perFrameConstants.ViewProjectionMatrix = projection * view;
            perFrameConstants.LightViewProjectionMatrix = lightViewProj;
            perFrameConstants.CameraPosition = glm::vec4(camera.Position, 1.0f);
            perFrameConstants.LightDirection = glm::vec4(lightDirection, 0.0f);
            perFrameConstants.LightColor = lightColor;
            perFrameConstants.AmbientColor = glm::vec4(0.2f, 0.2f, 0.25f, 1.0f);
            perFrameConstants.Time = m_TotalTime;
            perFrameConstants.DeltaTime = m_DeltaTime;
            perFrameConstants.ShadowBias = m_ShadowBias;
            perFrameConstants.ShadowsEnabled = m_ShadowsEnabled ? 1.0f : 0.0f;

            m_Renderer->SetPerFrameConstants(perFrameConstants);

            // Render shadow pass if enabled
            if (m_ShadowsEnabled) {
                m_Renderer->RenderShadowPass(m_ECS.get());
                
                // Reset viewport after shadow pass
                glViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
            }

            // Render scene
            m_Renderer->RenderScene(m_ECS.get(), m_DepthPrepassEnabled, m_ShadowsEnabled);
        }

        // Render ImGui
        RenderImGui(fpsCounter);

        m_Device->EndFrame();
    }

    void RenderImGui(const FPSCounter& fpsCounter) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main overlay window
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Henky3D Engine")) {
            ImGui::Text("FPS: %.1f", fpsCounter.GetFPS());
            ImGui::Text("Frame Time: %.2f ms", fpsCounter.GetFrameTime());
            
            ImGui::Separator();
            ImGui::Text("Rendering:");
            ImGui::Checkbox("Enable Depth Prepass", &m_DepthPrepassEnabled);
            m_Renderer->SetDepthPrepassEnabled(m_DepthPrepassEnabled);
            ImGui::Checkbox("Enable Shadows", &m_ShadowsEnabled);
            m_Renderer->SetShadowsEnabled(m_ShadowsEnabled);
            if (m_ShadowsEnabled) {
                ImGui::SliderFloat("Shadow Bias", &m_ShadowBias, 0.0f, 0.01f, "%.4f");
            }
            
            ImGui::Separator();
            ImGui::Text("Stats:");
            auto& stats = m_Renderer->GetStats();
            ImGui::Text("Draw Calls: %u", stats.DrawCount);
            ImGui::Text("Culled: %u", stats.CulledCount);
            ImGui::Text("Triangles: %u", stats.TriangleCount);
            
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::Checkbox("Enable Camera Control", &m_CameraControlEnabled);
            if (m_CameraControlEnabled) {
                ImGui::Text("WASD: Move");
                ImGui::Text("Q/E: Down/Up");
                ImGui::Text("Mouse: Look");
            }

            if (m_ECS->HasComponent<Camera>(m_CameraEntity)) {
                ImGui::Separator();
                auto& camera = m_ECS->GetComponent<Camera>(m_CameraEntity);
                ImGui::Text("Camera Position:");
                ImGui::DragFloat3("Pos", &camera.Position.x, 0.1f);
                ImGui::DragFloat("Move Speed", &camera.MoveSpeed, 0.1f, 0.1f, 100.0f);
                ImGui::DragFloat("Look Speed", &camera.LookSpeed, 0.0001f, 0.0001f, 0.01f);
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void OnResize() {
        if (m_Device) {
            m_Device->WaitForGPU();
            m_Device->ResizeBuffers(m_Window->GetWidth(), m_Window->GetHeight());
        }
    }

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<GraphicsDevice> m_Device;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ECSWorld> m_ECS;
    entt::entity m_CameraEntity;
    bool m_Running = true;
    bool m_CameraControlEnabled = false;
    bool m_DepthPrepassEnabled = true;
    bool m_ShadowsEnabled = true;
    float m_ShadowBias = 0.005f;
    float m_TotalTime = 0.0f;
    float m_DeltaTime = 0.0f;
};

int main(int argc, char** argv) {
    try {
        Application app;
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
