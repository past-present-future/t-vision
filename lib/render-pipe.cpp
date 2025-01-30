#include "../include/render-pipe.hpp"
#include <GL/gl.h>
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
#include <unistd.h>


void GLAPIENTRY MessageCallback_int(GLenum source, GLenum type, GLuint id,
                                    GLenum severity, GLsizei length,
                                    const GLchar *message,
                                    const void *userParam) {
if  (severity != GL_DEBUG_SEVERITY_NOTIFICATION)  
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( (type == GL_DEBUG_TYPE_ERROR) ? "** GL ERROR **" : "" ),
	   type, severity, message );
}

float default_vertices[] =
  {
    // positions          // texture coords
    0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
    0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
    -0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
    -0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
unsigned int default_indices[] = {
  0, 1, 3, // first triangle
  1, 2, 3  // second triangle
};
  
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

rp::Recorder::Recorder(char *const record_dir) {
  this->p_dir = opendir(record_dir);
  if (this->p_dir == nullptr)
    std::cerr << "Not a valid directory, error code: " << errno;

  struct dirent *dir_entry = nullptr;
  while ((dir_entry = readdir(this->p_dir)) != nullptr) {
    if (dir_entry->d_type == DT_REG) {
      strstr(dir_entry->d_name, "recording-");

    }      

  }  
  
}    

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
    printf("Dims got x: %zu, y: %zu\n\rFormat got: %x\n", dims.x, dims.y, this->fmt.fmt.pix.pixelformat);
  }
}
rp::Camera::~Camera() {
  ioctl(this->file_desc, VIDIOC_STREAMOFF, &(this->frame_handle.buffer.type));
  munmap(this->frame_handle.frame_data, this->frame_handle.buffer.length);

  close(this->file_desc);  
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

  this->frame_handle.frame_data = (uint8_t*)mmap(nullptr, this->frame_handle.buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->file_desc, this->frame_handle.buffer.m.offset);
  
  return 0;
}

int rp::Camera::start_stream(){
  int ret;
  ret = ioctl(this->file_desc, VIDIOC_STREAMON, &(this->frame_handle.buffer.type));
  if (ret < 0){
    ioctl(this->file_desc, VIDIOC_LOG_STATUS);
    std::cerr << "Could not start streaming, VIDIOC_STREAMON, error code: " << errno;
  }
  return ret; 
}

uint8_t* rp::Camera::get_frame(){
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

rp::Renderer::Renderer(rp::vec2 dims)
    : texture_num((size_t)0), tex_context_ary{0},
      viewport_dims{dims.x, dims.y} {}

int rp::Renderer::create_shader_program(const char * vert_shader_path, const char * frag_shader_path){
  GLint err = GL_NO_ERROR;
  std::string vertex_shader_src = load_shader_from_file(vert_shader_path);
  GLuint vert_shader = compile_shader_source(GL_VERTEX_SHADER, vertex_shader_src);
  if (vert_shader < 0)
    return 1;
 
  std::string fragment_shader_src = load_shader_from_file(frag_shader_path);
  GLuint frag_shader = compile_shader_source(GL_FRAGMENT_SHADER, fragment_shader_src);
  if (frag_shader < 0)
    return 1;

  this->shader_program = glCreateProgram();
  glAttachShader(this->shader_program, vert_shader);
  glAttachShader(this->shader_program, frag_shader);
  glLinkProgram(this->shader_program);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Gen program: %x\n", err );
  }
  GLuint i;
  GLint count;

  GLint size; // size of the variable
  GLenum type; // type of the variable (float, vec3 or mat4, etc)

  const GLsizei bufSize = 32; // maximum name length
  GLchar name[bufSize]; // variable name in GLSL
  GLsizei length; // name length

  std::cout << "Shader program variables: \n\r";

  glGetProgramiv(this->shader_program, GL_ACTIVE_ATTRIBUTES, &count);
  std::cout << "Active attributes: " << count << std::endl;

  for( i = 0; i < count ; ++i)
    { 
      glGetActiveAttrib(shader_program, i, bufSize, &length, &size, &type, name);
      std::cout << "\tAttribute #" << i << " \n\r\tType: " << type << "\n\r\tName: "<< name << std::endl;
    }

 
  return 0;
}

std::string rp::load_shader_from_file(const std::string& filename)
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

