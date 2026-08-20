cmake_minimum_required(VERSION 3.25)

project(EVO VERSION 0.1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_program(GLSLC glslc REQUIRED)

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(SHADER_OUTPUTS)
foreach(SHADER land.vert land.frag shadow.vert)
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
        COMMAND "${GLSLC}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}"
        VERBATIM
    )
    list(APPEND SHADER_OUTPUTS "${SHADER_OUTPUT}")
endforeach()
add_custom_target(EvoShaders ALL DEPENDS ${SHADER_OUTPUTS})

add_executable(EVO
    src/main.cpp
    src/core/ApplicationConfig.cpp
    src/core/Logger.cpp
    src/systems/AtmosphereSystem.cpp
    src/systems/CameraSystem.cpp
    src/systems/ChunkSimulationSystem.cpp
    src/systems/EventSystem.cpp
    src/systems/GameSystem.cpp
    src/systems/ScreenSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/SunSystem.cpp
    src/systems/SurfaceTemperatureSystem.cpp
    src/systems/TerrainAnalysisSystem.cpp
    src/systems/TerrainGenerationSystem.cpp
    src/systems/TerrainMeshSystem.cpp
    src/systems/System.cpp
    src/systems/TimeSystem.cpp
    src/systems/VulkanSystem.cpp
)

target_compile_features(EVO PRIVATE cxx_std_23)
target_include_directories(EVO PRIVATE src)
target_link_libraries(EVO PRIVATE Vulkan::Vulkan glfw OpenGL::GL)
target_compile_definitions(EVO PRIVATE EVO_SHADER_DIR="${SHADER_OUTPUT_DIR}")
add_dependencies(EVO EvoShaders)

if(MSVC)
    target_compile_options(EVO PRIVATE /W4 /permissive-)
else()
    target_compile_options(EVO PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
endif()
