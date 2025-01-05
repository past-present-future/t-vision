#ifndef RENDER_PIPE_HPP
#define RENDER_PIPE_HPP
#define GLEW
#include <cstdint>


#include <GL/glew.h>

#include <GL/gl.h>

#include <GLFW/glfw3.h>

#include <linux/videodev2.h>
#include <iostream>
namespace rp {

  
  typedef struct vec2Type
  {
	size_t x;
	size_t y;
  } vec2;
  
  typedef struct tex_contextType
  {
    GLuint id = 0;
    vec2 dims = {0,0};
    GLuint internal_format = GL_RGBA;
    GLuint update_format = GL_RED;
    GLuint data_type = GL_UNSIGNED_BYTE;
    char uniform_name[32] = "";
    uint8_t group_id = 0;    
  }tex_context;

  typedef struct frame_bufferType {
	struct v4l2_requestbuffers req = {0};
	struct v4l2_buffer buffer = {0};
	uint8_t *frame_data;
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
	uint8_t* get_frame();
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
   
    
  public:
    Renderer();
    Renderer(vec2 render_dims);
    void enable_gl_debug(void GLAPIENTRY (*MessageCallback)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *));
    int create_shader_program(const char * vert_shader_path, const char * frag_shader_path);
    int vertex_setup(float *vertices, unsigned int *indices, size_t vertices_size, size_t indices_size);
    int create_texture(rp::tex_context *tex_info, uint8_t *data);
    void clear_render_surface();
    void print_supported_extensions();
    void print_uniform_info();
    int update_surface_group(uint8_t *data, uint8_t group);
    void activate_program();
    int render_surface();
    int update_surface_other(uint8_t *data, uint8_t group);
  };
  

  /* Implement uniform location automatic pickup, list the variable names and assign them in declared order. 

   */
  GLuint compile_shader_source(GLenum type, const std::string& source);
  std::string load_shader_from_file(const std::string &filename);
  int yuv2rgb(uint8_t *in, uint8_t *out, vec2 dims);
};
#endif /*RENDER_PIPE_HPP*/
