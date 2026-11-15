#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

class MainApplication {
public:
    void run();

private:
    void initVulkan();
    void mainLoop();
    void cleanup();

    void initWindow();


    GLFWwindow* m_window = nullptr;
};