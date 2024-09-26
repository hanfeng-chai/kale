#include "ast.h"

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <map>

llvm::LLVMContext theContext;
llvm::IRBuilder<> Builder(theContext);
llvm::Module theModule("my coll jit", theContext);
std::map<std::string, llvm::Value *> NamedValues;

llvm::Value *NumExprAST::codegen() {
  return llvm::ConstantFP::get(theContext, llvm::APFloat(val));
}

llvm::Value *VarExprAST::codegen() { return NamedValues[name]; }

llvm::Value *BinExprAST::codegen() {
  auto L = lhs->codegen();
  auto R = rhs->codegen();
  switch (op) {
  case '<':
    L = Builder.CreateFCmpULT(L, R);
    return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(theContext));
  case '+':
    return Builder.CreateFAdd(L, R);
  case '-':
    return Builder.CreateFSub(L, R);
  case '*':
    return Builder.CreateFMul(L, R);
  default:
    return nullptr;
  }
}

llvm::Value *CallExprAST::codegen() {
  auto Callee = theModule.getFunction(callee);
  std::vector<llvm::Value *> Args;
  for (auto &arg : arguments) {
    Args.push_back(arg->codegen());
  }
  return Builder.CreateCall(Callee, Args);
}

llvm::Function *ProtoTypeAST::codegen() {
  std::vector<llvm::Type *> doubles(parameters.size(),
                                    llvm::Type::getDoubleTy(theContext));
  auto type = llvm::FunctionType::get(llvm::Type::getDoubleTy(theContext),
                                      doubles, false);
  auto func = llvm::Function::Create(type, llvm::Function::ExternalLinkage,
                                     name, &theModule);
  int i = 0;
  for (auto &arg : func->args()) {
    arg.setName(parameters[i++]);
  }
  return func;
}

llvm::Function *FuncAST::codegen() {
  auto func = proto.codegen();
  auto block = llvm::BasicBlock::Create(theContext, "entry", func);
  Builder.SetInsertPoint(block);
  for (auto &arg : func->args()) {
    NamedValues[arg.getName().str()] = &arg;
  }
  auto value = body->codegen();
  Builder.CreateRet(value);
  llvm::verifyFunction(*func);
  return func;
}
