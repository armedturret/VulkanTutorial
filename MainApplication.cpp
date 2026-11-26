#include "MainApplication.hpp"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <fstream>

using namespace std;

vector<uint8_t> MainApplication::readFileBinary(const std::string& filename)
{
    // start from end to get file size quickly
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open())
    {
        throw runtime_error("Unable to open " + filename + "!");
    }

    vector<uint8_t> buffer(file.tellg());
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<streamsize>(buffer.size()));
    file.close();

    return buffer;
}

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
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
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
        if (!validQueueAvailable)
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
        if (!features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2)
        {
            cout << "BAD (Must support synchronization 2)" << endl;
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
        if (isValidQueueFamily(m_physicalDevice, queueFamilyProperties, qfpIndex))
            break;
    }

    m_queueIndex = qfpIndex;

    // Queue priority required even w/ just one queue
    float queuePriority = 0.5f;
    // Creates a queue family of one
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = m_queueIndex,
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
            {.synchronization2 = true, .dynamicRendering = true}, // PhysicalDeviceVulkan13Features
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

    m_graphicsQueue = vk::raii::Queue(m_device, m_queueIndex, 0);
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

    auto availablePresentModes = m_physicalDevice.getSurfacePresentModesKHR(*m_surface);
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
    vector <vk::PresentModeKHR> const& availablePresentModes)
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

void MainApplication::createImageViews()
{
    assert(!m_swapchainImages.empty());
    assert(m_swapchainImageViews.empty());

    cout << "Creating image views for swap chain..." << endl;

    // creates image views that let us access parts of images
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = m_swapchainSurfaceFormat.format,
    };

    imageViewCreateInfo.subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .levelCount = 1, // no mip map levels
        .layerCount = 1 // not stereoscopic
    };

    for (auto& image : m_swapchainImages)
    {
        imageViewCreateInfo.image = image;
        m_swapchainImageViews.emplace_back(m_device, imageViewCreateInfo);
    }
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

void MainApplication::createGraphicsPipeline()
{
    cout << "Creating graphics pipeline" << endl;
    auto shaderCode = readFileBinary("shaders/TriangleShader.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain",
        .pSpecializationInfo = nullptr // optional compile time "uniforms"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain",
        .pSpecializationInfo = nullptr // optional compile time "uniforms"
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    // Normally specifies layout of vertex buffers, empty for now
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    // Tells vulkan to render like GL_TRIANGLES
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False, // discard fragments outside depth range
        .rasterizerDiscardEnable = vk::False, // if this is true, rasterizer stage discards everything
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False, // if true, allows rasterizer to alter depth values
        .lineWidth = 1.0f
    };
    // disable MSAA for now
    vk::PipelineMultisampleStateCreateInfo multiSampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };
    // specify alpha blending for this framebuffer
    // color = newColor * srcAlpha + oldColor * (1 - srcAlpha)
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    // this setting affects all framebuffers
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, // disable bitwise blending
        .logicOp = vk::LogicOp::eCopy, // ignored?
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // Tells vulkan we want to specify viewport and scissor size at render time (instead of now)
    vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

    // normally specifies available uniforms, keep empty for now
    // separate from push constants (which are sorta like uniforms too...)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutInfo);

    // future me: you can use basePipelineIndex or basePipelineHandle to create pipeline derivatives
    // cheaper than re-creating a full pipeline if functionality common
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {
            .stageCount = 2, // vert + frag
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multiSampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout,
            // since dynamic rendering is available, we can skip specifying renderpass + framebuffer objects
            .renderPass = nullptr
        },
        // this is necessary if renderpass is nullptr, otherwise it should be omitted
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_swapchainSurfaceFormat.format
        }
    };

    // finally, create the graphics pipeline
    // the nullptr can be used to load a pipeline cache, which significantly speeds this operation up
    // pipeline chaches can even be stored in files!
    m_graphicsPipeline = vk::raii::Pipeline(m_device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

vk::raii::ShaderModule MainApplication::createShaderModule(const vector<uint8_t>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(uint8_t),
        // this is safe since vector ensures bytes are worst-case aligned
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    return { m_device, createInfo };
}

void MainApplication::createCommandPool()
{
    assert(m_queueIndex != -1);
    cout << "Creating command pool..." << endl;
    vk::CommandPoolCreateInfo poolInfo
    {
        // allow command buffers to be re-recorded individually every frame
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        // command buffers are executed by device queues
        .queueFamilyIndex = m_queueIndex
    };

    m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
}

void MainApplication::createCommandBuffer()
{
    assert(m_commandPool != nullptr);
    cout << "Creating command buffer..." << endl;

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_commandPool,
        // primary = can be submitted to queues directly, secondary = can be called indirectly
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    m_commandBuffer = move(vk::raii::CommandBuffers(m_device, allocInfo).front());
}

void MainApplication::createSyncObjects()
{
    cout << "Creating sync objects..." << endl;
    assert(m_device != nullptr);
    m_presentCompleteSemaphore = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo());
    m_renderFinishedSemaphore = vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo());
    m_drawFence = vk::raii::Fence(
        m_device,
        {
            .flags = vk::FenceCreateFlagBits::eSignaled // the fence starts signaled
        }
    );
}

