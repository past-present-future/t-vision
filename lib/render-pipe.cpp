#include "../include/render-pipe.hpp"
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>


GLuint rp::compile_shader_source(GLenum type, const std::string& source){
  GLuint shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint ret;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
  if(!ret){
	GLchar info_log[512];
	glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
	std::cerr << "Shader compilation failed:\n" << info_log << std::endl;
	glDeleteShader(shader);
	return 0;
  }
  return shader;
}

rp::Camera::Camera(){
  this->file_desc = open("/dev/video0", O_RDWR);
  ioctl(this->file_desc, VIDIOC_QUERYCAP, &(this->caps));

  this->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->fmt.fmt.pix.width = 1024;
  this->fmt.fmt.pix.height = 768;
  this->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
  this->fmt.fmt.pix.field = V4L2_FIELD_NONE;

  ioctl(this->file_desc, VIDIOC_S_FMT, &(this->fmt));
}

rp::Camera::Camera(const char* cam_path, vec2 dims){
  this->file_desc = open(cam_path, O_RDWR);
  ioctl(this->file_desc, VIDIOC_QUERYCAP, &(this->caps));

  this->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->fmt.fmt.pix.width = dims.x;
  this->fmt.fmt.pix.height = dims.y;
  this->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
  this->fmt.fmt.pix.field = V4L2_FIELD_NONE;

  ioctl(this->file_desc, VIDIOC_S_FMT, &(this->fmt));
}

int rp::Camera::configure_buffers(){

  this->frame_handle.req.count = 1;
  this->frame_handle.req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->frame_handle.req.memory = V4L2_MEMORY_MMAP;
  ioctl(this->file_desc, VIDIOC_REQBUFS, &(this->frame_handle.req));
  
  this->frame_handle.buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->frame_handle.buffer.memory = V4L2_MEMORY_MMAP;
  this->frame_handle.buffer.index = 0;
  ioctl(this->file_desc, VIDIOC_QUERYBUF, &(this->frame_handle.buffer));

  this->frame_handle.frame_data = mmap(nullptr, this->frame_handle.buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->file_desc, this->frame_handle.buffer.m.offset);
  
  return 0;
}

int rp::Camera::start_stream(){
  return ioctl(this->file_desc, VIDIOC_STREAMON, &(this->frame_handle.buffer.type));
}

void* rp::Camera::get_frame(){
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(this->file_desc, &fds);
  select(this->file_desc + 1, &fds, nullptr, nullptr, nullptr);

  ioctl(this->file_desc, VIDIOC_QBUF, &(this->frame_handle.buffer));
  ioctl(this->file_desc, VIDIOC_DQBUF, &(this->frame_handle.buffer));
  return this->frame_handle.frame_data;
}

rp::Camera::~Camera(){
  ioctl(this->file_desc, VIDIOC_STREAMOFF, &(this->frame_handle.buffer.type));
  munmap(this->frame_handle.frame_data, this->frame_handle.buffer.length);
}
