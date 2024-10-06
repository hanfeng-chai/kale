#pragma once

#include <cassert>
#include <cstdlib>
#include <string>

enum Token {
  tok_eof = -1,

  // keywords
  tok_def = -2,
  tok_ext = -3,

  // primary
  tok_id = -4,
  tok_num = -5,

  // keyword
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,
};

struct Lexer {
  std::string id;
  double num;
  int ch = ' ';
  Token tok;

  Lexer() { getNextToken(); }

  bool isBinOp() {
    switch (int(tok)) {
    case '<':
    case '+':
    case '-':
    case '*':
      return true;
    default:
      return false;
    }
  }

  int getOpPrec() {
    assert(isBinOp());
    switch (int(tok)) {
    case '<':
      return 10;
    case '+':
      return 20;
    case '-':
      return 20;
    case '*':
      return 30;
    default:
      assert(0);
    }
  }

  template <typename T> void consume(T token) {
    assert(tok == token);
    getNextToken();
  }

  template <typename T> bool tryConsume(T token) {
    if (tok == token) {
      getNextToken();
      return true;
    }
    return false;
  }

  std::string consumeId() {
    assert(tok == tok_id);
    auto res = id;
    getNextToken();
    return res;
  }

  double consumeNum() {
    assert(tok == tok_num);
    auto res = num;
    getNextToken();
    return res;
  }

  int getToken() { return tok; }

  int popToken() {
    auto res = tok;
    getNextToken();
    return res;
  }

  int getNextToken() {
    while (std::isspace(ch))
      ch = getchar();
    if (ch == '#') {
      while (ch != EOF && ch != '\n')
        ch = getchar();
      return getNextToken();
    }
    if (std::isalpha(ch)) {
      id.clear();
      while (std::isalnum(ch))
        id += ch, ch = getchar();
      if (id == "def")
        return tok = tok_def;
      if (id == "ext")
        return tok = tok_ext;
      if (id == "if")
        return tok = tok_if;
      if (id == "then")
        return tok = tok_then;
      if (id == "else")
        return tok = tok_else;
      return tok = tok_id;
    }
    if (std::isdigit(ch)) {
      std::string nu;
      while (std::isdigit(ch) || ch == '.')
        nu += ch, ch = getchar();
      num = std::strtod(nu.c_str(), 0);
      return tok = tok_num;
    }
    tok = Token(ch);
    ch = getchar();
    return tok;
  }
};
