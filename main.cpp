#include <cstdio>
#include <ctime>
#include "include/main.hpp"


int main(int argc, const char *argv[]) {
  for (size_t i = 0; i < argc; ++i)
    printf("%zu: %s\n", i, argv[i]);
  switch(argv[argc - 1][0])
    {
    case 'd':
      demo();
      break;    
    case 'a':render_api_test();
      break;
    case 's':shader_playground(argv[argc - 1][1]);
      break;
    default:printf("Unknown option selected\n");    
    }

  return 0;
}
