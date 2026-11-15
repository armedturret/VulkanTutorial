#include "MainApplication.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;

void MainApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void MainApplication::initVulkan()
{
    cout << "Initializing vulkan..." << endl;
}

void MainApplication::mainLoop()
{
    cout << "Beginning main loop..." << endl;
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
    }
}

void MainApplication::cleanup()
{
    cout << "Cleaning up..." << endl;
    // We love RAII (no other vulkans should need this :))
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void MainApplication::initWindow()
{
    cout << "Initializing window..." << endl;
    // Disable GLFW creating an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Disable resizable windows (for now)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Baby's First Vulkan App", nullptr, nullptr);
}