void MainApplication::transitionImageLayout(
    std::uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask)
{
    // this function is useful to transition the image's layout
    // to one for rendering color attachments or presenting
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapchainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };
    m_commandBuffer.pipelineBarrier2(dependencyInfo);
}

void MainApplication::mainLoop()
{
    cout << "Beginning main loop..." << endl;
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        drawFrame();
    }

    // wait for all asynch processes to stop
    m_device.waitIdle();
}

void MainApplication::drawFrame()
{
    vk::Result fenceResult = m_device.waitForFences(*m_drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for fence!");
    }
    // fences, unlike semaphores, need to be cleared manually
    m_device.resetFences(*m_drawFence);

    auto [result, imageIndex] = m_swapchain.acquireNextImage(UINT64_MAX, *m_presentCompleteSemaphore, nullptr);
    recordCommandBuffer(imageIndex);

    // Wait until presentation is finished before rendering again
    // This can be parallelized better so the CPU doesn't have to wait here
    m_graphicsQueue.waitIdle();

    // technically, we can start executing the vertex stage while waiting on the image to be free
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        // wait on presentation to be complete
        .pWaitSemaphores = &*m_presentCompleteSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_commandBuffer,
        .signalSemaphoreCount = 1,
        // signal rendering finished when done
        .pSignalSemaphores = &*m_renderFinishedSemaphore
    };

    // start rendering while we set up presentation
    m_graphicsQueue.submit(submitInfo, *m_drawFence);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*m_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr // optional, pass an array to store results in for multiple swap chains
    };

    result = m_graphicsQueue.presentKHR(presentInfoKHR);
}

void MainApplication::recordCommandBuffer(std::uint32_t imageIndex)
{
    // begin info has some hints for recording a command buffer, but it's not pertinent rn
    m_commandBuffer.begin({});

    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal, // optimise for color attachments
        {}, // don't  wait on anything from previous stage
        vk::AccessFlagBits2::eColorAttachmentWrite, // subsequent stages wait on this
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = m_swapchainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear, // clear image before rendering
        .storeOp = vk::AttachmentStoreOp::eStore, // store to image after rendering
        .clearValue = clearColor
    };
    vk::RenderingInfo renderingInfo = {
        .renderArea = {
            .offset = {0, 0},
            .extent = m_swapchainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };
    // actually render the damn thing
    m_commandBuffer.beginRendering(renderingInfo);

    // use our previous created graphics pipeline (i.e. shaders and stuff)
    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline);

    // dynamic rendering, specify here instead of during pipeline creation
    m_commandBuffer.setViewport(0,
        vk::Viewport(
            0.0f, // x
            0.0f, // y
            static_cast<float>(m_swapchainExtent.width),
            static_cast<float>(m_swapchainExtent.height),
            0.0f, // min depth
            1.0f)); // max depth
    m_commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapchainExtent));

    // 700+ lines of code in.... draw a triangle
    m_commandBuffer.draw(3, 1, 0, 0);

    m_commandBuffer.endRendering();

    // need to transition to a presentation layout
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite, // wait for color attachment
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    // phew... it's over
    m_commandBuffer.end();
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