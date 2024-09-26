#include <iostream>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include "parser.h"

Parser parser;

void handleExt() {
  auto ast = parser.parseExt();
  ast.dump();
  std::cerr << std::endl;
  ast.codegen()->print(llvm::errs());
  std::cerr << std::endl;
}
void handleDef() {
  auto ast = parser.parseFunc();
  ast.dump();
  std::cerr << std::endl;
  ast.codegen()->print(llvm::errs());
  std::cerr << std::endl;
}
void handleExp() {
  auto ast = parser.parseExpr();
  ast->dump();
  std::cerr << std::endl;
  ast->codegen()->print(llvm::errs());
  std::cerr << std::endl;
}

int main() {
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
