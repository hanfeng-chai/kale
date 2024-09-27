#include <iostream>

#include "ast.h"
#include "jit.h"
#include "llvm.h"
#include "parser.h"

Parser parser;

void handleExt() {
  auto ast = parser.parseExt();
  FunctionProtos.insert_or_assign(ast.name, ast);
  ast.dump();
  std::cerr << std::endl;
  ast.codegen();
  TheModule->print(llvm::errs(), nullptr);
  std::cerr << std::endl;
  auto TSM =
      llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
  ExitOnErr(TheJIT->addModule(std::move(TSM)));
  InitializeModuleAndManagers();
}
void handleDef() {
  auto ast = parser.parseFunc();
  FunctionProtos.insert_or_assign(ast.proto.name, ast.proto);
  ast.dump();
  std::cerr << std::endl;
  ast.codegen();
  TheModule->print(llvm::errs(), nullptr);
  std::cerr << std::endl;
  auto TSM =
      llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
  ExitOnErr(TheJIT->addModule(std::move(TSM)));
  InitializeModuleAndManagers();
}
void handleExp() {
  auto ast = FuncAST(ProtoTypeAST("_expr_", {}), parser.parseExpr());
  ast.body->dump();
  std::cerr << std::endl;
  ast.codegen();
  TheModule->print(llvm::errs(), nullptr);
  std::cerr << std::endl;
  auto RT = TheJIT->getMainJITDylib().createResourceTracker();
  auto TSM =
      llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
  ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
  InitializeModuleAndManagers();
  auto ExprSymbol = ExitOnErr(TheJIT->lookup("_expr_"));
  double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
  std::cerr << FP() << std::endl;
  ExitOnErr(RT->remove());
}

extern "C" double greet(double x) {
  std::cerr << "Hello, " << x << std::endl;
  return x;
}

int main() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
  InitializeModuleAndManagers();
  while (true) {
    switch (parser.getToken()) {
    case tok_eof:
      return 0;
    case tok_ext:
      handleExt();
      break;
    case tok_def:
      handleDef();
      break;
    case ';':
      parser.consume(';');
      break;
    default:
      handleExp();
    }
  }
  return 0;
}
