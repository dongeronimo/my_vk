#include "window.h"
#include "glm/glm.hpp"
/// <summary>
/// I need this global variable here bc glfwSetCursorPosCallback does not accept lamba scope capture.
/// </summary>
glm::vec2 gMousePosition;

namespace app {
    app::Window::Window(uint32_t w, uint32_t h)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//don't want to use opengl
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        mWindow = glfwCreateWindow(w, h, "Hello Vulkan", nullptr, nullptr);
        glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, double xpos, double ypos) {
            gMousePosition.x = static_cast<float>(xpos);
            gMousePosition.y = static_cast<float>(ypos);
        });
        glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int button, int action, int mods) {

        });
    }

    app::Window::~Window()
    {
        glfwDestroyWindow(mWindow);
    }
    void Window::MainLoop()
    {
        while (!glfwWindowShouldClose(mWindow))
        {
            glfwPollEvents();
            //TODO: draw the game objects

        }
    }

}
