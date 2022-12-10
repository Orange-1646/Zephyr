#pragma once
#include "EventSystem.h"

namespace Zephyr
{
    enum class MouseButton : uint16_t
    {
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Left    = Button0,
        Right   = Button1,
        Middle  = Button2
    };

    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(double x, double y, double diffX, double diffY) :
            m_X(x), m_Y(y), m_DiffX(diffX), m_DiffY(diffY) {};
        ~MouseMovedEvent() = default;

        inline double GetX() { return m_X; }
        inline double GetY() { return m_Y; }
        inline double GetDiffX() { return m_DiffX; }
        inline double GetDiffY() { return m_DiffY; }

        ZEPHYR_EVENT_TYPE(MouseMoved)

    private:
        double m_X;
        double m_Y;
        double m_DiffX;
        double m_DiffY;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {};
        ~MouseScrolledEvent() = default;

        inline float GetX() { return m_XOffset; }
        inline float GetY() { return m_YOffset; }

        ZEPHYR_EVENT_TYPE(MouseScrolled)

    private:
        float m_XOffset;
        float m_YOffset;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseButtonEvent(MouseButton button) : m_Button(button) {}
        virtual ~MouseButtonEvent() = default;

        inline MouseButton GetButton() { return m_Button; }

    private:
        MouseButton m_Button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseButton button) : MouseButtonEvent(button) {}
        ~MouseButtonPressedEvent() override = default;

        ZEPHYR_EVENT_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseButton button) : MouseButtonEvent(button) {}
        ~MouseButtonReleasedEvent() override = default;

        ZEPHYR_EVENT_TYPE(MouseButtonReleased)
    };
} // namespace Zephyr