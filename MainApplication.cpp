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
    pickPhysicalDevice();
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
    for (const auto& requiredLayer : requiredLayers)
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
    for (const auto& requiredExtension : requiredExtensions)
    {
        cout << "\t" << requiredExtension << " ... ";

        if (ranges::none_of(extensionProperties,
            [requiredExtension]
            (const auto& extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
            }
        ))
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

void MainApplication::pickPhysicalDevice()
{
    // pick a GPU to use for vulkan
    vector<vk::raii::PhysicalDevice> physicalDevices = m_instance.enumeratePhysicalDevices();

    if (physicalDevices.empty())
    {
        throw runtime_error("No Vulkan capable GPUs available!");
    }

    // keep track of the best scoring physical device
    vk::raii::PhysicalDevice bestDevice = nullptr;
    int bestScore = 0;

    cout << "Available GPUs:" << endl;
    for (const auto& physicalDevice : physicalDevices)
    {
        // overall device details
        vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
        // optional vulkan features supported
        vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();

        cout << "\t" << deviceProperties.deviceName << " ... ";

        if (deviceProperties.apiVersion < vk::ApiVersion13)
        {
            cout << "BAD (Vulkan API < 1.3)" << endl;
            continue;
        }

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        if (ranges::none_of(queueFamilies, 
            [](const auto& qfp) 
            { 
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags(0); 
            }
        ))
        {
            cout << "BAD (No valid queue family)" << endl;
            continue;
        }

        vector<const char*> requiredDeviceExtensions = { vk::KHRSwapchainExtensionName };
        auto deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
        if (ranges::any_of(requiredDeviceExtensions, 
            [&deviceExtensionProperties](const auto& requiredExtension)
            { 
                return ranges::none_of(deviceExtensionProperties, 
                    [requiredExtension](const auto& extension) {
                        return strcmp(extension.extensionName, requiredExtension) == 0; 
                    }
                ); 
            }
        ))
        {
            cout << "BAD (Doesn't support all device extension properties)" << endl;
            continue;
        }

        auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        if (!features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters)
        {
            cout << "BAD (Must support shader draw parameters)" << endl;
            continue;
        }
        if (!features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering)
        {

            cout << "BAD (Must support dynamic rendering)" << endl;
            continue;
        }
        if (!features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState)
        {
            cout << "BAD (Must support extended dynamic state)" << endl;
            continue;
        }

        int score = 0;
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            // dedicated gpu, REALLY good
            score += 1000;
        }

        // Maximum texture size
        score += deviceProperties.limits.maxImageDimension2D;

        cout << "OK (Score: " << score << ")" << endl;

        if (score > bestScore)
        {
            bestScore = score;
            bestDevice = physicalDevice;
        }
    }

    if (bestDevice == nullptr)
    {
        throw runtime_error("No suitable GPUs found!");
    }

    cout << "Using " << bestDevice.getProperties().deviceName << endl;
    m_physicalDevice = bestDevice;
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