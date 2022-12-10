#include "EventSystem.h"

namespace Zephyr
{
    enum class KeyCode : uint16_t
    {
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,

        Escape = 256
    };

    class KeyboardEvent : public Event
    {

    public:
        KeyboardEvent(KeyCode code) : m_KeyCode(code) {}
        virtual ~KeyboardEvent() override = default;

        inline KeyCode GetKeyCode() { return m_KeyCode; }

    protected:
        KeyCode m_KeyCode;
    };

    class KeyPressedEvent final : public KeyboardEvent
    {
    public:
        KeyPressedEvent(KeyCode code, uint32_t repeatCount) : KeyboardEvent(code), m_RepeatCount(repeatCount) {};
        inline uint32_t RepeatCount() { return m_RepeatCount; }
        ~KeyPressedEvent() override = default;

        inline uint32_t GetRepeatCount() { return m_RepeatCount; }

        ZEPHYR_EVENT_TYPE(KeyPressed)

    private:
        uint32_t m_RepeatCount = 0;
    };

    class KeyReleasedEvent final : public KeyboardEvent
    {
    public:
        KeyReleasedEvent(KeyCode code) : KeyboardEvent(code) {};
        ~KeyReleasedEvent() override = default;

        ZEPHYR_EVENT_TYPE(KeyReleased);
    };

    class KeyTypedEvent final : public KeyboardEvent
    {
    public:
        KeyTypedEvent(KeyCode code) : KeyboardEvent(code) {};
        ~KeyTypedEvent() override = default;

        ZEPHYR_EVENT_TYPE(KeyTyped);
    };
} // namespace Zephyr