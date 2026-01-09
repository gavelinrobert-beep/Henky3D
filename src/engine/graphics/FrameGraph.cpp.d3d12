#include "FrameGraph.h"
#include <algorithm>

namespace Henky3D {

FrameGraph::FrameGraph(GraphicsDevice* device)
    : m_Device(device) {
}

FrameGraph::~FrameGraph() {
}

void FrameGraph::AddPass(const std::string& name, std::function<void(ID3D12GraphicsCommandList*)> execute) {
    RenderPass pass;
    pass.Name = name;
    pass.Execute = execute;
    pass.Enabled = true;
    m_Passes.push_back(pass);
}

void FrameGraph::SetPassEnabled(const std::string& name, bool enabled) {
    auto it = std::find_if(m_Passes.begin(), m_Passes.end(),
        [&name](const RenderPass& pass) { return pass.Name == name; });
    
    if (it != m_Passes.end()) {
        it->Enabled = enabled;
    }
}

bool FrameGraph::IsPassEnabled(const std::string& name) const {
    auto it = std::find_if(m_Passes.begin(), m_Passes.end(),
        [&name](const RenderPass& pass) { return pass.Name == name; });
    
    return it != m_Passes.end() && it->Enabled;
}

void FrameGraph::Execute(ID3D12GraphicsCommandList* commandList) {
    for (const auto& pass : m_Passes) {
        if (pass.Enabled && pass.Execute) {
            pass.Execute(commandList);
        }
    }
}

void FrameGraph::Clear() {
    m_Passes.clear();
}

size_t FrameGraph::GetEnabledPassCount() const {
    return std::count_if(m_Passes.begin(), m_Passes.end(),
        [](const RenderPass& pass) { return pass.Enabled; });
}

} // namespace Henky3D
