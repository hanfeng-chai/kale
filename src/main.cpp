#include <iostream>

#include "parser.h"

Parser parser;

void handleExt() {
  parser.parseExt().dump();
  std::cerr << std::endl;
}
void handleDef() {
  parser.parseFunc().dump();
  std::cerr << std::endl;
}
void handleExp() {
  parser.parseExpr()->dump();
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
