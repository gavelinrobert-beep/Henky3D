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
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <memory>

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
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
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

        // Get SRV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        
        ID3D12DescriptorHeap* srvHeap = nullptr;
        if (FAILED(m_Device->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap)))) {
            throw std::runtime_error("Failed to create ImGui descriptor heap");
        }

        ImGui_ImplWin32_Init(m_Window->GetHandle());
        ImGui_ImplDX12_Init(
            m_Device->GetDevice(),
            GraphicsDevice::FrameCount,
            GraphicsDevice::BackBufferFormat,
            srvHeap,
            srvHeap->GetCPUDescriptorHandleForHeapStart(),
            srvHeap->GetGPUDescriptorHandleForHeapStart()
        );
        
        m_ImGuiSRVHeap = srvHeap;
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
            const float maxPitch = DirectX::XM_PIDIV2 - 0.01f;
            if (camera.Pitch > maxPitch) camera.Pitch = maxPitch;
            if (camera.Pitch < -maxPitch) camera.Pitch = -maxPitch;

            // Movement
            float moveSpeed = camera.MoveSpeed * deltaTime;
            DirectX::XMVECTOR forward = DirectX::XMVectorSet(
                sinf(camera.Yaw), 0.0f, cosf(camera.Yaw), 0.0f
            );
            DirectX::XMVECTOR right = DirectX::XMVectorSet(
                cosf(camera.Yaw), 0.0f, -sinf(camera.Yaw), 0.0f
            );
            DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

            DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&camera.Position);

            if (Input::IsKeyDown('W')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(forward, moveSpeed));
            if (Input::IsKeyDown('S')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(forward, -moveSpeed));
            if (Input::IsKeyDown('A')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(right, -moveSpeed));
            if (Input::IsKeyDown('D')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(right, moveSpeed));
            if (Input::IsKeyDown('E')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(up, moveSpeed));
            if (Input::IsKeyDown('Q')) pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(up, -moveSpeed));

            DirectX::XMStoreFloat3(&camera.Position, pos);
            camera.UpdateTargetFromAngles();
        }
    }

    void Render(const FPSCounter& fpsCounter) {
        m_Device->BeginFrame();
        m_Renderer->BeginFrame();
        
        auto commandList = m_Device->GetCommandList();
        
        // Transition to render target with explicit state tracking
        m_Device->TransitionBackBufferToRenderTarget(commandList);

        // Clear
        auto rtv = m_Device->GetCurrentRTV();
        auto dsv = m_Device->GetDSV();
        const float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Set render targets
        commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        // Set viewport and scissor
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_Window->GetWidth());
        viewport.Height = static_cast<float>(m_Window->GetHeight());
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor = {};
        scissor.right = m_Window->GetWidth();
        scissor.bottom = m_Window->GetHeight();
        commandList->RSSetScissorRects(1, &scissor);

        // Setup per-frame constants
        if (m_ECS->HasComponent<Camera>(m_CameraEntity)) {
            auto& camera = m_ECS->GetComponent<Camera>(m_CameraEntity);
            camera.AspectRatio = static_cast<float>(m_Window->GetWidth()) / static_cast<float>(m_Window->GetHeight());

            // Get directional light from scene
            DirectX::XMFLOAT3 lightDirection = { 0.5f, -1.0f, 0.3f };
            DirectX::XMFLOAT4 lightColor = { 1.0f, 1.0f, 0.9f, 1.0f };
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
            DirectX::XMFLOAT3 sceneBoundsMin = { -5.0f, -5.0f, -5.0f };
            DirectX::XMFLOAT3 sceneBoundsMax = { 5.0f, 5.0f, 5.0f };
            DirectX::XMMATRIX lightViewProj = ShadowMap::ComputeLightViewProjection(
                lightDirection, sceneBoundsMin, sceneBoundsMax);

            PerFrameConstants perFrameConstants;
            DirectX::XMMATRIX view = camera.GetViewMatrix();
            DirectX::XMMATRIX projection = camera.GetProjectionMatrix();
            
            DirectX::XMStoreFloat4x4(&perFrameConstants.ViewMatrix, DirectX::XMMatrixTranspose(view));
            DirectX::XMStoreFloat4x4(&perFrameConstants.ProjectionMatrix, DirectX::XMMatrixTranspose(projection));
            DirectX::XMStoreFloat4x4(&perFrameConstants.ViewProjectionMatrix, DirectX::XMMatrixTranspose(view * projection));
            DirectX::XMStoreFloat4x4(&perFrameConstants.LightViewProjectionMatrix, DirectX::XMMatrixTranspose(lightViewProj));
            perFrameConstants.CameraPosition = DirectX::XMFLOAT4(camera.Position.x, camera.Position.y, camera.Position.z, 1.0f);
            perFrameConstants.LightDirection = DirectX::XMFLOAT4(lightDirection.x, lightDirection.y, lightDirection.z, 0.0f);
            perFrameConstants.LightColor = lightColor;
            perFrameConstants.AmbientColor = DirectX::XMFLOAT4(0.2f, 0.2f, 0.25f, 1.0f);
            perFrameConstants.Time = m_TotalTime;
            perFrameConstants.DeltaTime = m_DeltaTime;
            perFrameConstants.ShadowBias = m_ShadowBias;
            perFrameConstants.ShadowsEnabled = m_ShadowsEnabled ? 1.0f : 0.0f;

            m_Renderer->SetPerFrameConstants(perFrameConstants);

            // Render shadow pass if enabled
            if (m_ShadowsEnabled) {
                m_Renderer->RenderShadowPass(m_ECS.get());
            }

            // Render scene
            m_Renderer->RenderScene(m_ECS.get(), m_DepthPrepassEnabled, m_ShadowsEnabled);
        }

        // Render ImGui
        RenderImGui(fpsCounter);

        // Transition to present
        m_Device->TransitionBackBufferToPresent(commandList);

        m_Device->EndFrame();
    }

    void RenderImGui(const FPSCounter& fpsCounter) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
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
        
        auto commandList = m_Device->GetCommandList();
        commandList->SetDescriptorHeaps(1, &m_ImGuiSRVHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    void OnResize() {
        if (m_Device) {
            m_Device->WaitForGPU();
            ImGui_ImplDX12_InvalidateDeviceObjects();
            m_Device->ResizeBuffers(m_Window->GetWidth(), m_Window->GetHeight());
            ImGui_ImplDX12_CreateDeviceObjects();
        }
    }

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<GraphicsDevice> m_Device;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<ECSWorld> m_ECS;
    
    ID3D12DescriptorHeap* m_ImGuiSRVHeap = nullptr;
    entt::entity m_CameraEntity;
    bool m_Running = true;
    bool m_CameraControlEnabled = false;
    bool m_DepthPrepassEnabled = true;
    bool m_ShadowsEnabled = true;
    float m_ShadowBias = 0.005f;
    float m_TotalTime = 0.0f;
    float m_DeltaTime = 0.0f;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    try {
        Application app;
        app.Run();
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR);
        return -1;
    }
    return 0;
}
