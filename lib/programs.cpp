
#include <fstream>
#include <sstream>
#include <string.h>
#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include "../include/main.hpp"
#include "../include/render-pipe.hpp"
#include "../include/my-utils.hpp"
#include <libcamera/libcamera.h>


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, 1280, 720);
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
if  (severity != GL_DEBUG_SEVERITY_NOTIFICATION)    
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
	   type, severity, message );
}

int ring_list_test(struct main_params* init_data) {
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Shader Example", NULL, NULL);

  if (!window)
  {
	std::cerr << "Failed to create GLFW window" << std::endl;
	glfwTerminate();
	return -1;
  } 

  glfwMakeContextCurrent(window); // Initialize GLEW
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental=true; // Needed in core profilexZx
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
}

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
	  std::cerr << "Warning: Not using Wayland" << std::endl;
  } else {
	  std::cout << "Using Wayland" << std::endl;
  }

  rp::vec2 dims{init_data->dims.x, init_data->dims.y};
  rp::Camera cam(init_data->device_path, dims);
  cam.configure_buffers();

  rp::Renderer yuv_streamer(dims);

  yuv_streamer.enable_gl_debug(MessageCallback);
  //yuv_streamer.print_supported_extensions();

  
    yuv_streamer.create_shader_program(
        "shaders/basic_vertex.glsl",
        init_data->frag_path);
  
  float vertices[] =
    {
	// positions          // texture coords
	0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
	0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
	-0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
  unsigned int indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
  };

  yuv_streamer.vertex_setup(vertices, indices, sizeof(vertices), sizeof(indices));

  rp::tex_context y_tex{
    0, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureY", 0};
  
  rp::tex_context u_tex{
    1, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureU", 0};

  rp::tex_context v_tex{
    2, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureV", 0};
 
  rp::tex_context still_y_tex{
    3, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureY", 1};
  
  rp::tex_context still_u_tex{
    4, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureU", 1};

  rp::tex_context still_v_tex{
    5, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureV", 1};
 
  
  cam.start_stream();
  uint8_t *tmp=(uint8_t*)cam.get_frame();
  printf("\nY Dims: (y:%zu,x:%zu)\n", y_tex.dims.x, y_tex.dims.y);
  yuv_streamer.create_texture(&y_tex, tmp);
  printf("U Dims: (y:%zu,x:%zu)\n", u_tex.dims.x, u_tex.dims.y);
  yuv_streamer.create_texture(&u_tex, tmp + (dims.x*dims.y));
  printf("V Dims: (y:%zu,x:%zu)\n", v_tex.dims.x, v_tex.dims.y);
  yuv_streamer.create_texture(&v_tex,
                              tmp + (dims.x * dims.y) + (dims.x * dims.y) / 4);
  
  printf("still_Y Dims: (y:%zu,x:%zu)\n", still_y_tex.dims.x, still_y_tex.dims.y);
  yuv_streamer.create_texture(&still_y_tex, tmp);
  printf("still_U Dims: (y:%zu,x:%zu)\n", still_u_tex.dims.x, still_u_tex.dims.y);
  yuv_streamer.create_texture(&still_u_tex, tmp + (dims.x*dims.y));
  printf("still_V Dims: (y:%zu,x:%zu)\n", still_v_tex.dims.x, still_v_tex.dims.y);
  yuv_streamer.create_texture(&still_v_tex,
                              tmp + (dims.x * dims.y) + (dims.x * dims.y) / 4);  

  //yuv_streamer.print_uniform_info();
  yuv_streamer.activate_program();
  char c = '\0';
  int state = 0;
  size_t buffer_offset;
  buffer_offset = (size_t)(dims.x * dims.y * 1.5);
  mu::RingList<uint8_t *> frame_list;
  uint8_t *frames_buffer =
      (uint8_t *)calloc(60 * (dims.x * dims.y * 1.5), sizeof(uint8_t));
  uint8_t *buffer_curr = frames_buffer;
  uint8_t *buffer_end = frames_buffer + 60*buffer_offset;

  while (!glfwWindowShouldClose(window)) { 
    state = glfwGetKey(window, GLFW_KEY_F);

    tmp = (uint8_t *)cam.get_frame();
    memcpy(buffer_curr, tmp, buffer_offset);
    frame_list.add_elem(buffer_curr);
    buffer_curr += buffer_offset;  
    yuv_streamer.update_surface_group(tmp, 0);
    if (frame_list.get_number_of_elements() == 60) {
      printf("Delay update\n");
      yuv_streamer.update_surface_group(frame_list.pop_elem(), 1);
    }
    if (buffer_curr >= buffer_end)
      buffer_curr = frames_buffer;
    yuv_streamer.render_surface();
    glfwSwapBuffers(window);
    glfwPollEvents();

    yuv_streamer.clear_render_surface();
  };
  return 0;
}
int shader_playground(struct main_params* init_data)
{
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Shader Example", NULL, NULL);

  if (!window)
  {
	std::cerr << "Failed to create GLFW window" << std::endl;
	glfwTerminate();
	return -1;
  } 

  glfwMakeContextCurrent(window); // Initialize GLEW
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental=true; // Needed in core profilexZx
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
}

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
	  std::cerr << "Warning: Not using Wayland" << std::endl;
  } else {
	  std::cout << "Using Wayland" << std::endl;
  }

  rp::vec2 dims{init_data->dims.x, init_data->dims.y};
  rp::Camera cam(init_data->device_path, dims);
  cam.configure_buffers();

  rp::Renderer yuv_streamer(dims);

  yuv_streamer.enable_gl_debug(MessageCallback);
  //yuv_streamer.print_supported_extensions();

  
    yuv_streamer.create_shader_program(
        "shaders/basic_vertex.glsl",
        init_data->frag_path);
  
  float vertices[] =
    {
	// positions          // texture coords
	0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
	0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
	-0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
  unsigned int indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
  };

  yuv_streamer.vertex_setup(vertices, indices, sizeof(vertices), sizeof(indices));

  rp::tex_context y_tex{
    0, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureY", 0};
  
  rp::tex_context u_tex{
    1, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureU", 0};

  rp::tex_context v_tex{
    2, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureV", 0};
 
  rp::tex_context still_y_tex{
    3, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureY", 1};
  
  rp::tex_context still_u_tex{
    4, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureU", 1};

  rp::tex_context still_v_tex{
    5, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureV", 1};
 
  
  cam.start_stream();
  uint8_t *tmp=(uint8_t*)cam.get_frame();
  printf("\nY Dims: (y:%zu,x:%zu)\n", y_tex.dims.x, y_tex.dims.y);
  yuv_streamer.create_texture(&y_tex, tmp);
  printf("U Dims: (y:%zu,x:%zu)\n", u_tex.dims.x, u_tex.dims.y);
  yuv_streamer.create_texture(&u_tex, tmp + (dims.x*dims.y));
  printf("V Dims: (y:%zu,x:%zu)\n", v_tex.dims.x, v_tex.dims.y);
  yuv_streamer.create_texture(&v_tex,
                              tmp + (dims.x * dims.y) + (dims.x * dims.y) / 4);
  
  printf("still_Y Dims: (y:%zu,x:%zu)\n", still_y_tex.dims.x, still_y_tex.dims.y);
  yuv_streamer.create_texture(&still_y_tex, tmp);
  printf("still_U Dims: (y:%zu,x:%zu)\n", still_u_tex.dims.x, still_u_tex.dims.y);
  yuv_streamer.create_texture(&still_u_tex, tmp + (dims.x*dims.y));
  printf("still_V Dims: (y:%zu,x:%zu)\n", still_v_tex.dims.x, still_v_tex.dims.y);
  yuv_streamer.create_texture(&still_v_tex,
                              tmp + (dims.x * dims.y) + (dims.x * dims.y) / 4);  

  //yuv_streamer.print_uniform_info();
  yuv_streamer.activate_program();
  char c = '\0';
  int state = 0;
  while(!glfwWindowShouldClose(window))
    {
        state = glfwGetKey(window, GLFW_KEY_F);

    tmp = (uint8_t *)cam.get_frame();

    yuv_streamer.update_surface_group(tmp, 0);
     if (state == GLFW_PRESS) {
      printf("F key pressed\n");
      yuv_streamer.update_surface_group(tmp, 1);
      
    }
    yuv_streamer.render_surface();
    glfwSwapBuffers(window);
    glfwPollEvents();

    yuv_streamer.clear_render_surface();
    };
  return 0;
}
int render_api_test(struct main_params* init_data)
{
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Shader Example", NULL, NULL);

  if (!window)
  {
	std::cerr << "Failed to create GLFW window" << std::endl;
	glfwTerminate();
	return -1;
  } 

  glfwMakeContextCurrent(window); // Initialize GLEW
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental=true; // Needed in core profilexZx
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
}

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
	  std::cerr << "Warning: Not using Wayland" << std::endl;
  } else {
	  std::cout << "Using Wayland" << std::endl;
  }

  rp::vec2 dims{320,180};
  rp::Camera cam("/dev/video0", dims);
  cam.configure_buffers();

  rp::Renderer yuv_streamer(dims);

  yuv_streamer.enable_gl_debug(MessageCallback);
  //yuv_streamer.print_supported_extensions();

  yuv_streamer.create_shader_program("shaders/basic_vertex.glsl", "shaders/basic_fragment.glsl");
  
  float vertices[] =
	{
	// positions          // texture coords
	0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
	0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
	-0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
  unsigned int indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
  };

  yuv_streamer.vertex_setup(vertices, indices, sizeof(vertices), sizeof(indices));

  rp::tex_context y_tex{
    0, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE,
    "textureY"
  };
  
  rp::tex_context u_tex{
    1, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE,
    "textureU"        
  };

  rp::tex_context v_tex{
    2, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE,
    "textureV"
  };
  
  cam.start_stream();
  uint8_t *tmp=(uint8_t*)cam.get_frame();
  printf("\nY Dims: (y:%zu,x:%zu)\n", y_tex.dims.x, y_tex.dims.y);
  yuv_streamer.create_texture(&y_tex, tmp);
  printf("U Dims: (y:%zu,x:%zu)\n", u_tex.dims.x, u_tex.dims.y);
  yuv_streamer.create_texture(&u_tex, tmp + (dims.x*dims.y));
  printf("V Dims: (y:%zu,x:%zu)\n", v_tex.dims.x, v_tex.dims.y);
  yuv_streamer.create_texture(&v_tex,
			      tmp + (dims.x * dims.y) + (dims.x * dims.y) / 4);

  //yuv_streamer.print_uniform_info();
  yuv_streamer.activate_program();
  char c = '\0';
  while(!glfwWindowShouldClose(window))
    {
      //c = std::getchar();
      //std::printf("tick\n");
      tmp = (uint8_t*)cam.get_frame();
      yuv_streamer.update_surface_group(tmp,0);
      yuv_streamer.render_surface();
      glfwSwapBuffers(window);
      glfwPollEvents();
      /*struct timespec ts {
	0,
	2000000000
      };
      nanosleep(&ts, nullptr);*/
      yuv_streamer.clear_render_surface();
    };
  return 0;
}

int demo(struct main_params* init_data) {
  char device_path[64];

  if (init_data->device_path != nullptr) {
    memcpy(device_path, init_data->device_path, strlen(init_data->device_path));
    printf("Using %s", device_path);    
  }
  else{
    sprintf(device_path, "/dev/video0");
    }
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Shader Example", NULL, NULL);

  if (!window)
  {
	std::cerr << "Failed to create GLFW window" << std::endl;
	glfwTerminate();
	return -1;
  } 

  glfwMakeContextCurrent(window); // Initialize GLEW
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental=true; // Needed in core profilexZx
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
}

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
	  std::cerr << "Warning: Not using Wayland" << std::endl;
  } else {
	  std::cout << "Using Wayland" << std::endl;
  }
  rp::vec2 dims{640,360};
  rp::Camera cam(init_data->device_path, dims);
  cam.configure_buffers();
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback( MessageCallback, 0 );
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
  std::string vertex_shader_src = load_shader_from_file("shaders/basic_vertex.glsl");
  GLuint vert_shader = rp::compile_shader_source(GL_VERTEX_SHADER, vertex_shader_src);

  std::string fragment_shader_src = load_shader_from_file("shaders/basic_fragment.glsl");
  GLuint frag_shader = rp::compile_shader_source(GL_FRAGMENT_SHADER, fragment_shader_src);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vert_shader);
  glAttachShader(shader_program, frag_shader);
  glLinkProgram(shader_program);

  //glDeleteShader(vert_shader);
  //glDeleteShader(frag_shader);
 
  float vertices[] =
	{
	// positions          // texture coords
	0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
	0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
	-0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
  unsigned int indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
  };
  GLuint VBO, VAO, EBO;
  GLenum err=GL_NO_ERROR;
  glGenVertexArrays(1, &VAO);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - VertexArray: %x\n", err );
  }
  glGenBuffers(1, &VBO);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - GenVBO: %x\n", err);
  }
  glGenBuffers(1, &EBO);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - GenEBO: %x\n", err );
  }
  glBindVertexArray(VAO);
  printf("Vertices size: %zu\n", sizeof(vertices));
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
  printf("Indices size: %zu\n", sizeof(indices));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  
  while((err = glGetError()) != GL_NO_ERROR){
	  std::cerr << "ERROR - BindBuffers: %d"<< err << std::endl;
  }

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // texture coordinate attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - vertex attrib array: %x\n", err);
  }
  cam.start_stream();
  uint8_t *tmp;
  tmp = (uint8_t*)cam.get_frame();

  
  unsigned int textureY, textureU, textureV;
  glGenTextures(1, &textureY);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex0 gen_tex: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glBindTexture(GL_TEXTURE_2D, textureY);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex0 bind: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  // set the texture minimize/magnify parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex0 tex_param_1: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // set texture filtering parameters
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex0 tex_param_2: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x, dims.y, 0, GL_RED, GL_UNSIGNED_BYTE, tmp);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex0: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glGenerateMipmap(GL_TEXTURE_2D);

  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &textureU);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex1 gen_tex: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glBindTexture(GL_TEXTURE_2D, textureU);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex1 bind: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  // set the texture minimize/magnify parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex1 tex_param_2: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // set texture filtering parameters
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex1 tex_param_2: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x/2, dims.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, tmp + (dims.x * dims.y));
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex1: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glActiveTexture(GL_TEXTURE2);
  glGenTextures(1, &textureV);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex2 gen_tex: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glBindTexture(GL_TEXTURE_2D, textureV);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex2 bind: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  // set the texture minimize/magnify parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex2 tex_param_1: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // set texture filtering parameters
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex2 tex_param_2: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dims.x/2, dims.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, tmp + (dims.x * dims.y) + (dims.x * dims.y)/4);
  while((err = glGetError()) != GL_NO_ERROR){
	printf("ERROR - Tex2: %x\n", err);
	fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  
  glUseProgram(shader_program);

  glUniform1i(glGetUniformLocation(shader_program, "textureY"), 0);
  glUniform1i(glGetUniformLocation(shader_program, "textureU"), 1);
  glUniform1i(glGetUniformLocation(shader_program, "textureV"), 2);
 
 
  
  while((err = glGetError()) != GL_NO_ERROR){
	  std::cerr << "ERROR - Var linking: "<< err << std::endl;
  }
  
  while(!glfwWindowShouldClose(window))
    {
      {
	  
	// magic happens here
	glClearColor(0.0f, 0.2f, 0.4f, 1.0);
	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - glClearColor(): %x\n", err);
	  fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - glClear(): %x\n", err);
	  fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
	}
	  
	tmp = (uint8_t*)cam.get_frame();
	  
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dims.x, dims.y, GL_RED, GL_UNSIGNED_BYTE, tmp);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - tex_sub0: %x\n", err);
	  fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
	}
	   
	glActiveTexture(GL_TEXTURE1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dims.x/2, dims.y/2, GL_RED, GL_UNSIGNED_BYTE, tmp + (dims.x * dims.y));
	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - tex_sub1: %x\n", err);
	  fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
	}

	glActiveTexture(GL_TEXTURE2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dims.x/2, dims.y/2, GL_RED, GL_UNSIGNED_BYTE, tmp + (dims.x * dims.y) + (dims.x * dims.y)/4);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - tex_sub2: %x\n", err);
	  fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
	}
	glUseProgram(shader_program);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	while((err = glGetError()) != GL_NO_ERROR){
	  printf("ERROR - Draw command: %x\n", err);
	}
	  
      }
      glfwSwapBuffers(window);
      glfwPollEvents();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shader_program);

  glfwTerminate();
  return 0;
}

