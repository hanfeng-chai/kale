#include "ast.h"
#include "llvm.h"

#include <map>

llvm::LLVMContext theContext;
llvm::IRBuilder<> Builder(theContext);
llvm::Module theModule("my cool jit", theContext);
std::map<std::string, llvm::Value *> NamedValues;

// pass and analysis manager
auto theFunctionPassManager = llvm::FunctionPassManager();
auto theLoopAnalysisManager = llvm::LoopAnalysisManager();
auto theFunctionAnalysisManager = llvm::FunctionAnalysisManager();
auto theCGSCCAnalysisManager = llvm::CGSCCAnalysisManager();
auto theModuleAnalysisManager = llvm::ModuleAnalysisManager();
auto thePassInstrumentationCallbacks = llvm::PassInstrumentationCallbacks();
auto theStandardInstrumentations =
    llvm::StandardInstrumentations(theContext, true);
auto thePassBuilder = llvm::PassBuilder();
auto _ = (theStandardInstrumentations.registerCallbacks(
              thePassInstrumentationCallbacks, &theModuleAnalysisManager),
          theFunctionPassManager.addPass(llvm::InstCombinePass()),
          theFunctionPassManager.addPass(llvm::ReassociatePass()),
          theFunctionPassManager.addPass(llvm::GVNPass()),
          theFunctionPassManager.addPass(llvm::SimplifyCFGPass()),
          thePassBuilder.registerModuleAnalyses(theModuleAnalysisManager),
          thePassBuilder.registerFunctionAnalyses(theFunctionAnalysisManager),
          thePassBuilder.crossRegisterProxies(
              theLoopAnalysisManager, theFunctionAnalysisManager,
              theCGSCCAnalysisManager, theModuleAnalysisManager),
          nullptr);

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
  theFunctionPassManager.run(*func, theFunctionAnalysisManager);
  return func;
}
