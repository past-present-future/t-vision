#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "include/render-pipe.hpp"
#include "include/my-utils.hpp"

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

  GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Shader Example", NULL, NULL);

  if (!window)
  {
	std::cerr << "Failed to create GLFW window" << std::endl;
	glfwTerminate();
	return -1;
  }

  glfwMakeContextCurrent(window); // Initialize GLEW
  glewExperimental=true; // Needed in core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
}

  if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
        std::cerr << "Warning: Not using Wayland" << std::endl;
    } else {
        std::cout << "Using Wayland" << std::endl;
    }

  std::string vertex_shader_src = load_shader_from_file("vertex.glsl");
  GLuint vert_shader = compile_shader_source(GL_VERTEX_SHADER, vertex_shader_src);

  std::string fragment_shader_src = load_shader_from_file("fragment.glsl");
  GLuint frag_shader = compile_shader_source(GL_FRAGMENT_SHADER, fragment_shader_src);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vert_shader);
  glAttachShader(shader_program, frag_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  // Set up vertex data
  float vertices[] =
  {
	-0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
  };
  GLuint VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), NULL);
  glEnableVertexAttribArray(0);

  while(!glfwWindowShouldClose(window))
  {
	{
	  // magic happens here
	  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  glUseProgram(shader_program);
	  
	  float time_val = glfwGetTime();
	  int time_shader_var = glGetUniformLocation(shader_program, "time");
	  glUniform1f(time_shader_var, time_val);

	  glBindVertexArray(VAO);
	  glDrawArrays(GL_TRIANGLES, 0, 3);
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

