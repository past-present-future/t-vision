
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include "../include/main.hpp"
#include "../include/render-pipe.hpp"
#include "../include/my-utils.hpp"


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

GLFWwindow* window_start()
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  

  GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Shader Example", NULL, NULL);

  if (!window)
    {
      std::cerr << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return nullptr;
    } 

  glfwMakeContextCurrent(window); // Initialize GLEW
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glewExperimental=true; // Needed in core profilexZx
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return nullptr;
  }

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
    std::cerr << "Warning: Not using Wayland" << std::endl;
  }
  else {
    std::cout << "Using Wayland" << std::endl;
  }
  
  return window;  
}    

int recorder_program(struct main_params *init_data) {
  printf("Started the recorder program.\n");
  
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

  printf("GL context created\n");
  char c = '\0';
  int state = 0;
  FILE *recording = fopen("hello_video.rd", "w+");
  if (recording == nullptr)
    printf("Cannot create hello_video.rd\n");
  else
    printf("Created hello_video.rd\n");
  size_t n_sec = 5;
  size_t frames_to_save = n_sec * 30;
  size_t buffer_offset = (size_t)(dims.x * dims.y * 1.5);
  
  mu::RingList<uint8_t *> frame_list;
  uint8_t *frames_buffer =
      (uint8_t *)calloc(frames_to_save * buffer_offset + 1, sizeof(uint8_t));
  
  uint8_t *buffer_curr = frames_buffer;
  uint8_t *buffer_end = frames_buffer + frames_to_save * buffer_offset+1;
  printf("buffer size: %zu\n", frames_to_save*buffer_offset);
  printf("prepared to read camera frames\n");
  size_t ind;
  for (ind = 0; ind < frames_to_save; ++ind) {
    buffer_curr = (uint8_t*)cam.get_frame();
    fwrite(buffer_curr, sizeof(uint8_t), buffer_offset, recording);
    //printf("Itteration: %zu\n", ind);
  }
  printf("recorded video\n");
  fclose(recording);
  free(frames_buffer);


  recording = fopen("hello_video.rd", "r");
  fseek(recording, 0, SEEK_END);
  printf("ftell return END: %ld\n", ftell(recording));
  frames_buffer = (uint8_t*)malloc(ftell(recording));
  buffer_curr = frames_buffer;
  fseek(recording, 0, SEEK_SET);
  printf("ftell return SET: %ld\n", ftell(recording));
  while (fread(buffer_curr, sizeof(uint8_t), buffer_offset, recording)) {
    frame_list.add_elem(buffer_curr);
    buffer_curr += buffer_offset;
  }
  fclose(recording);
  printf("read from recording\n");
  printf("Frames saved: %zu\n", frame_list.get_number_of_elements());
  for (; frame_list.get_number_of_elements() != 0; ) { 
    state = glfwGetKey(window, GLFW_KEY_F);
    
    tmp = frame_list.pop_elem();
    yuv_streamer.update_surface_group(tmp, 1);
    
    yuv_streamer.render_surface();
    glfwSwapBuffers(window);
    glfwPollEvents();
    usleep(333);
    yuv_streamer.clear_render_surface();

    printf(" %c\n", tmp[0]);

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
