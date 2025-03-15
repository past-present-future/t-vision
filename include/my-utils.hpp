#ifndef MY_UTILS_HPP 
#define MY_UTILS_HPP
#include <stddef.h>
#include <string>
/* @TODO
   Make an event manager that will handle requests. Idea is that it will have a queue of tasks that will be distributed to worker threads
*/
std::string load_shader_from_file(const std::string &filename);

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
    void clear_list();
  };
  
  template<typename T>
  RingList<T>::RingList(){
    this->items =  nullptr;
    this->elem_num = 0;
  }

  template <typename T> Node<T>::Node(T value) { this->data = value; }

  template <typename T >
  void RingList<T>::add_elem(T data)
  {
    Node<T> *new_elem = new Node(data);
    if (items != nullptr) {

      new_elem->left = items->left;
      new_elem->right = new_elem; 
      items->left = new_elem;
    }
    else {
      new_elem->left = new_elem;
      new_elem->right = new_elem;
    }  
    items = new_elem;
    ++this->elem_num;
  }

  template <typename T>
  T RingList<T>::pop_elem(){
    Node<T> *left = this->items->left;
    this->items->left = left->left;
    T elem_data = left->data;
    delete[] left;
    --this->elem_num;  
    return elem_data;
  }

  template <typename T >
  size_t RingList<T>::get_number_of_elements(){
    return this->elem_num;
  }

  template <typename T> void RingList<T>::clear_list() {
    while (this->elem_num > 0) {
      Node<T> *left = this->items->left;
      this->items->left = left->left;
      T elem_data = left->data;
      delete[] left;
      --this->elem_num;
    }
  }
}; // namespace mu


#endif /* MY-UTILS_HPP */
