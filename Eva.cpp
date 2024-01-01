/*
 * Eva LLVM Executable.
 */

#include "src/Eva.h"

#include <string>

int main(int argc, char const *argv[]) {
  // Program to execute.
  std::string program = R"(
    (var VERSION 42)
    (begin
      (var VERSION "Hello")
      (printf "Version: %d\n" VERSION)
    (printf "Version: %d\n" VERSION)
  )";

  // Compiler instance.
  Eva vm;

  //Generate LLVM IR.
  vm.exec(program);

  return 0;
}