#pragma once
#include "rhi/Handle.h"
#include "rhi/RHIRenderTarget.h"

namespace Zephyr
{
    class FrameGraph;

    struct PassRenderTarget
    {
        Handle<RHIRenderTarget> rt;
    };

    class FrameGraphPassBase
    {
    public:
        FrameGraphPassBase(const std::string& name) : m_Name(name) {}
        virtual ~FrameGraphPassBase()           = default;
        virtual void Execute(FrameGraph* graph, PassRenderTarget rt) = 0;

        std::string GetName() { return m_Name; }

    private:
        std::string m_Name;
    };

    template<typename Data, typename Executor>
    class FrameGraphPass : public FrameGraphPassBase
    {
    public:
        FrameGraphPass(const std::string& name, Executor&& execute) :
            FrameGraphPassBase(name), m_Execute(std::move(execute))
        {}
        ~FrameGraphPass() = default;

        inline Data* GetData() { return &m_Data; }

        void Execute(FrameGraph* graph, PassRenderTarget rt) override { m_Execute(graph, &m_Data, rt); }

    private:
        Data     m_Data;
        Executor m_Execute;
    };
} // namespace Zephyr