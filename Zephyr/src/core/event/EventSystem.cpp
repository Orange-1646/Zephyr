#include "EventSystem.h"

namespace Zephyr
{
    EventCenter* EventCenter::s_EventCenter = nullptr;
    bool         EventCenter::s_Inited      = false;

    void EventCenter::Init()
    {
        if (s_Inited)
        {
            return;
        }
        s_Inited      = true;
        s_EventCenter = new EventCenter();
    }

    void EventCenter::Shutdown()
    {
        if (s_Inited)
        {
            delete s_EventCenter;
        }
    }

    EventCenter* EventCenter::Get()
    {
        assert(s_Inited);
        return s_EventCenter;
    }
} // namespace Zephyr