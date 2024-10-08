#pragma once

#include "ast.h"
#include "lexer.h"

#include <memory>

struct Parser : Lexer {
  ExprPtr parseNumExpr() { return std::make_unique<NumExprAST>(consumeNum()); }

  ExprPtr parseVarOrCallExpr() {
    auto name = consumeId();
    if (tryConsume('(')) {
      ExprVec arguments;
      if (!tryConsume(')')) {
        while (true) {
          if (auto arg = parseExpr()) {
            arguments.push_back(std::move(arg));
            if (tryConsume(')'))
              break;
            if (tryConsume(','))
              continue;
            return nullptr;
          } else {
            return nullptr;
          }
        }
      }
      return std::make_unique<CallExprAST>(std::move(name),
                                           std::move(arguments));
    } else {
      return std::make_unique<VarExprAST>(std::move(name));
    }
  }

  ExprPtr parseIfExpr() {
    consume(tok_if);
    if (auto Cond = parseExpr()) {
      consume(tok_then);
      if (auto Then = parseExpr()) {
        consume(tok_else);
        if (auto Else = parseExpr()) {
          return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                             std::move(Else));
        }
      }
    }
    return nullptr;
  }

  ExprPtr parseForExpr() {
    consume(tok_for);
    auto name = consumeId();
    consume('=');
    if (auto Init = parseExpr()) {
      consume(',');
      if (auto Cond = parseExpr()) {
        consume(',');
        if (auto Next = parseExpr()) {
          consume(tok_in);
          if (auto Body = parseExpr())
            return std::make_unique<ForExprAST>(
                std::move(name), std::move(Init), std::move(Cond),
                std::move(Next), std::move(Body));
        }
      }
    }
    return nullptr;
  }

  ExprPtr parseParenExpr() {
    consume('(');
    if (auto expr = parseExpr()) {
      consume(')');
      return expr;
    } else {
      return nullptr;
    }
  }

  ExprPtr parsePrimaryExpr() {
    switch (getToken()) {
    case tok_num:
      return parseNumExpr();
    case tok_id:
      return parseVarOrCallExpr();
    case tok_if:
      return parseIfExpr();
    case tok_for:
      return parseForExpr();
    case '(':
      return parseParenExpr();
    default:
      return nullptr;
    }
  }

  ExprPtr parseBinExpr(ExprPtr lhs, int prec) {
    while (isBinOp() && prec < getOpPrec()) {
      int newPrec = getOpPrec();
      char op = popToken();
      if (auto rhs = parseExpr(newPrec)) {
        lhs = std::make_unique<BinExprAST>(op, std::move(lhs), std::move(rhs));
      } else {
        return nullptr;
      }
    }
    return lhs;
  }

  ExprPtr parseExpr(int prec = 0) {
    if (auto lhs = parsePrimaryExpr()) {
      return parseBinExpr(std::move(lhs), prec);
    } else {
      return nullptr;
    }
  }

  ProtoTypeAST parseProtoType() {
    auto name = consumeId();
    std::vector<std::string> parameters;
    consume('(');
    if (!tryConsume(')')) {
      while (true) {
        parameters.push_back(consumeId());
        if (tryConsume(')'))
          break;
        if (tryConsume(','))
          continue;
        assert(0);
      }
    }
    return ProtoTypeAST(std::move(name), std::move(parameters));
  }

  ProtoTypeAST parseExt() {
    consume(tok_ext);
    return parseProtoType();
  }

  FuncAST parseFunc() {
    consume(tok_def);
    auto proto = parseProtoType();
    auto body = parseExpr();
    return FuncAST(std::move(proto), std::move(body));
  }
};
