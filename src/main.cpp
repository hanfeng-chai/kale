#include <iostream>

#include "llvm.h"
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
      extern llvm::Module theModule;
      theModule.print(llvm::errs(), nullptr);
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
