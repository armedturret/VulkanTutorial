#include "MainApplication.hpp"

#include <iostream>
#include <stdexcept>
#include <algorithm>

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
    createInstance();
}

void MainApplication::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "Baby's First Vulkan App",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "My Car",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = vk::ApiVersion14
    };

    // verify glfw supports vulkan
    int vulkanSupported = glfwVulkanSupported();
    if (vulkanSupported == GLFW_FALSE)
    {
        throw runtime_error("GLFW failed to find vulkan loader!");
    }

    // grab the glfw extensions needed to initialize the vulkan instance
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // check if the extensions are valid and supported
    vector<vk::ExtensionProperties> extensionProperties = m_context.enumerateInstanceExtensionProperties();

    cout << "Vulkan extension supported by device:" << endl;

    for (const auto& extension : extensionProperties)
    {
        cout << "\t" << extension.extensionName << endl;
    }

    cout << "Checking if required extensions are supported..." << endl;
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        auto glfwExtension = glfwExtensions[i];

        cout << "\t" << glfwExtension << " ... ";

        if (ranges::none_of(extensionProperties,
            [glfwExtension]
            (const auto& extensionProperty) {
                return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
            }))
        {
            throw runtime_error("Required extension not supported" + string(glfwExtension));
        }
        cout << "OK" << endl;
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions
    };

    m_instance = vk::raii::Instance(m_context, createInfo);
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
    cout << "Initializing glfw..." << endl;
    glfwInit();

    cout << "Initializing window..." << endl;
    // Disable GLFW creating an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Disable resizable windows (for now)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Baby's First Vulkan App", nullptr, nullptr);
}