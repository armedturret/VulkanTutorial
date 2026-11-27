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

// how many flames will we allow to concurrently render
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// Validation layers to use for debugging
const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation" // comes with the lunarg vulkan sdk
};

const std::vector<const char*> requiredDeviceExtensions = {
    vk::KHRSwapchainExtensionName // swapchain extension necessary for rendering to windows
};

class MainApplication {
public:
    void run();

private:
    static std::vector<std::uint8_t> readFileBinary(const std::string& filename);

    // glfw functions
    void initWindow();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowRefreshCallback(GLFWwindow* window);

    // vulkan init functions
    void initVulkan();

    void createInstance();
    std::vector<const char*> getRequiredValidationLayers();
    std::vector<const char*> getRequiredInstanceExtensions();

    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    // swap chain functions
    void recreateSwapChain();
    void createSwapChain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        std::vector<vk::SurfaceFormatKHR> const& availableFormats);
    vk::PresentModeKHR choosePresentMode(
        std::vector <vk::PresentModeKHR> const& availablePresentModes);
    vk::Extent2D chooseSwapExtent(
        vk::SurfaceCapabilitiesKHR const& capabilities);
    uint32_t chooseSwapMinImageCount(
        vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);

    void createImageViews();

    void cleanupSwapChain();
    // returns if a queue family has all the properties we expect
    bool isValidQueueFamily(
        vk::raii::PhysicalDevice physicalDevice,
        const std::vector<vk::QueueFamilyProperties>& qfpList,
        uint32_t qfpIndex
    );

    // funcs for creating the graphics pipeline
    void createGraphicsPipeline();
    vk::raii::ShaderModule createShaderModule(const std::vector<std::uint8_t>& code) const;

    // funcs for creating the command pool + buffers
    void createCommandPool();
    void createCommandBuffers();

    void createSyncObjects();

    // main loop functions
    void mainLoop();

    void drawFrame();

    void recordCommandBuffer(std::uint32_t imageIndex);
    void transitionImageLayout(
        std::uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask);

    void cleanup();

    GLFWwindow* m_window = nullptr;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance = nullptr;
    vk::raii::PhysicalDevice m_physicalDevice = nullptr;
    // logical device tied to the physical one
    vk::raii::Device m_device = nullptr;
    vk::raii::Queue m_graphicsQueue = nullptr;
    std::uint32_t m_queueIndex = -1;
    // window surface to render to
    vk::raii::SurfaceKHR m_surface = nullptr;
    vk::raii::SwapchainKHR m_swapchain = nullptr;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
    vk::SurfaceFormatKHR m_swapchainSurfaceFormat;
    vk::Extent2D m_swapchainExtent;
    // graphics pipeline stuff
    vk::raii::PipelineLayout m_pipelineLayout = nullptr;
    vk::raii::Pipeline m_graphicsPipeline = nullptr;
    // rendering stuff
    vk::raii::CommandPool m_commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> m_commandBuffers;
    // frame index for accessing command buffers + presentation semaphores
    std::uint32_t m_frameIndex = 0;
    // semaphores = gpu side synchronization
    std::vector<vk::raii::Semaphore> m_presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    // fence = cpu to gpu sychnronization (i.e. block program for something)
    std::vector<vk::raii::Fence> m_frameFences;

    bool m_framebufferResized = false;
};