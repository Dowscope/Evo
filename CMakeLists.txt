cmake_minimum_required(VERSION 3.25)

project(EVO VERSION 0.1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(EVO
    src/main.cpp
    src/core/ApplicationConfig.cpp
    src/core/Logger.cpp
    src/systems/CameraSystem.cpp
    src/systems/EventSystem.cpp
    src/systems/GameSystem.cpp
    src/systems/ScreenSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/System.cpp
    src/systems/VulkanSystem.cpp
)

target_compile_features(EVO PRIVATE cxx_std_23)
target_include_directories(EVO PRIVATE src)
target_link_libraries(EVO PRIVATE Vulkan::Vulkan glfw)

if(MSVC)
    target_compile_options(EVO PRIVATE /W4 /permissive-)
else()
    target_compile_options(EVO PRIVATE -Wall -Wextra -Wpedantic)
endif()
