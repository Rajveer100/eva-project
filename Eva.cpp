/*
 * Eva interpreter.
 */
#include <iostream>
#include <cassert>

class Eva {

  template<typename T>
  bool isNumber(T exp);

  template<typename T>
  bool isString(T exp);

 public:

  template<typename T>
  T eval(T exp);

  template<typename T>
  T eval(std::vector<T> exp);
};

template<typename T>
bool Eva::isNumber(T exp) {
  return std::is_integral<T>::value;
}

template<typename T>
bool Eva::isString(T exp) {
  return std::is_same<decltype(exp), std::string>::value;
}

template<typename T>
T Eva::eval(T exp) {
  try {
    if (isNumber(exp)) {
      return exp;
    }

    if (isString(exp)) {
      return exp;
    }
  } catch (const std::exception& e) {
    std::cerr << "Unimplemented: " << e.what() << "\n";
  }
}

template<typename T>
T Eva::eval(std::vector<T> exp) {
  try {
    if (exp[0] == "+") {
      return std::to_string(
              eval(std::stoi(exp[1])) + eval(std::stoi(exp[2]))
              );
    }

    if (exp[0] == "-") {
      return std::to_string(
              eval(std::stoi(exp[1])) - eval(std::stoi(exp[2]))
              );
    }

    if (exp[0] == "*") {
      return std::to_string(
              eval(std::stoi(exp[1])) * eval(std::stoi(exp[2]))
              );
    }

    if (exp[0] == "/") {
      return std::to_string(
              eval(std::stoi(exp[1])) / eval(std::stoi(exp[2]))
              );
    }
  } catch (const std::exception& e) {
    std::cerr << "Unimplemented: " << e.what() << "\n";
  }
}

int main() {
  Eva eva;

  assert(eva.eval<int>(1) == 1);
  assert(eva.eval<std::string>("1") == "1");

  assert(eva.eval<std::string>({"*", "2", "3"}) == "6");

  std::cout << "All assertions passed!";
}