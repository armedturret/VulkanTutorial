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

function (add_slang_shader_target TARGET)
    cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
    set (SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders)
    set (ENTRY_POINTS -entry vertMain -entry fragMain)
    set (SLANGC_EXECUTABLE slangc)
    set (OUT_FILE ${SHADERS_DIR}/${TARGET}.spv)
    set (SLANG_COMMAND ${SLANGC_EXECUTABLE} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${OUT_FILE} ${SHADER_SOURCES})
    add_custom_command (
        OUTPUT  ${OUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "${SLANG_COMMAND}"
        COMMAND ${SLANG_COMMAND}
        DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shaders"
        VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${OUT_FILE})
endfunction()
function (target_add_shaders TARGET)
    cmake_parse_arguments ("ADDSH" "" "" "SHADERS" ${ARGN})
    set (SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders)
    set (NEW_SHADERS_DIR "$<TARGET_FILE_DIR:${TARGET}>/shaders")
    foreach(shader IN LISTS ADDSH_SHADERS)
        add_dependencies(${TARGET} ${shader})
        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SHADERS_DIR}/${shader}.spv"
                "${NEW_SHADERS_DIR}/${shader}.spv"
        )
    endforeach()
endfunction()

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
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS=1 # disable out of date error
    GLFW_INCLUDE_VULKAN=1 # setup glfw for vulkan
)

add_slang_shader_target(TriangleShader SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/BasicTriangle.slang)

set_property(TARGET VulkanTutorial PROPERTY CXX_STANDARD 20)

target_link_libraries(VulkanTutorial PRIVATE glm::glm glfw Vulkan::Vulkan slang::slang)
target_add_shaders(VulkanTutorial SHADERS TriangleShader)
