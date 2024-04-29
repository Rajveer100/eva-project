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
#include "Environment.h"

using syntax::EvaParser;

/*
 * Environment type.
 */
using Env = std::shared_ptr<Environment>;

class Eva {
 public:

  Eva(): parser(std::make_unique<EvaParser>()) {
    moduleInit();
    setupExternalFunctions();
    setupGlobalEnvironment();
  }

  /*
   * Executes a program.
   */
  void exec(const std::string &program) {
    // 1. Parse the program:
    auto ast = parser->parse("(begin " + program + ")");

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
    fn = createFunction("main", llvm::FunctionType::get(/* return type */ builder->getInt32Ty(),
                                                                          /* vararg */false), GlobalEnv);

    createGlobalVar("VERSION", builder->getInt32(42));

    // 2. Compile main body.
    gen(ast, GlobalEnv);

    builder->CreateRet(builder->getInt32(0));
  }

  /*
   * Main compile loop.
   */
  llvm::Value* gen(const Expr& expr, Env env) {
    switch (expr.type) {
      /*
       * Numbers
       */
      case ExprType::NUMBER: {
        return builder->getInt32(expr.number);
      }
      /*
       * Strings
       */
      case ExprType::STRING: {
        // Unescape special characters. TODO: Support all characters or handle in parser.
        auto re = std::regex("\\\\n");
        auto str = std::regex_replace(expr.string, re, "\n");
        return builder->CreateGlobalStringPtr(str);
      }
      case ExprType::SYMBOL: {
        /*
         * Boolean
         */
        if (expr.string == "true" || expr.string == "false") {
          return builder->getInt1(expr.string == "true");
        } else {
          /*
           * Variables
           */
          auto varName = expr.string;
          auto value = env->lookup(varName);

          // Local Variables
          if (auto localVar = llvm::dyn_cast<llvm::AllocaInst>(value)) {
            return builder->CreateLoad(localVar->getType(), localVar, varName.c_str());
          }
          // Global Variables
          else if (auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
            return builder->CreateLoad(globalVar->getInitializer()->getType(), globalVar, varName.c_str());
          }
        }
      }
      case ExprType::LIST: {
        auto tag = expr.list[0];
        /*
         * Special Cases.
         */
        if (tag.type == ExprType::SYMBOL) {
          auto op = tag.string;

          /*
           * Variable declaration: (var x (+ y 10))
           *
           * Typed: (var (x number) 42)
           *
           * Locals are allocated on the stack.
           */
          if (op == "var") {
            // TODO: Handle Generics
            auto varNameDecl = expr.list[1];
            auto varName = extractVarName(varNameDecl);
            // Initializer
            auto init = gen(expr.list[2], env);

            // Type
            auto varTy = extractVarType(varNameDecl);

            // Variable
            auto varBinding = allocVar(varName, varTy, env);

            // Set value
            return builder->CreateStore(init, varBinding);
          } else if (op == "set") {
            /*
             * Variable update: (set x 100)
             */
            // Value
            auto value = gen(expr.list[2], env);

            auto varName = expr.list[1].string;

            // Variable
            auto varBinding = env->lookup(varName);

            // Set value
            return builder->CreateStore(value, varBinding);
          } else if (op == "begin") {
            /*
             * Blocks (begin <expressions>)
             */
            auto blockEnv = std::make_shared<Environment>(std::map<std::string, llvm::Value*>{}, env);

            llvm::Value *blockRes;
            for (auto i = 1; i < expr.list.size(); i += 1) {
              // Generate expression code.
              blockRes = gen(expr.list[i], blockEnv);
            }
            return blockRes;
          } else if (op == "printf") {
            // printf extern function:
            //
            // (printf "Value: %d" 42)
            auto printfFn = module->getFunction("printf");
            std::vector<llvm::Value *> args;

            for (auto i = 1; i < expr.list.size(); i += 1) {
              args.push_back(gen(expr.list[i], env));
            }
            return builder->CreateCall(printfFn, args);
          }
        }
      }
    }

    // Unreachable
    return builder->getInt32(0);
  }

  /*
   * Extracts variable or parameter type with i32 as default.
   * x -> i32
   * (x number) -> number
   */
  std::string extractVarName(const Expr& expr) {
    return expr.type == ExprType::LIST ? expr.list[0].string : expr.string;
  }

