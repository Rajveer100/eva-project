/*
 * Eva to LLVM IR Compiler.
 */

#ifndef EVA_EVA_H
#define EVA_EVA_H

#include <iostream>
#include <string>
#include <regex>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>

#include "parser/EvaParser.h"

using syntax::EvaParser;

class Eva {
 public:
  Eva(): parser(std::make_unique<EvaParser>()) {
    moduleInit();
    setupExternalFunctions();
  }

  // Executes a program.
  void exec(const std::string &program) {
    // 1. Parse the program:
    auto ast = parser->parse(program);

    // 2. Compile to LLVM IR:
    compile(ast);

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
  void compile(const Expr& ast) {
    // 1. Create main function.
    fn = createFunction("main",
                        llvm::FunctionType::get(
                                /* return type */ builder->getInt32Ty(), false));

    createGlobalVar("VERSION", builder->getInt32(42));

    // 2. Compile main body.
    gen(ast);

    builder->CreateRet(builder->getInt32(0));
  }

  // Main compile loop.
  llvm::Value* gen(const Expr& expr) {
    switch (expr.type) {
      case ExprType::NUMBER:
        return builder->getInt32(expr.number);
      case ExprType::STRING: {
        // Unescape special characters. TODO: Support all characters or handle in parser.
        auto re = std::regex("\\\\n");
        auto str = std::regex_replace(expr.string, re, "\n");
        return builder->CreateGlobalStringPtr(str);
      }
      case ExprType::SYMBOL:
        // Boolean
        if (expr.string == "true" || expr.string == "false") {
          return builder->getInt1(expr.string == "true");
        } else {
          // Variables

          // TODO: Local Variables

          // Global Variables
          return module->getNamedGlobal(expr.string)->getInitializer();
        }

        return builder->getInt32(0);
      case ExprType::LIST:
        auto tag = expr.list[0];

        /*
         * Special Cases.
         */
        if (tag.type == ExprType::SYMBOL) {
          auto op = tag.string;

          // Variable declaration: (var x (+ y 10))
          if (op == "var") {
            // TODO: Handle Generics

            auto varName = expr.list[1].string;
            // Inititaliser
            auto init = gen(expr.list[2]);
            return createGlobalVar(varName, (llvm::Constant*) init)->getInitializer();
          } else if (op == "printf") {
            // printf extern function:
            //
            // (printf "Value: %d" 42)
            auto printfFn = module->getFunction("printf");
            std::vector<llvm::Value*> args{};

            for (auto i = 1; i < expr.list.size(); i += 1) {
              args.push_back(gen(expr.list[i]));
            }

            return builder->CreateCall(printfFn, args);
          }
        }
    }

    // Unreachable
    return builder->getInt32(0);
  }

  // Creates a global variable.
  llvm::GlobalVariable* createGlobalVar(const std::string& name, llvm::Constant* init) {
    module->getOrInsertGlobal(name, init->getType());
    auto variable = module->getGlobalVariable(name);
    variable->setAlignment(llvm::MaybeAlign(4));
    variable->setConstant(false);
    variable->setInitializer(init);
    return variable;
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

  // Parser
  std::unique_ptr<EvaParser> parser;

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
