#ifndef AST_H
#define AST_H

#include <string>
#include <unordered_map>
#include <list>
#include <ostream>

class Stmt;

using namespace std;

//class Visitor;

// Operadores binarios soportados
enum BinaryOp {
    EQUAL_OP, // =
    LEQ_OP, // <=
    LES_OP, // <
    GEQ_OP, // >=
    GER_OP, // >
    AND_OP
};

// Clase abstracta Exp
class Exp {
public:
    //virtual int  accept(Visitor* visitor) = 0;
    virtual ~Exp() = 0;  // Destructor puro → clase abstracta
    static string binopToChar(BinaryOp op);  // Conversión operador → string
};

// Expresión binaria
class BinaryExp : public Exp {
public:
    Exp* left;
    Exp* right;
    BinaryOp op;
    //int accept(Visitor* visitor);
    BinaryExp(Exp* l, Exp* r, BinaryOp op);
    ~BinaryExp();

};

// Expresión numérica
class NumberExp : public Exp {
public:
    int value;
    //int accept(Visitor* visitor);
    NumberExp(int v);
    ~NumberExp();
};

// Raiz cuadrada
class SqrtExp : public Exp {
public:
    Exp* value;
    //int accept(Visitor* visitor);
    SqrtExp(Exp* v);
    ~SqrtExp();
};

// identificador
class IdExp: public Exp {
public:
    string value;
    //int accept(Visitor* visitor);
    IdExp(string v);
    ~IdExp();
};

//
class UnaryExp: public Exp {
public:
    Exp* son;
    Exp* id;
    //int accept(Visitor *visitor);
    UnaryExp(Exp* s, Exp* i);
    ~UnaryExp();
};

// Bewteen expresion
class BetweenEXp: public Exp {
public:
    Exp* id;
    Exp* low;
    Exp* high;
    //int accept(Visitor *visitor);
    BetweenEXp(Exp* i,Exp* l, Exp* h);
    ~BetweenEXp();
};

// Expresion string
class StringExp : public Exp {
public:
    string value;
    //int accept(Visitor* visitor); // modificar urgente
    StringExp(string s);
    ~StringExp();
};

// Clase que define al programa

class Program {
public:
    list<Stmt*> stmtList;
    //void accept(Visitor visitor);
    ~Program();
    Program();
};


// Clase que define los statements
class Stmt {
public:
    //virtual void accept(Visitor visitor) = 0;
    virtual ~Stmt() = 0;
};

class SelectStmt: public Stmt {
public:
    string table;
    Exp* where_cond;
    //void accept(Visitor visitor) override;
    SelectStmt(string table, Exp* where_cond = nullptr);
    ~SelectStmt();
};

class InsertStmt: public Stmt {
public:
    string table_name;
    list<Exp*> values;
    //void accept(Visitor visitor) override;
    InsertStmt(string i,list<Exp*> v);
    ~InsertStmt();
};

class DeleteStmt: public Stmt {
public:
    string table;
    Exp* where_cond;
    //void accept(Visitor visitor) override;
    DeleteStmt(string s,Exp* w);
    ~DeleteStmt();
};

#endif // AST_H
