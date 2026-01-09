#include "FrameGraph.h"

namespace Henky3D {

FrameGraph::FrameGraph(GraphicsDevice* device)
    : m_Device(device) {
}

FrameGraph::~FrameGraph() {
}

void FrameGraph::AddPass(const std::string& name, std::function<void()> execute) {
    RenderPass pass;
    pass.Name = name;
    pass.Execute = execute;
    pass.Enabled = true;
    m_Passes.push_back(pass);
}

void FrameGraph::SetPassEnabled(const std::string& name, bool enabled) {
    for (auto& pass : m_Passes) {
        if (pass.Name == name) {
            pass.Enabled = enabled;
            return;
        }
    }
}

bool FrameGraph::IsPassEnabled(const std::string& name) const {
    for (const auto& pass : m_Passes) {
        if (pass.Name == name) {
            return pass.Enabled;
        }
    }
    return false;
}

void FrameGraph::Execute() {
    for (const auto& pass : m_Passes) {
        if (pass.Enabled && pass.Execute) {
            pass.Execute();
        }
    }
}

void FrameGraph::Clear() {
    m_Passes.clear();
}

size_t FrameGraph::GetEnabledPassCount() const {
    size_t count = 0;
    for (const auto& pass : m_Passes) {
        if (pass.Enabled) {
            count++;
        }
    }
    return count;
}

} // namespace Henky3D
