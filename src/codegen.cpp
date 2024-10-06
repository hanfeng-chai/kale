#include "ast.h"
#include "jit.h"
#include "llvm.h"

#include <map>

using namespace llvm;
using namespace llvm::orc;

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, Value *> NamedValues;
std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;
std::map<std::string, ProtoTypeAST> FunctionProtos;
ExitOnError ExitOnErr;

Function *getFunction(std::string name) {
  if (auto F = TheModule->getFunction(name)) {
    return F;
  }
  auto FI = FunctionProtos.find(name);
  if (FI != FunctionProtos.end()) {
    return FI->second.codegen();
  }
  return nullptr;
}

void InitializeModuleAndManagers() {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("KaleidoscopeJIT", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                     /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

llvm::Value *NumExprAST::codegen() {
  return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

llvm::Value *VarExprAST::codegen() { return NamedValues[name]; }

llvm::Value *BinExprAST::codegen() {
  auto L = lhs->codegen();
  auto R = rhs->codegen();
  switch (op) {
  case '<':
    L = Builder->CreateFCmpULT(L, R);
    return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext));
  case '+':
    return Builder->CreateFAdd(L, R);
  case '-':
    return Builder->CreateFSub(L, R);
  case '*':
    return Builder->CreateFMul(L, R);
  default:
    return nullptr;
  }
}

llvm::Value *CallExprAST::codegen() {
  auto Callee = getFunction(callee);
  std::vector<llvm::Value *> Args;
  for (auto &arg : arguments) {
    Args.push_back(arg->codegen());
  }
  return Builder->CreateCall(Callee, Args);
}

llvm::Value *IfExprAST::codegen() {
  auto CondV = Cond->codegen();
  CondV =
      Builder->CreateFCmpONE(CondV, ConstantFP::get(*TheContext, APFloat(0.0)));
  auto TheFunction = Builder->GetInsertBlock()->getParent();
  auto ThenBB = BasicBlock::Create(*TheContext);
  auto ElseBB = BasicBlock::Create(*TheContext);
  auto MergeBB = BasicBlock::Create(*TheContext);
  Builder->CreateCondBr(CondV, ThenBB, ElseBB);
  // Then
  TheFunction->insert(TheFunction->end(), ThenBB);
  Builder->SetInsertPoint(ThenBB);
  auto ThenV = Then->codegen();
  Builder->CreateBr(MergeBB);
  ThenBB = Builder->GetInsertBlock();
  // Else
  TheFunction->insert(TheFunction->end(), ElseBB);
  Builder->SetInsertPoint(ElseBB);
  auto ElseV = Else->codegen();
  Builder->CreateBr(MergeBB);
  ElseBB = Builder->GetInsertBlock();
  // Merge
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  auto phiNode = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2);
  phiNode->addIncoming(ThenV, ThenBB);
  phiNode->addIncoming(ElseV, ElseBB);
  return phiNode;
}

llvm::Function *ProtoTypeAST::codegen() {
  std::vector<llvm::Type *> doubles(parameters.size(),
                                    llvm::Type::getDoubleTy(*TheContext));
  auto type = llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext),
                                      doubles, false);
  auto func = llvm::Function::Create(type, llvm::Function::ExternalLinkage,
                                     name, TheModule.get());
  int i = 0;
  for (auto &arg : func->args()) {
    arg.setName(parameters[i++]);
  }
  return func;
}

llvm::Function *FuncAST::codegen() {
  auto func = proto.codegen();
  auto block = llvm::BasicBlock::Create(*TheContext, "entry", func);
  Builder->SetInsertPoint(block);
  NamedValues.clear();
  for (auto &arg : func->args()) {
    NamedValues[arg.getName().str()] = &arg;
  }
  auto value = body->codegen();
  Builder->CreateRet(value);
  llvm::verifyFunction(*func);
  TheFPM->run(*func, *TheFAM);
  return func;
}
