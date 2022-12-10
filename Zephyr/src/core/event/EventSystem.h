#pragma once
#include "pch.h"

namespace Zephyr
{

    enum class EventType
    {
        None = 0,

        WindowCreate,
        WindowClose,
        WindowResize,
        WindowMinimize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,

        KeyPressed,
        KeyReleased,
        KeyTyped,

        MouseMoved,
        MouseScrolled,
        MouseButtonPressed,
        MouseButtonReleased
    };
    
#define ZEPHYR_EVENT_TYPE(type) \
    static EventType   GetStaticType() { return EventType::type; } \
    static std::string GetStaticName() { return #type; } \
    EventType          GetEventType() override { return GetStaticType(); } \
    std::string        GetEventName() override { return #type; }

    class Event
    {
    public:
        bool m_Handled = false;

        virtual ~Event()                   = default;
        virtual EventType   GetEventType() = 0;
        virtual std::string GetEventName() = 0;
    };

    // TODO: support unregistration
    class EventCenter
    {
    public:
        template<typename T>
        using EventFn       = std::function<bool(T&)>;
        using EventCallback = std::function<bool(Event&)>;

        static void         Init();
        static void         Shutdown();
        static EventCenter* Get();

        template<typename T>
        static void Register(EventFn<T>&& handler)
        {
            static_assert(std::is_base_of_v<Event, T>);

            auto  eventCenter = Get();
            auto iter        = eventCenter->m_EventCallbacks.find(T::GetStaticType());

            auto cb = [handler](Event& event) {
                T& casted = static_cast<T&>(event);
                return handler(casted);
            };

            if (iter == eventCenter->m_EventCallbacks.end())
            {
                eventCenter->m_EventCallbacks.insert({T::GetStaticType(), {cb}});
            }
            else
            {
                iter->second.push_back(cb);
            }
        }

        template<typename T, typename... U>
        static void Dispatch(U&&... args)
        {
            static_assert(std::is_base_of_v<Event, T>);

            T e(std::forward<U>(args)...);

            auto eventCenter = Get();

            auto iter = eventCenter->m_EventCallbacks.find(T::GetStaticType());

            if (iter == eventCenter->m_EventCallbacks.end())
            {
                return;
            }

            auto& callbackList = iter->second;

            for (auto& callback : callbackList)
            {
                auto consumed = callback(e);
                if (consumed)
                {
                    break;
                }
            }
        }

    private:
        EventCenter()  = default;
        ~EventCenter() = default;

        static EventCenter* s_EventCenter;
        static bool         s_Inited;

        std::unordered_map<EventType, std::vector<EventCallback>> m_EventCallbacks;
    };
} // namespace Zephyr