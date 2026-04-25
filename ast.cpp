#include "ast.h"
#include <iostream>

using namespace std;

// ------------------ Exp ------------------
Exp::~Exp() {}

string Exp::binopToChar(BinaryOp op) {
    switch (op) {
        case EQUAL_OP:  return "=";
        case LEQ_OP:    return "<=";
        case LES_OP:    return "<";
        case GEQ_OP:    return ">=";
        case GER_OP:    return ">";
        case AND_OP:    return "y";
        default:        return "?";
    }
}

// ------------------ BinaryExp ------------------
BinaryExp::BinaryExp(Exp* l, Exp* r, BinaryOp o)
    : left(l), right(r), op(o) {}

    
BinaryExp::~BinaryExp() {
    delete left;
    delete right;
}


// ------------------ NumberExp ------------------
NumberExp::NumberExp(int v) : value(v) {}

NumberExp::~NumberExp() {}

// ------------------ IdExp ------------------
IdExp::IdExp(string v):value(v) {}

IdExp::~IdExp() {}

// ------------------ UnaryExp ------------------
UnaryExp::UnaryExp(Exp *s, Exp *i):son(s),id(i) {}

UnaryExp::~UnaryExp() {}

// ------------------ BetweenExp ------------------
BetweenEXp::BetweenEXp(Exp* i,Exp* l, Exp* h):id(i),low(l),high(h){}

BetweenEXp::~BetweenEXp() {}

// ------------------ SqrtExp ------------------
SqrtExp::SqrtExp(Exp* v) : value(v) {}

SqrtExp::~SqrtExp() {}

// ------------------ StringExp ------------------
StringExp::StringExp(string s):value(s) {}
StringExp::~StringExp() {}

// ------------------Program-----------------------
Program::Program() {}

Program::~Program() {}

// -----------------------Stmt----------------------
Stmt::~Stmt() {}

// ------------------Select_ Stmt----------------------
SelectStmt::SelectStmt(string table, Exp *where_cond):table(table),where_cond(where_cond) {}

SelectStmt::~SelectStmt() {}

// ------------------Insert_Stmt----------------------
InsertStmt::InsertStmt(string i, list<Exp*> v):table_name(i),values(v) {
}

InsertStmt::~InsertStmt() {
    for (Exp* e:values)
        delete e;
}

// ------------------Delete_stmt----------------------
DeleteStmt::DeleteStmt(string s, Exp *w):table(s),where_cond(w) {}

DeleteStmt::~DeleteStmt() {}
