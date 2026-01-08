#pragma once
#include "GraphicsDevice.h"
#include <d3d12.h>
#include <functional>
#include <vector>
#include <string>

namespace Henky3D {

// Forward declarations
class Renderer;
class ECSWorld;

// Represents a render pass in the frame graph
struct RenderPass {
    std::string Name;
    std::function<void(ID3D12GraphicsCommandList*)> Execute;
    bool Enabled = true;
};

// Lightweight frame graph for pass ordering and barrier management
class FrameGraph {
public:
    FrameGraph(GraphicsDevice* device);
    ~FrameGraph();

    // Add passes
    void AddPass(const std::string& name, std::function<void(ID3D12GraphicsCommandList*)> execute);
    
    // Enable/disable specific passes
    void SetPassEnabled(const std::string& name, bool enabled);
    bool IsPassEnabled(const std::string& name) const;
    
    // Execute all enabled passes
    void Execute(ID3D12GraphicsCommandList* commandList);
    
    // Clear all passes (typically called per-frame)
    void Clear();
    
    // Get statistics
    size_t GetPassCount() const { return m_Passes.size(); }
    size_t GetEnabledPassCount() const;

private:
    GraphicsDevice* m_Device;
    std::vector<RenderPass> m_Passes;
};

} // namespace Henky3D
