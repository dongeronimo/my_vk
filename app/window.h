#pragma once
#include <GLFW/glfw3.h>
#include "ui/mouse_event.h"

namespace app {
    /// <summary>
    /// Encapsulates a window. I'm using GLFW so much of the functionaly
    /// is done by it.
    /// </summary>
    class Window {
    public:
        /// <summary>
        /// Creates a resizable window without decoration 
        /// </summary>
        /// <param name="w"></param>
        /// <param name="h"></param>
        Window(uint32_t w, uint32_t h);
        ~Window();
        GLFWwindow* GetWindow()const { return mWindow; }
        /// <summary>
        /// The main event loop. This function blocks until 
        /// finished.
        /// </summary>
        void MainLoop();
        const ui::MouseState& GetMouseState()const;

    private:
        GLFWwindow* mWindow = nullptr;
    };
}