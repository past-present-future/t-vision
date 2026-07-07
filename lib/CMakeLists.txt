#add_library(my-utils STATIC my-utils.cpp)

add_library(render-pipe render-pipe.cpp)

add_library(programs programs.cpp)

target_link_libraries(programs PRIVATE
				    ${GLFW_LIBRARIES}
						${WAYLAND_LIBRARIES}
						${OPENGL_LIBRARIES}

						EGL
						GLEW
						render-pipe)
target_include_directories(programs PRIVATE
    ${LIBCAMERA_INCLUDE_DIRS}
    include
)