#pragma once
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "pch.h"

namespace Zephyr
{
    struct WindowDescription
    {
        uint32_t width;
        uint32_t height;
        std::string title           = "";
        bool     allowResize     = false;
        bool     allowFullscreen = false;
    };

    class Window
    {
    public:
        Window(const WindowDescription& desc);
        ~Window();
        bool Init();
        HWND GetNativeWindowHandleWin32();

        inline bool ShouldClose() { return m_ShouldClose; }

        void GetFramebufferSize(uint32_t* width, uint32_t* height) {
            int nWidth, nHeight;
            glfwGetFramebufferSize(m_Window, &nWidth, &nHeight);
            *width  = nWidth;
            *height = nHeight;
        }

        void WaitEvents() { glfwWaitEvents(); }

        void PollEvents();
    private:
        WindowDescription m_Description;
        GLFWwindow*       m_Window;
        void* m_NativeWindow;

        bool m_ShouldClose = false;

        // unused
        uint32_t m_CurrentWidth;
        uint32_t m_CurrentHeight;

        double m_LastCursorX = 0.;
        double m_LastCursorY = 0.;

        bool m_MoveKeyPressedX = false;
        bool m_MoveKeyPressedY = false;
    };
}