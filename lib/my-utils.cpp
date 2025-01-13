#include <fstream>
#include <sstream>
#include "../include/my-utils.hpp"

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

template<typename T>
mu::RingList<T>::RingList(){
  this->items =  nullptr;
  elem_num = 0;
}

template <typename T> mu::Node<T>::Node(T value) { this->data = value; }

template <typename T >
void mu::RingList<T>::add_elem(T data)
{
  Node<T> *tmp = new Node(data);
  if (items != nullptr) {

    tmp->left = items->left;
    tmp->right = tmp; 
    items->left = tmp;
  }
  else {
    tmp->left = tmp;
    tmp->right = tmp;
  }  
  items = tmp;
  ++this->elem_num;
 }

template <typename T>
T mu::RingList<T>::pop_elem(){
  Node<T> *left = this->items->left;
  this->items->left = left->left;
  T value = left->data;
  delete[] left;
  --this->elem_num;  
  return value;
}


template <typename T >
size_t mu::RingList<T>::get_number_of_elements(){
  return this->elem_num;
  }
