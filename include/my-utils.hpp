#ifndef MY_UTILS_HPP 
#define MY_UTILS_HPP
#include <iostream>

/* @TODO
   Make an event manager that will handle requests. Idea is that it will have a queue of tasks that will be distributed to worker threads
*/
std::string load_shader_from_file(const std::string& filename);
namespace mu {
template <typename T> class Node {
public:  
  Node *left, *right;
  T data;
  Node(T data);
};
template <typename  T>
class RingList{
private:
  Node<T> *items;
  size_t elem_num;

public:
  RingList();
  size_t get_number_of_elements();
  void add_left(T data);
  void add_elem(T data);
  T pop_elem();
  T pop_right();
  
  };

}; // namespace mu

#include "../lib/my-utils.cpp"
#endif /* MY-UTILS_HPP */
