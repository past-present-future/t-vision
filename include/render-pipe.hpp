#ifndef RENDER_PIPE_HPP
#define RENDER_PIPE_HPP

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>
#include <iostream>

GLuint compile_shader_source(GLenum type, const std::string& source);

#endif /*RENDER_PIPE_HPP*/
