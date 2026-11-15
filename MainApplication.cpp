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

    // check if layers are supported
    vector<char const*> requiredLayers = getRequiredValidationLayers();
    vector<vk::LayerProperties> layerProperties = m_context.enumerateInstanceLayerProperties();
    cout << "Vulkan validation layers supported by device:" << endl;

    for (const auto& layer : layerProperties)
    {
        cout << "\t" << layer.layerName << endl;
    }
    cout << "Checking if required layers are supported..." << endl;
    for (auto& requiredLayer : requiredLayers)
    {
        cout << "\t" << requiredLayer << " ... ";

        if (ranges::none_of(layerProperties,
            [requiredLayer]
            (const auto& layerProperty) {
                return strcmp(layerProperty.layerName, requiredLayer) == 0;
            }))
        {
            throw runtime_error("Required layer not supported" + string(requiredLayer));
        }
        cout << "OK" << endl;
    }

    vector<char const*> requiredExtensions = getRequiredInstanceExtensions();
    vector<vk::ExtensionProperties> extensionProperties = m_context.enumerateInstanceExtensionProperties();
    cout << "Vulkan validation extensions supported by device:" << endl;

    for (const auto& extension : extensionProperties)
    {
        cout << "\t" << extension.extensionName << endl;
    }
    cout << "Checking if required extensions are supported..." << endl;
    for (auto& requiredExtension : requiredExtensions)
    {
        cout << "\t" << requiredExtension << " ... ";

        if (ranges::none_of(extensionProperties,
            [requiredExtension]
            (const auto& extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
            }))
        {
            throw runtime_error("Required extension not supported" + string(requiredExtension));
        }
        cout << "OK" << endl;
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    m_instance = vk::raii::Instance(m_context, createInfo);
}

std::vector<const char*> MainApplication::getRequiredValidationLayers()
{
    vector<char const*> requiredLayers;
    if (USE_VALIDATION_LAYERS)
    {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    return requiredLayers;
}

std::vector<const char*> MainApplication::getRequiredInstanceExtensions()
{
    // grab the glfw extensions needed to initialize the vulkan instance
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // check if the extensions are valid and supported
    vector<vk::ExtensionProperties> extensionProperties = m_context.enumerateInstanceExtensionProperties();

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
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