cmake_minimum_required(VERSION 3.30.5)
project(ghastly)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Finde required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-egl)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory(lib)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_INCLUDE_DIRS}
    ${WAYLAND_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    Include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLFW_LIBRARIES}
    ${WAYLAND_LIBRARIES}
    ${OPENGL_LIBRARIES}
    EGL
	GLEW
	my-utils
	render-pipe
)

# Set compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    ${GLFW_CFLAGS_OTHER}
    ${WAYLAND_CFLAGS_OTHER}
)

configure_file(${CMAKE_SOURCE_DIR}/shaders/vertex.glsl ${CMAKE_BINARY_DIR}/vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/shaders/fragment.glsl ${CMAKE_BINARY_DIR}/fragment.glsl COPYONLY)
