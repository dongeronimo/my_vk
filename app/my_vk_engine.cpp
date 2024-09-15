#include "app/window.h"

int main(int argc, char** argv)
{
    glfwInit();
    app::Window mainWindow(1024, 762);
    mainWindow.MainLoop();
}