int rp::Renderer::vertex_setup(float *vertices, unsigned int *indices, size_t vertices_size, size_t indices_size)
{
  GLenum err=GL_NO_ERROR;
  
  glGenVertexArrays(1, &(this->VAO));
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - GenVAO: %x\n", err );
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

  glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);
  
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

  return 0;
}

int rp::Renderer::create_texture(rp::tex_context *tex_info, uint8_t *data)
{
  GLint active_tex = 0, err=GL_NO_ERROR;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex);
  
  if( active_tex != (GL_TEXTURE0 + this->texture_num)) {
    glActiveTexture(GL_TEXTURE0 + this->texture_num);
    std::cout << "GL_TEXTURE" << this->texture_num << " activated\n";
  }
  else {
    std::cout << "Didn't change texture for creation." << " GL_TEXTURE"
              << this->texture_num << " already active\n";
  }

  glGenTextures(1, &(tex_info->id));
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Tex #%zu gen_tex: %x\n", this->texture_num, err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  //tex_context_ary[this->texture_num].id = tex_info->id;
  glBindTexture(GL_TEXTURE_2D, tex_info->id);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Tex #%u bind: %x\n", tex_info->id, err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }

  // set the texture minimize/magnify parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Tex%zu tex_param_1: %x\n", this->texture_num, err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // set texture filtering parameters
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Tex%zu tex_param_2: %x\n", this->texture_num, err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexImage2D(GL_TEXTURE_2D, 0, tex_info->internal_format, tex_info->dims.x, tex_info->dims.y, 0, GL_RED, tex_info->data_type,  data);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Tex%zu: %x\n", this->texture_num, err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glGenerateMipmap(GL_TEXTURE_2D);

  //glUseProgram(this->shader_program);

  tex_context_ary[this->texture_num].id = tex_info->id;
  tex_context_ary[this->texture_num].data_type = tex_info->data_type;
  tex_context_ary[this->texture_num].dims.x = tex_info->dims.x;
  tex_context_ary[this->texture_num].dims.y = tex_info->dims.y;
  tex_context_ary[this->texture_num].internal_format =
      tex_info->internal_format;
  tex_context_ary[this->texture_num].group_id = tex_info->group_id;

  /* GLsizei length;
     GLint size, count;
     GLenum type;
     GLchar name[32];

     glGetProgramiv(shader_program, GL_ACTIVE_UNIFORMS, &count);
     glGetActiveUniform(shader_program, (GLuint) this->texture_num , 32, &length,
     &size, &type, name);
     std::strncpy(this->tex_context_ary[this->texture_num].uniform_name, name,
     length); printf("Uniform #%zu Type: %u Name: %s\n", this->texture_num, type,
     this->tex_context_ary[this->texture_num].uniform_name);

     while((err = glGetError()) != GL_NO_ERROR){
     printf("ERROR - Get Var info: %x\n", err);
     }   */
  std::strncpy(this->tex_context_ary[this->texture_num].uniform_name, tex_info->uniform_name, strlen(tex_info->uniform_name));  
 
  printf("Created texture : #%u,\n\r\tName: %s\n\r\t Dims(x:%zu, y:%zu)\n", this->tex_context_ary[this->texture_num].id, this->tex_context_ary[this->texture_num].uniform_name, this->tex_context_ary[this->texture_num].dims.x, this->tex_context_ary[this->texture_num].dims.y);
   
  this->texture_num++;
  return err;
}

void rp::Renderer::print_supported_extensions()
{
  GLint n=0; 
  glGetIntegerv(GL_NUM_EXTENSIONS, &n); 
  PFNGLGETSTRINGIPROC glGetStringi = 0;
  glGetStringi = (PFNGLGETSTRINGIPROC)glfwGetProcAddress("glGetStringi");
  
  std::cout << "Supported extensions: \n";
  for (GLint i=0; i<n; i++) 
    { 
      const char* extension = 
	(const char*)glGetStringi(GL_EXTENSIONS, i);
      std::cout << "     " << extension << std::endl;
    } 
}

void rp::Renderer::enable_gl_debug(void GLAPIENTRY (*MessageCallback)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *))
{
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback( MessageCallback, 0 );
}

void rp::Renderer::clear_render_surface()
{
  GLint err = GL_NO_ERROR;
  
  glClearColor(0.0f, 0.2f, 0.4f, 1.0);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - glClear(): %x\n", err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
}

