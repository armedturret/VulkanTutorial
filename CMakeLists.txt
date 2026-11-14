# CMakeList.txt : CMake project for VulkanTutorial, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

project ("VulkanTutorial")

find_package(assimp CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(slang CONFIG REQUIRED)

file(GLOB SOURCES CMAKE_CONFIGURE_DEPENDS "*.cpp")

add_executable (VulkanTutorial ${SOURCES})

set_property(TARGET VulkanTutorial PROPERTY CXX_STANDARD 20)

target_link_libraries(VulkanTutorial PRIVATE glm::glm glfw assimp::assimp Vulkan::Vulkan slang::slang)
