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


const std::vector<const char*> requiredDeviceExtensions = { 
    vk::KHRSwapchainExtensionName 
};

class MainApplication {
public:
    void run();

private:
    void initWindow();

    // vulkan init functions
    void initVulkan();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    std::vector<const char*> getRequiredValidationLayers();
    std::vector<const char*> getRequiredInstanceExtensions();

    // returns if a queue family has all the properties we expect
    bool isValidQueueFamily(
        vk::raii::PhysicalDevice physicalDevice,
        const std::vector<vk::QueueFamilyProperties>& qfpList,
        uint32_t qfpIndex
    );

    // main loop functions
    void mainLoop();

    void cleanup();

    GLFWwindow* m_window = nullptr;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::PhysicalDevice m_physicalDevice = nullptr;
    // logical device tied to the physical one
    vk::raii::Device m_device = nullptr;
    vk::raii::Queue m_graphicsQueue = nullptr;
    // window surface to render to
    vk::raii::SurfaceKHR m_surface = nullptr;
};