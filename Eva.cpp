/*
 * Eva LLVM Executable.
 */

#include "src/Eva.h"

#include <string>

int main(int argc, char const *argv[]) {
  // Program to execute.
  std::string program = R"(
    (printf "Value: %d\n" 42)
  )";

  // Compiler instance.
  Eva vm;

  //Generate LLVM IR.
  vm.exec(program);

  return 0;
}