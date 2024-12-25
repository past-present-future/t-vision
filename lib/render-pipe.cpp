#include "../include/render-pipe.hpp"
#include <cstddef>
#include <fstream>
#include <bits/types/struct_timeval.h>
#include <cerrno>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <ostream>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <time.h>


GLuint rp::Renderer::compile_shader_source(GLenum type, const std::string& source){
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
rp::Renderer::Renderer(rp::vec2 dims) : texture_num((size_t)0), tex_context_ary{}, viewport_dims(dims) {}


rp::Camera::Camera(){
  this->file_desc = open("/dev/video0", O_RDWR);
  if(this->file_desc < 0){
	perror("Failed to open device, OPEN");
  }
  if( ioctl(this->file_desc, VIDIOC_QUERYCAP, &(this->caps)) < 0){
	perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
  }
  
  this->fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  this->fmt.fmt.pix.width = 320;
  this->fmt.fmt.pix.height = 180;
  this->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
  this->fmt.fmt.pix.field = V4L2_FIELD_NONE;

  if( ioctl(this->file_desc, VIDIOC_S_FMT, &(this->fmt)) < 0 ){
	perror("Device could not set format, VIDIOC_S_FMT");
  }
}

rp::Camera::Camera(const char* cam_path, vec2 dims){
  this->file_desc = open(cam_path, O_RDWR);
  if(this->file_desc < 0){
	perror("Failed to open device, OPEN");
  }
  if( ioctl(this->file_desc, VIDIOC_QUERYCAP, &(this->caps)) < 0){
	perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
  }
  else {
     printf("Caps reported got: %x\n", this->caps.capabilities);
  }
   	
  this->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->fmt.fmt.pix.width = dims.x;
  this->fmt.fmt.pix.height = dims.y;
  this->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YVU420;
  this->fmt.fmt.pix.field = V4L2_FIELD_NONE;

  if( ioctl(this->file_desc, VIDIOC_G_FMT, &(this->fmt)) < 0){
	perror("Device could not get format, VIDIOC_S_FMT");
  }
  else{
    dims.x = this->fmt.fmt.pix.width;
    dims.y= this->fmt.fmt.pix.height;
    printf("Dims got x: %d, y: %d\n\rFormat got: %x\n", dims.x, dims.y, this->fmt.fmt.pix.pixelformat);
  }
}

int rp::Camera::configure_buffers(){

  this->frame_handle.req.count = 1;
  this->frame_handle.req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->frame_handle.req.memory = V4L2_MEMORY_MMAP;
  if(ioctl(this->file_desc, VIDIOC_REQBUFS, &(this->frame_handle.req)) < 0){
	perror("Could not request buffer from device, VIDIOC_REQBUFS");
	return 1;
  }
  
  this->frame_handle.buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  this->frame_handle.buffer.memory = V4L2_MEMORY_MMAP;
  this->frame_handle.buffer.index = 0;
  if(ioctl(this->file_desc, VIDIOC_QUERYBUF, &(this->frame_handle.buffer)) < 0){
	perror("Could not request buffer from device, VIDIOC_QUERYBUF");
    return 1;
  }

  this->frame_handle.frame_data = mmap(nullptr, this->frame_handle.buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->file_desc, this->frame_handle.buffer.m.offset);
  
  return 0;
}

int rp::Camera::start_stream(){
  int ret;
  ret = ioctl(this->file_desc, VIDIOC_STREAMON, &(this->frame_handle.buffer.type));
  if (ret < 0){
	ioctl(this->file_desc, VIDIOC_LOG_STATUS);
	std::cerr << "Could not start streaming, VIDIOC_STREAMON ,error code: " << errno;
	
	
  }
  return ret; 
}

void* rp::Camera::get_frame(){
  struct timeval tv;
  int ret=0;
  
  ret = ioctl(this->file_desc, VIDIOC_QBUF, &(this->frame_handle.buffer));
   if (ret < 0){
	ioctl(this->file_desc, VIDIOC_LOG_STATUS);
	std::cerr << "Could not queue buffer: " << errno << std::endl;
  }
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 16000;
  nanosleep(&ts, nullptr);
  ret = ioctl(this->file_desc, VIDIOC_DQBUF, &(this->frame_handle.buffer));
   if (ret < 0){
	ioctl(this->file_desc, VIDIOC_LOG_STATUS);
	std::cerr << "Could not return buffer: " << errno << std::endl;
  }
  return this->frame_handle.frame_data;
}

rp::Camera::~Camera(){
  ioctl(this->file_desc, VIDIOC_STREAMOFF, &(this->frame_handle.buffer.type));
  munmap(this->frame_handle.frame_data, this->frame_handle.buffer.length);
}

int rp::Renderer::create_shader_program(const char * vert_shader_path, const char * frag_shader_path){
  std::string vertex_shader_src = load_shader_from_file(vert_shader_path);
  GLuint vert_shader = compile_shader_source(GL_VERTEX_SHADER, vertex_shader_src);
  if (vert_shader < 0)
    return 1;
 
  std::string fragment_shader_src = load_shader_from_file(frag_shader_path);
  GLuint frag_shader = compile_shader_source(GL_FRAGMENT_SHADER, fragment_shader_src);
  if (frag_shader < 0)
    return 1;

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vert_shader);
  glAttachShader(shader_program, frag_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  GLuint i;
  GLint count;

  GLint size; // size of the variable
  GLenum type; // type of the variable (float, vec3 or mat4, etc)

  const GLsizei bufSize = 16; // maximum name length
  GLchar name[bufSize]; // variable name in GLSL
  GLsizei length; // name length

  std::cout << "Shader program variables: \n\r";

  glGetProgramiv(shader_program, GL_ACTIVE_ATTRIBUTES, &count);
  std::cout << "Active attributes:\n";

  for( i = count-1 ; i <= 0 ; --i)
    { 
      glGetActiveAttrib(shader_program, i, bufSize, &length, &size, &type, name);
      std::cout << "\tAttribute #" << i << " \n\r\tType: " << type << "\n\r\t Name: "<< name << std::endl;
    }
  glGetProgramiv(shader_program, GL_ACTIVE_UNIFORMS, &count);
  printf("Active Uniforms: %d\n", count);

  for (i = 0; i < count; i++)
    {
      glGetActiveUniform(shader_program, (GLuint)i, bufSize, &length, &size, &type, name);

      printf("Uniform #%d Type: %u Name: %s\n", i, type, name);
    }
  return 0;
}

std::string rp::Renderer::load_shader_from_file(const std::string& filename)
{
	std::ifstream file(filename);
	if(!file.is_open()){
		std::cerr << "Failed to open file: " << filename << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

int rp::Renderer::vertex_setup(float *vertices, uint8_t *indices)
{
  GLenum err=GL_NO_ERROR;
  
  glGenVertexArrays(1, &(this->VAO));
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - VertexArray: %x\n", err );
  }
  
  glGenBuffers(1, &(this->VBO));
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - GenVBO: %x\n", err);
  }

  glGenBuffers(1, &(this->EBO));
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - GenEBO: %x\n", err );
  }
  glBindVertexArray(this->VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  
  while((err = glGetError()) != GL_NO_ERROR){
	  std::cerr << "ERROR - BindBuffers: %d"<< err << std::endl;
  }

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - vertex attrib array: %x\n", err);
  }
}

int rp::Renderer::create_texture(rp::tex_context tex_info)
{
  GLuint active_tex, err=GL_NO_ERROR;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex);
  if( active_tex != (GL_TEXTURE0 + this->texture_num))
    glActiveTexture(GL_TEXTURE0 + this->texture_num);

  glGenTextures(1, &(tex_info.id));
   while((err = glGetError()) != GL_NO_ERROR){
     printf("ERROR - Tex #%u gen_tex: %x\n", tex_info.id, err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glBindTexture(GL_TEXTURE_2D, tex_info.id);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex #%u bind: %x\n", tex_info.id, err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  
  return 0;
}
