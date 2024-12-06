#ifndef RENDER_PIPE_HPP
#define RENDER_PIPE_HPP

#include <cstdint>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
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
  
  GLuint compile_shader_source(GLenum type, const std::string& source);
};
#endif /*RENDER_PIPE_HPP*/
