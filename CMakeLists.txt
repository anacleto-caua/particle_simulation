# Config
cmake_minimum_required(VERSION 3.2...4.2)
project(particles)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

# Shader compilation
find_program(GLSLC_EXECUTABLE glslc REQUIRED HINTS "$ENV{VULKAN_SDK}/bin")

file(GLOB_RECURSE SHADER_SOURCE_FILES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)
set(SPV_BINARY_FILES "")

foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
    set(SPV_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPV_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SPV_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader: ${SHADER_NAME} to ${SPV_OUTPUT}"
    )

    list(APPEND SPV_BINARY_FILES ${SPV_OUTPUT})
endforeach()

# Add shader compilation target
add_custom_target(Shaders ALL DEPENDS ${SPV_BINARY_FILES})

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

# The executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Add dependencies
add_dependencies(${PROJECT_NAME} Shaders)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Setup header only Libraries
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/glm-1.0.2"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/stb_image"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/tinyobjloader"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    Vulkan::Vulkan
)

# Global definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    GLFW_INCLUDE_VULKAN
)

# Os specifics
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        gdi32 
        user32 
        shell32
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dl
        pthread
        X11
        Xxf86vm
        Xrandr
        Xi
    )
endif()

# Copy assets, textures and shaders to the build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"

    COMMENT "Copying assets to output directory..."
)