int rp::Renderer::update_surface_group(uint8_t *data, uint8_t group)
{
  tex_context *curr_tex = nullptr;
  GLint active_tex = GL_TEXTURE0, err = GL_NO_ERROR;
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  for( size_t i = 0 ; i < this->texture_num; ++i)
    {
      curr_tex = &(this->tex_context_ary[i]);
      if (curr_tex->group_id == group) {
       continue;
      } 
      glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex);
      if( active_tex != (GL_TEXTURE0 + i))
	glActiveTexture(GL_TEXTURE0 + i);
      else
	 std::cout << "Didn't change texture.\n";

      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curr_tex->dims.x, curr_tex->dims.y, curr_tex->internal_format, curr_tex->data_type, data);
      data = data + (curr_tex->dims.x * curr_tex->dims.y);
      }
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - glClear(): %x\n", err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }  
  return 0;
}

int rp::Renderer::render_surface()
{
  GLint err = GL_NO_ERROR;
  glUseProgram(this->shader_program);
  glBindVertexArray(this->VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - Draw command: %x\n", err);
  }
  return err;
}

void rp::Renderer::print_uniform_info()
{
  GLsizei length;
  GLint size, count;
  GLenum type;
  GLchar name[32];

  glGetProgramiv(shader_program, GL_ACTIVE_UNIFORMS, &count);

  for (size_t i = 0; i < count; ++i) {
    glGetActiveUniform(shader_program, i, 32, &length,
		       &size, &type, name);  
    //std::strncpy(this->tex_context_ary[this->texture_num].uniform_name, name, length);
    printf("Uniform #%zu Type: %u Name: %s\n", i, type, name);
  }  
}

void rp::Renderer::activate_program() {

  GLint err=GL_NO_ERROR;
 
  glUseProgram(this->shader_program);
  for (size_t i = 0; i < this->texture_num; ++i){
    glUniform1i(glGetUniformLocation(this->shader_program, this->tex_context_ary[i].uniform_name), i);
    while((err = glGetError()) != GL_NO_ERROR){
      std::cerr << "ERROR - Var linking: "<< err << std::endl;
    }
  } 
}


/*
  Does not provide same data as a shader does. Avoid for now
*/
int rp::yuv2rgb(uint8_t *in, uint8_t *out, vec2 dims) {
  
  if ((in == nullptr) || (out == nullptr))
    return -1;
  size_t frame_size = dims.x * dims.y;

  uint8_t *Cr = in + frame_size;
  uint8_t *Br = Cr + frame_size / 4;

  uint8_t y,u,v;

  double r,g,b;
  for (size_t i = 0; i < frame_size; ++i) {
    y = 1.164*(in[i] - 16);
    u = Cr[i / 4] - 128;
    v = Br[i / 4] - 128;

    r = y + 1.596 * u;
    g = y - 0.391 * v - 0.813 * u;
    b = y + 2.018 * v;

    out[i * 3] = (r < 255) ? 255 : ((r < 0) ? 0 : r);
    out[i * 3 + 1] = (g < 255) ? 255 : ((g < 0) ? 0 : g);
    out[i * 3 + 2] = (b < 255) ? 255 : ((b < 0) ? 0 : b);
  }    

  return 0;
}

int rp::Renderer::update_surface_other(uint8_t *data, uint8_t group)
{
  tex_context *curr_tex = nullptr;
  GLint active_tex = GL_TEXTURE0, err = GL_NO_ERROR;
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  for( size_t i = 0 ; i < this->texture_num; ++i)
    {
      curr_tex = &(this->tex_context_ary[i]);
      if (curr_tex->group_id != group) {
       continue;
      } 
      glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex);
      if( active_tex != (GL_TEXTURE0 + i))
	glActiveTexture(GL_TEXTURE0 + i);
      else
	 std::cout << "Didn't change texture.\n";

      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curr_tex->dims.x,
                      curr_tex->dims.y, curr_tex->internal_format,
                      curr_tex->data_type, data);
      printf("Updated skipped texture, %u :: %s :: %u\n", curr_tex->id, curr_tex->uniform_name, curr_tex->group_id);      
      data = data + (curr_tex->dims.x * curr_tex->dims.y);
      }
  while((err = glGetError()) != GL_NO_ERROR){
    printf("ERROR - glClear(): %x\n", err);
    fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }  
  return 0;
}
