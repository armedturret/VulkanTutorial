#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

class MainApplication {
public:
    void run();

private:
    void initWindow();
    
    // vulkan init functions
    void initVulkan();
    void createInstance();

    // main loop functions
    void mainLoop();

    void cleanup();

    GLFWwindow* m_window = nullptr;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
};