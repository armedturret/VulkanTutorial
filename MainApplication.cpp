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
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
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

void MainApplication::createSurface()
{
    cout << "Creating Vulkan surface..." << endl;
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface) != 0)
    {
        throw runtime_error("Failed to create Vulkan window surface!");
    }
    m_surface = vk::raii::SurfaceKHR(m_instance, surface);
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

        auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        bool validQueueAvailable = false;
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if (isValidQueueFamily(physicalDevice, queueFamilyProperties, qfpIndex))
            {
                validQueueAvailable = true;
                break;
            }
        }
        if(!validQueueAvailable)
        {
            cout << "BAD (No valid queue family)" << endl;
            continue;
        }

        // lets us render to a window
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

void MainApplication::createLogicalDevice()
{
    cout << "Creating logical device..." << endl;
    auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
    // find the index of the first graphics-capable queue family
    uint32_t qfpIndex = 0;
    for (qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if(isValidQueueFamily(m_physicalDevice, queueFamilyProperties, qfpIndex))
            break;
    }

    // Queue priority required even w/ just one queue
    float queuePriority = 0.5f;
    // Creates a queue family of one
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = qfpIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    // Request the previous verified features
    // StructureChain here does the heavily lifting of filling in the .pNext values
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {
            {}, // PhysicalDeviceFeatures2
            {.shaderDrawParameters = true}, // PhysicalDeviceVulkan11Features
            {.dynamicRendering = true}, // PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true} // PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(), // chain into requesting device features
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };
    m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);

    m_graphicsQueue = vk::raii::Queue(m_device, qfpIndex, 0);
}

void MainApplication::createSwapChain()
{
    assert(m_surface != nullptr);
    assert(m_physicalDevice != nullptr);
    assert(m_device != nullptr);
    cout << "Creating swap chain..." << endl;
    // choose swap chain properties based on available options
    vk::SurfaceCapabilitiesKHR surfaceCapabilities =
        m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
    m_swapchainExtent = chooseSwapExtent(surfaceCapabilities);
    uint32_t minImgCount = chooseSwapMinImageCount(surfaceCapabilities);

    auto availablePresentModes = m_physicalDevice.getSurfacePresentModesKHR( *m_surface);
    vk::PresentModeKHR presentMode = choosePresentMode(availablePresentModes);

    auto availableFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);
    m_swapchainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

    // actually create the thing
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = *m_surface,
        .minImageCount = minImgCount,
        .imageFormat = m_swapchainSurfaceFormat.format,
        .imageColorSpace = m_swapchainSurfaceFormat.colorSpace,
        .imageExtent = m_swapchainExtent,
        .imageArrayLayers = 1, // this isn't a stereoscopic app
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment, // simple color buffer for now
        .imageSharingMode = vk::SharingMode::eExclusive, // images can only be in one queue
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque, // window should be solid
        .presentMode = presentMode,
        .clipped = true, // cull if operation won't be visible
        .oldSwapchain = nullptr // used to specify the previous swap chain after a window refresh
    };

    m_swapchain = vk::raii::SwapchainKHR(m_device, swapchainCreateInfo);
    m_swapchainImages = m_swapchain.getImages();
}

vk::SurfaceFormatKHR MainApplication::chooseSwapSurfaceFormat(
    vector<vk::SurfaceFormatKHR> const& availableFormats)
{
    assert(!availableFormats.empty());
    const auto formatIt = ranges::find_if(availableFormats,
        [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && 
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );
    // If we found our ideal sRGB format, great, if not just pick the first one
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR MainApplication::choosePresentMode(
    vector <vk::PresentModeKHR> const &availablePresentModes)
{
    // Triple buffered vsync
    // While waiting on the refresh, keep writing to non-presented buffers
    // This results in reduced latency
    if (ranges::any_of(availablePresentModes, [](const auto val) { return val == vk::PresentModeKHR::eMailbox; }))
        return vk::PresentModeKHR::eMailbox;

    // FIFO MUST be available
    // This is classical vsync, do not attempt to swap buffers until the next buffer in the queue is available
    assert(ranges::any_of(availablePresentModes, [](const auto val) { return val == vk::PresentModeKHR::eFifo; }));
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D MainApplication::chooseSwapExtent(
    vk::SurfaceCapabilitiesKHR const& capabilities)
{
    // If currentExtent width or height is set to the max integer value,
    // the window manager lets us pick a resolution.
    if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
    {
        // Window manager dictated a resolution, just return this
        return capabilities.currentExtent;
    }
    // set width and height to the size of the glfw framebuffer
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    return {
        clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t MainApplication::chooseSwapMinImageCount(
    vk::SurfaceCapabilitiesKHR const& surfaceCapabilities)
{
    // try to request 3 images at least for triple buffering
    auto minImageCount = max(3u, surfaceCapabilities.minImageCount);
    // if max image count is not 0, we can't request unlimited images
    if (surfaceCapabilities.maxImageCount != 0)
        minImageCount = clamp(minImageCount,
            surfaceCapabilities.minImageCount,
            surfaceCapabilities.maxImageCount);
    return minImageCount;
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

bool MainApplication::isValidQueueFamily(
    vk::raii::PhysicalDevice physicalDevice,
    const vector<vk::QueueFamilyProperties>& qfpList,
    uint32_t qfpIndex
)
{
    const auto& qfp = qfpList[qfpIndex];
    bool graphicsSupported = (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
        static_cast<vk::QueueFlags>(0);
    bool surfacesSupported = physicalDevice.getSurfaceSupportKHR(qfpIndex, *m_surface);
    return graphicsSupported && surfacesSupported;
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