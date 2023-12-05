/*
 * Eva to LLVM IR Compiler.
 */

#ifndef EVA_EVA_H
#define EVA_EVA_H

#include <iostream>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>

class Eva {
 public:
  Eva() {
    moduleInit();
    setupExternalFunctions();
  }

  // Executes a program.
  void exec(const std::string &program) {
    // 1. Parse the program:
    // auto ast = parser->parser(program)

    // 2. Compile to LLVM IR:
     compile();

    // Print generated code.
    module->print(llvm::outs(), nullptr);

    std::cout << "\n";

    // 3. Save module IR to file:
     saveModuleToFile("./out.ll");
  }

 private:
  /*
   * Compiles an expression.
   */
  void compile(/* TODO: AST*/) {
    // 1. Create main function.
    fn = createFunction("main",
                        llvm::FunctionType::get(
                                /* return type */ builder->getInt32Ty(), false));

    // 2. Compile main body.
    gen(/* AST */);

    builder->CreateRet(builder->getInt32(0));
  }

  // Main compile loop.
  llvm::Value* gen(/* expr */) {
    // return builder->getInt32(42);

    // strings:
    auto str = builder->CreateGlobalStringPtr("Hello, world!\n");

    // call to printf:
    auto printfFn = module->getFunction("printf");

    // args:
    std::vector<llvm::Value*> args{str};

    return builder->CreateCall(printfFn, args);
  }

  // Define external functions (from libc++)
  void setupExternalFunctions() {
    // i8* to substitute for char*, void*, etc.
    auto bytePtrTy = builder->getInt8Ty()->getPointerTo();

    // int printf (const char* format, ...);
    module->getOrInsertFunction("printf",
                                llvm::FunctionType::get(
                                        /* return type */ builder->getInt32Ty(),
                                        /* format arg */ bytePtrTy,
                                        /* vararg */ true));
  }

  // Creates a function.
  llvm::Function* createFunction(const std::string &fnName, llvm::FunctionType* fnType) {
    // Function prototype might already be defined.
    auto fn = module->getFunction(fnName);

    // If not, allocate the function.
    if (!fn) {
      fn = createFunctionProto(fnName, fnType);
    }

    createFunctionBlock(fn);
    return fn;
  }

  // Create function prototype (defines the function, excluding the body).
  llvm::Function* createFunctionProto(const std::string &fnName, llvm::FunctionType* fnType) {
    auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);
    llvm::verifyFunction(*fn);

    return fn;
  }

  // Create function block.
  void createFunctionBlock(llvm::Function* fn) {
    auto entry = createBB("entry", fn);
    builder->SetInsertPoint(entry);
  }

  // Create a basic block.
  llvm::BasicBlock* createBB(std::string name, llvm::Function* fn = nullptr) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
  }

  // Saves IR to file.
  void saveModuleToFile(const std::string &filename) {
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(filename, errorCode);

    module->print(outLL, nullptr);
  }

  // Initialise the module.
  void moduleInit() {
    // Open a new context and module.
    ctx = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("Eva", *ctx);

    // Create a builder for the module.
    builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
  }

  // Currently compiling function.
  llvm::Function* fn;

  // LLVM Context
  std::unique_ptr<llvm::LLVMContext> ctx;

  // LLVM Module Instance
  std::unique_ptr<llvm::Module> module;

  // LLVM IR Builder
  std::unique_ptr<llvm::IRBuilder<>> builder;
};

#endif //EVA_EVA_H