  /*
   *
   */
  llvm::Type* extractVarType(const Expr& expr) {
    return expr.type == ExprType::LIST ? getTypeFromString(expr.list[1].string) : builder->getInt32Ty();
  }

  llvm::Type* getTypeFromString(const std::string& type_) {
    // number -> i32
    if (type_ == "number") {
      return builder->getInt32Ty();
    }

    // string -> i8* (aka char*)
    if (type_ == "string") {
      return builder->getInt8Ty()->getPointerTo();
    }

    // default
    return builder->getInt32Ty();
  }

  llvm::Value* allocVar(const std::string& name, llvm::Type* type_, Env env) {
    varsBuilder->SetInsertPoint(&fn->getEntryBlock());

    auto varAlloc = varsBuilder->CreateAlloca(type_, 0, name.c_str());

    // Add to the environment
    env->define(name, varAlloc);

    return varAlloc;
  }

  /*
   * Creates a global variable.
   */
  llvm::GlobalVariable* createGlobalVar(const std::string& name, llvm::Constant* init) {
    module->getOrInsertGlobal(name, init->getType());

    auto variable = module->getGlobalVariable(name);

    variable->setAlignment(llvm::MaybeAlign(4));
    variable->setConstant(false);
    variable->setInitializer(init);

    return variable;
  }

  /*
   * Define external functions (from libc++)
   */
  void setupExternalFunctions() {
    // i8* to substitute for char*, void*, etc.
    auto bytePtrTy = builder->getInt8Ty()->getPointerTo();

    // int printf (const char* format, ...);
    module->getOrInsertFunction("printf", llvm::FunctionType::get(
                                        /* return type */ builder->getInt32Ty(),
                                        /* format arg */ bytePtrTy,
                                        /* vararg */ true));
  }

  /*
   * Creates a function.
   */
  llvm::Function* createFunction(const std::string &fnName, llvm::FunctionType* fnType, Env env) {
    // Function prototype might already be defined.
    auto fn = module->getFunction(fnName);

    // If not, allocate the function.
    if (!fn) {
      fn = createFunctionProto(fnName, fnType, env);
    }

    createFunctionBlock(fn);
    return fn;
  }

  /*
   * Create function prototype (defines the function, excluding the body).
   */
  llvm::Function* createFunctionProto(const std::string &fnName, llvm::FunctionType* fnType, Env env) {
    auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);
    llvm::verifyFunction(*fn);

    // Install in the environment.
    env->define(fnName, fn);

    return fn;
  }

  /*
   * Create function block.
   */
  void createFunctionBlock(llvm::Function* fn) {
    auto entry = createBB("entry", fn);
    builder->SetInsertPoint(entry);
  }

  /*
   * Create a basic block.
   */
  llvm::BasicBlock* createBB(std::string name, llvm::Function* fn = nullptr) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
  }

  /*
   * Saves IR to file.
   */
  void saveModuleToFile(const std::string &filename) {
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(filename, errorCode);

    module->print(outLL, nullptr);
  }

  /*
   * Initialise the module.
   */
  void moduleInit() {
    // Open a new context and module.
    ctx = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("Eva", *ctx);

    // Create a builder for the module.
    builder = std::make_unique<llvm::IRBuilder<>>(*ctx);

    // Vars builder
    varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
  }

  /*
   * Sets up the Global Environment.
   */
  void setupGlobalEnvironment() {
    std::map<std::string, llvm::Value*> globalObject {
      {"VERSION", builder->getInt32(42)},
    };

    std::map<std::string, llvm::Value*> globalRec{};

    for (auto &entry: globalObject) {
      globalRec[entry.first] = createGlobalVar(entry.first, (llvm::Constant*) entry.second);
    }

    GlobalEnv = std::make_shared<Environment>(globalRec, nullptr);
  }

  // Global Environment (symbol table)
  std::shared_ptr<Environment> GlobalEnv;

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

  // Extra builder for variable declarations. This builder prepends
  // to the beginning of every function block.
  std::unique_ptr<llvm::IRBuilder<>> varsBuilder;
};

#endif //EVA_EVA_H
