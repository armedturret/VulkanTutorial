# CMakeList.txt : CMake project for VulkanTutorial, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.28)

project (VulkanTutorialProject)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(slang CONFIG REQUIRED)

file(GLOB SOURCES CMAKE_CONFIGURE_DEPENDS "*.cpp")

add_executable (VulkanTutorial ${SOURCES})

add_custom_command(
    TARGET VulkanTutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/vk_layer_settings.txt"
        "$<TARGET_FILE_DIR:VulkanTutorial>/vk_layer_settings.txt"
)

target_compile_definitions(VulkanTutorial PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 # easier extension referencing
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1 # lets us use c++ 20 initializer lists
    GLFW_INCLUDE_VULKAN=1 # setup glfw for vulkan
)

set_property(TARGET VulkanTutorial PROPERTY CXX_STANDARD 20)

target_link_libraries(VulkanTutorial PRIVATE glm::glm glfw Vulkan::Vulkan slang::slang)
