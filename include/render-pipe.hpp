#ifndef RENDER_PIPE_HPP
#define RENDER_PIPE_HPP
#define GLEW
#include <cstdint>


#include <GL/glew.h>

#include <GL/gl.h>

#include <GLFW/glfw3.h>

#include <linux/videodev2.h>
#include <iostream>
namespace rp
{
  typedef struct vec2Type
  {
	uint32_t x;
	uint32_t y;
  } vec2;
  
  typedef struct tex_contextType
  {
    GLuint id = 0;
    vec2 dims = {0,0};
    GLuint internal_format = GL_RGBA;
    GLuint data_type = GL_UNSIGNED_BYTE;
  }tex_context;

  typedef struct frame_bufferType {
	struct v4l2_requestbuffers req = {0};
	struct v4l2_buffer buffer = {0};
	void *frame_data;
  } frame_buffer;

  class Camera{
  private:
	int file_desc;
	struct v4l2_capability caps;
	struct v4l2_format fmt = {0};
	frame_buffer frame_handle;
  public:
	Camera();
	Camera(const char* cam_path, vec2 dims);
	int configure_buffers();
	int start_stream();
	void* get_frame();
	~Camera();
  };

  class Renderer{
  private:
    size_t texture_num;
    tex_context tex_context_ary[64];
    GLuint shader_program;
    GLuint VBO, VAO, EBO;
    vec2 viewport_dims;
    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    GLuint compile_shader_source(GLenum type, const std::string& source);
    std::string load_shader_from_file(const std::string& filename);
    int create_texture(rp::tex_context tex_info);
  public:
    Renderer();
    Renderer(vec2 render_dims);
    void enable_gl_debug();
    int create_shader_program(const char * vert_shader_path, const char * frag_shader_path);
    int vertex_setup(float *vertices, uint8_t *indices);
  };
  

  /* Implement uniform location automatic pickup, list the variable names and assign them in declared order. 

   */
  
};
#endif /*RENDER_PIPE_HPP*/
