#include "engine/core/Window.h"
#include "engine/core/Timer.h"
#include "engine/input/Input.h"
#include "engine/graphics/GraphicsDevice.h"
#include "engine/graphics/Renderer.h"
#include "engine/ecs/ECSWorld.h"
#include "engine/ecs/Components.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <wrl/client.h>
#include <memory>

using namespace Henky3D;
using Microsoft::WRL::ComPtr;

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

        while (m_Running) {
            if (!m_Window->ProcessMessages()) {
                m_Running = false;
                break;
            }

            float deltaTime = timer.GetDeltaTime();
            fpsCounter.Update(deltaTime);
            
            Input::Update();
            Update(deltaTime);
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
        m_Device->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap));

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
        m_ECS->AddComponent<Camera>(cameraEntity);
        m_CameraEntity = cameraEntity;

        // Create light entity
        auto lightEntity = m_ECS->CreateEntity();
        auto& light = m_ECS->AddComponent<Light>(lightEntity);
        light.LightType = Light::Type::Directional;
        light.Direction = { 0.0f, -1.0f, 0.5f };
        light.Color = { 1.0f, 1.0f, 1.0f, 1.0f };

        // Create a renderable entity
        auto entity = m_ECS->CreateEntity();
        m_ECS->AddComponent<Transform>(entity);
        m_ECS->AddComponent<Renderable>(entity);
    }

    void Update(float deltaTime) {
        m_ECS->Update(deltaTime);
        UpdateCamera(deltaTime);
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
        
        auto commandList = m_Device->GetCommandList();
        
        // Get current render target
        ComPtr<ID3D12Resource> renderTarget;
        m_Device->GetSwapChain()->GetBuffer(m_Device->GetBackBufferIndex(), IID_PPV_ARGS(&renderTarget));
        
        // Transition to render target
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderTarget.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        commandList->ResourceBarrier(1, &barrier);

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

        // Render triangle
        m_Renderer->RenderTriangle();

        // Render ImGui
        RenderImGui(fpsCounter);

        // Transition to present
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        m_Device->EndFrame();
    }

    void RenderImGui(const FPSCounter& fpsCounter) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main overlay window
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Henky3D Engine")) {
            ImGui::Text("FPS: %.1f", fpsCounter.GetFPS());
            ImGui::Text("Frame Time: %.2f ms", fpsCounter.GetFrameTime());
            
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
