#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <vector>

constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

#ifdef NDEBUG
constexpr bool USE_VALIDATION_LAYERS = false;
#else
constexpr bool USE_VALIDATION_LAYERS = true;
#endif

// Validation layers to use for debugging
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation" // comes with the lunarg vulkan sdk
};

class MainApplication {
public:
    void run();

private:
    void initWindow();

    // vulkan init functions
    void initVulkan();
    void createInstance();
    void pickPhysicalDevice();
    std::vector<const char*> getRequiredValidationLayers();
    std::vector<const char*> getRequiredInstanceExtensions();

    // main loop functions
    void mainLoop();

    void cleanup();

    GLFWwindow* m_window = nullptr;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::PhysicalDevice m_physicalDevice = nullptr;
};