#include "Window.h"
#include "core/event/EventSystem.h"
#include "core/event/KeyboardEvent.h"
#include "core/event/MouseEvent.h"

namespace Zephyr
{
    Window::Window(const WindowDescription& desc) : m_Description(desc) { 
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    }

    bool Window::Init() { 
        m_Window =
            glfwCreateWindow(m_Description.width, m_Description.height, m_Description.title.c_str(), nullptr, nullptr);
        
        if (m_Window == nullptr)
        {
            return false;
        }
        glfwGetCursorPos(m_Window, &m_LastCursorX, &m_LastCursorY);
        glfwSetWindowUserPointer(m_Window, this);

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            switch (action)
            {
                case GLFW_PRESS:
                    std::cout << "glfw key press\n";
                    EventCenter::Dispatch<KeyPressedEvent>((KeyCode)key, 0);
                    break;
                case GLFW_RELEASE:
                    EventCenter::Dispatch<KeyReleasedEvent>((KeyCode)key);
                    break;
                case GLFW_REPEAT:
                    std::cout << "glfw key repeat\n";
                    EventCenter::Dispatch<KeyPressedEvent>((KeyCode)key, 1);
                    break;
            }
        });

        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
            switch (action)
            {
                case GLFW_PRESS:
                    EventCenter::Dispatch<MouseButtonPressedEvent>((MouseButton)button);
                    break;
                case GLFW_RELEASE:
                    EventCenter::Dispatch<MouseButtonReleasedEvent>((MouseButton)button);
                    break;
            }
        });

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPosition, double yPosition) {
            auto self = (Window*)glfwGetWindowUserPointer(window);

            double diffX = xPosition - self->m_LastCursorX;
            double diffY = yPosition - self->m_LastCursorY;

            self->m_LastCursorX = xPosition;
            self->m_LastCursorY = yPosition;
            EventCenter::Dispatch<MouseMovedEvent>(xPosition, yPosition, diffX, diffY);
        });

        return true;
    }

    HWND Window::GetNativeWindowHandleWin32() { 
        return glfwGetWin32Window(m_Window);
    }

    void Window::PollEvents() { 
        glfwPollEvents();

        if (glfwWindowShouldClose(m_Window))
        {
            m_ShouldClose = true;
            EventCenter::Dispatch<KeyPressedEvent>(KeyCode::Escape, 1);
        }
    }

    Window::~Window() { glfwDestroyWindow(m_Window); }
} // namespace Zephyr