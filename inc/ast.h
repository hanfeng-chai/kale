#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct AST {
  virtual void dump() = 0;
  virtual ~AST() = default;
};

struct ExprAST : AST {};
using ExprPtr = std::unique_ptr<ExprAST>;
using ExprVec = std::vector<ExprPtr>;

struct NumExprAST : ExprAST {
  double val;
  NumExprAST(double val) : val(val) {}
  virtual void dump() override { std::cerr << val; }
};

struct VarExprAST : ExprAST {
  std::string name;
  VarExprAST(std::string name) : name(std::move(name)) {}
  virtual void dump() override { std::cerr << name; }
};

struct BinExprAST : ExprAST {
  char op;
  ExprPtr lhs;
  ExprPtr rhs;
  BinExprAST(char op, ExprPtr lhs, ExprPtr rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  virtual void dump() override {
    std::cerr << '(';
    lhs->dump();
    std::cerr << op;
    rhs->dump();
    std::cerr << ')';
  }
};

struct CallExprAST : ExprAST {
  std::string callee;
  ExprVec arguments;
  CallExprAST(std::string callee, ExprVec arguments)
      : callee(std::move(callee)), arguments(std::move(arguments)) {}
  virtual void dump() override {
    std::cerr << callee;
    std::cerr << '(';
    int flag = 0;
    for (auto &arg : arguments) {
      if (flag)
        std::cerr << ',';
      arg->dump();
      flag = 1;
    }
    std::cerr << ')';
  }
};

struct ProtoTypeAST : AST {
  std::string name;
  std::vector<std::string> parameters;
  ProtoTypeAST(std::string name, std::vector<std::string> parameters)
      : name(std::move(name)), parameters(std::move(parameters)) {}
  virtual void dump() override {
    std::cerr << name;
    std::cerr << '(';
    int flag = 0;
    for (auto &par : parameters) {
      if (flag)
        std::cerr << ',';
      std::cerr << par;
      flag = 1;
    }
    std::cerr << ')';
    std::cerr << '\n';
  }
};

struct FuncAST : AST {
  ProtoTypeAST proto;
  ExprPtr body;
  FuncAST(ProtoTypeAST proto, ExprPtr body)
      : proto(std::move(proto)), body(std::move(body)) {}
  virtual void dump() override {
    std::cerr << "def ";
    proto.dump();
    body->dump();
  }
};
