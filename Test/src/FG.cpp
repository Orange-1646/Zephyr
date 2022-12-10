#include "engine/Engine.h"
#include "framegraph/FrameGraph.h"

int main()
{
    using namespace Zephyr;

    auto engine = Engine::Create({1080, 640, DriverType::Vulkan, "FGTest", false, false, true});

    engine->Run([](Engine* enginen) {});
}