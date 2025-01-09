#ifndef MAIN_HPP
#define MAIN_HPP

#include <iostream>

struct main_params
{
  const char *device_path;
  struct{
    size_t x;
    size_t y;
  }dims;
  char frag_path[32];
};    

int shader_playground(struct main_params* init_data);//char option);
int render_api_test(struct main_params* init_data);
int demo(struct main_params* init_data);

#endif /*MAIN_HPP*/
