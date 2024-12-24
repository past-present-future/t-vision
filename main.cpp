#include <iostream>
#include <ostream>
#include <string>
#include <fstream>
#include <sstream>
#include "include/render-pipe.hpp"
#include <stdio.h>
#include "include/my-utils.hpp"
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, 640, 360);
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
	   type, severity, message );
}

int main(void)
{
  
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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
  std::string vertex_shader_src = load_shader_from_file("vertex.glsl");
  GLuint vert_shader = rp::compile_shader_source(GL_VERTEX_SHADER, vertex_shader_src);

  std::string fragment_shader_src = load_shader_from_file("fragment.glsl");
  GLuint frag_shader = rp::compile_shader_source(GL_FRAGMENT_SHADER, fragment_shader_src);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vert_shader);
  glAttachShader(shader_program, frag_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);
 
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
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  
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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  while((err = glGetError()) != GL_NO_ERROR){
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

std::string load_shader_from_file(const std::string& filename)
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