using namespace libcamera;
static std::mutex mtx;
static std::condition_variable cv;
static bool frame_ready = false;
static std::shared_ptr<Camera> camera;
//Camera camera;
static void requestComplete(Request *request);
static void requestComplete(Request *request)
{
    if (request->status() == Request::RequestCancelled)
        return;

    const auto &buffers = request->buffers();

    for (auto const &bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        std::cout << "seq=" << std::setw(6) << std::setfill('0')
                  << metadata.sequence << " bytesused=";

        unsigned int nplane = 0;
        for (const FrameMetadata::Plane &plane : metadata.planes()) {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size())
                std::cout << "/";
        }

        std::cout << std::endl;
    }

    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);
}

int rpi_libcamera_demo(struct main_params* init_data){

  /*Initialize GLFW*/
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW\n";
    return EXIT_FAILURE;
  }

  /* Create window */
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(init_data->dims.x, init_data->dims.y, "Libcamera Demo", NULL, NULL);
  if (!window)
  {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);

  /*Configure window*/
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental = true; // Needed in core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW\n";
    return EXIT_FAILURE;
  }

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
    std::cerr << "Warning: Not using Wayland\n";
    return EXIT_FAILURE;
  } else {
    std::cout << "Using Wayland\n";
  }

  rp::vec2 dims{init_data->dims.x, init_data->dims.y};
  rp::Renderer yuv_streamer(dims);
  yuv_streamer.enable_gl_debug(MessageCallback);

  yuv_streamer.create_shader_program("shaders/basic_vertex.glsl", init_data->frag_path);

  float vertices[] =
  {
    // positions          // texture coords
    0.5f,  0.5f, 0.0f,     1.0f, 0.0f,		// top right
    0.5f, -0.5f, 0.0f,     1.0f, 1.0f,		// bottom right
    -0.5f, -0.5f, 0.0f,    0.0f, 1.0f,		// bottom left
    -0.5f,  0.5f, 0.0f,    0.0f, 0.0f		// top left  
  };
  unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
  };

  yuv_streamer.vertex_setup(vertices, indices, sizeof(vertices), sizeof(indices));

  rp::tex_context y_tex{
    0, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureY", 0};
  rp::tex_context u_tex{
    1, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureU", 0};
  rp::tex_context v_tex{
    2, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "textureV", 0};
    rp::tex_context still_y_tex{
    3, {dims.x, dims.y}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureY", 1};
  rp::tex_context still_u_tex{
    4, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureU", 1};
  rp::tex_context still_v_tex{
    5, {dims.x / 2, dims.y / 2}, GL_RED, GL_RGBA, GL_UNSIGNED_BYTE, "still_textureV", 1}; 

  yuv_streamer.create_texture(&y_tex, nullptr);
  yuv_streamer.create_texture(&u_tex, nullptr);
  yuv_streamer.create_texture(&v_tex, nullptr);
  yuv_streamer.create_texture(&still_y_tex, nullptr);
  yuv_streamer.create_texture(&still_u_tex, nullptr);
  yuv_streamer.create_texture(&still_v_tex, nullptr);

  yuv_streamer.activate_program();
  
  /* Initialize the camera manager */
  std::unique_ptr cm = std::make_unique<libcamera::CameraManager>();

  /* Start the camera manager */
  if(cm->start())
  {
    std::cerr << "Failed to start camera manage\n";
    return EXIT_FAILURE;
  }

  auto cameras = cm->cameras();

  if (cameras.empty())
  {
    std::cerr << "No cameras found\n";
    return EXIT_FAILURE;
  }

  std::string camerasId = cameras[0]->id();
  camera = cm->get(camerasId);

  if(!camera)
  {
    std::cerr << "Failed to get camera\n";
    cm->stop();
    return EXIT_FAILURE;
  }

  if(camera->acquire())
  {
    std::cerr << "Failed to acquire camera\n";
    camera.reset();
    cm->stop();
    return EXIT_FAILURE;
  }
  
  std::unique_ptr<CameraConfiguration> config =
        camera->generateConfiguration({ StreamRole::Viewfinder });

  if (!config || config->empty()) {
      std::cerr << "Failed to generate configuration\n";
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
  }
  
  StreamConfiguration &streamConfig = config->at(0);
  StreamFormats format = streamConfig.formats();
  std::cout << "Default config: " << streamConfig.toString() << "\n";

  streamConfig.size.width = 640;
  streamConfig.size.height = 480;
  streamConfig.pixelFormat = libcamera::formats::YUV420;

  CameraConfiguration::Status validation = config->validate();
  
  std::cout << "Supported formats: \n";
  for (const PixelFormat &pf : format.pixelformats())
  {
    std::cout << "  " << pf.toString() << "\n";
    for (const auto &size : format.sizes(pf)) {
      //std::cout << "  " << size.width << "x" << size.height << "\n";
    }
  }
  
  if(validation == CameraConfiguration::Invalid)
  {
    std::cerr << "Invalid camera configuration\n";
    camera->release();
    camera.reset();
    cm->stop();
    return EXIT_FAILURE;
  }

  std::cout << "Validated config: " << streamConfig.toString() << std::endl;

  if (camera->configure(config.get()) < 0) {
      std::cerr << "Failed to configure camera\n";
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
  }
  auto allocator = std::make_unique<FrameBufferAllocator>(camera);

  for (StreamConfiguration &cfg : *config)
  {
    if (allocator->allocate(cfg.stream()) < 0)
    {
      std::cerr << "Failed to allocate buffers\n";
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
    }

    std::cout << "Allocated " << allocator->buffers(cfg.stream()).size() << " buffers\n";
  }
  
  Stream *stream = streamConfig.stream();
  const auto &buffers = allocator ->buffers(stream);

  std::vector<std::unique_ptr<Request>> requests;
  requests.reserve(buffers.size());

  // Create requests and add buffers to them
  for (unsigned int i = 0; i < buffers.size(); ++i)
  {
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request)
    {
      std::cerr << "Failed to create request\n";
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
    }

    if (request->addBuffer(stream, buffers[i].get()) < 0)
    {
      std::cerr << "Failed to add buffer to request\n";
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
    }

    requests.push_back(std::move(request));
  }

  camera->requestCompleted.connect(requestComplete);

  if (camera->start() < 0)
  {
    std::cerr << "Failed to start camera\n";
    camera->release();
    camera.reset();
    cm->stop();
    return EXIT_FAILURE;
  }

  // Queue the requests to the camera device

  for (auto &request : requests)
  {
    if (camera->queueRequest(request.get()) < 0)
    {
      std::cerr << "Failed to queue request\n";
      camera->stop();
      camera->release();
      camera.reset();
      cm->stop();
      return EXIT_FAILURE;
    }
  }
  std::this_thread::sleep_for(std::chrono::seconds(3)); // Wait for a few seconds to capture frames


  camera->stop();
  allocator->free(stream);
  camera->release();
  camera.reset();
  cm->stop();
  return EXIT_FAILURE;
}


/*std::string load_shader_from_file(const std::string& filename)
{
	std::ifstream file(filename);
	if(!file.is_open()){
		std::cerr << "Failed to open file: " << filename << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}*/
