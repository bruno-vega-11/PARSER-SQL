#ifndef AST_H
#define AST_H

#include <string>
#include <list>
#include <ostream>

class Stmt;

using namespace std;

class Visitor;

// Operadores binarios soportados
enum BinaryOp {
    EQUAL_OP, // =
    LEQ_OP, // <=
    LES_OP, // <
    GEQ_OP, // >=
    GER_OP, // >
    AND_OP,
};

enum IndexType {
    EHASH,
    BTREE,
    RTREE,
};

// Clase abstracta Exp
class Exp {
public:
    //virtual int  accept(Visitor* visitor) = 0;
    virtual ~Exp() = 0;  // Destructor puro → clase abstracta
    virtual void toDot(std::ostream& out, int& id) const = 0;  // Visualización en DOT
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
    void toDot(std::ostream& out, int& id) const override;
};

// Expresión numérica entero
class IntExp : public Exp {
public:
    int value;
    //int accept(Visitor* visitor);
    IntExp(int v);
    ~IntExp();
    void toDot(std::ostream& out, int& id) const override;
};

// Expresion numerica float
class FloatExp: public Exp {
public:
    float value;
    // int accept(Visitor* visitor);
    FloatExp(float v);
    ~FloatExp();
    void toDot(std::ostream &out, int &id) const override;
};

// identificador
class IdExp: public Exp {
public:
    string value;
    //int accept(Visitor* visitor);
    IdExp(string v);
    ~IdExp();
    void toDot(std::ostream& out, int& id) const override;
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
    void toDot(std::ostream& out, int& id) const override;
};

// Expresion string
class StringExp : public Exp {
public:
    string value;
    //int accept(Visitor* visitor); // modificar urgente
    StringExp(string s);
    ~StringExp();
    void toDot(std::ostream& out, int& id) const override;
};

// Expresion para el punto
class PointExp: public Exp {
public:
    double x,y;
    PointExp(double _x,double _y);
    ~PointExp();
    void toDot(std::ostream &out, int &id) const override;
};

// Expresion para busqueda por radio
class SpatialRaidusExp: public  Exp {
public:
    Exp* column;       // id de la columna
    PointExp* center;
    double radius;
    SpatialRaidusExp(Exp* co,PointExp* ce,double ra);
    ~SpatialRaidusExp();
    void toDot(std::ostream &out, int &id) const override;
};

// Expresion para busqueda k-nn
class SpatialKnnExp: public Exp {
public:
    Exp* column;
    PointExp* center;
    int k;
    SpatialKnnExp(Exp* co,PointExp* ce,int _k);
    ~SpatialKnnExp();
    void toDot(std::ostream &out, int &id) const override;
};

// Clase que define al programa

class Program {
public:
    list<Stmt*> stmtList;
    //void accept(Visitor visitor);
    ~Program();
    Program();
    void toDot(ostream& out, int& id) const;
};


// Clase que define los statements
class Stmt {
public:
    //virtual void accept(Visitor* visitor) = 0;
    virtual ~Stmt() = 0;
    virtual void toDot(std::ostream& out, int& id) const = 0;
};

class SelectStmt: public Stmt {
public:
    string table;
    Exp* where_cond;
    //void accept(Visitor* visitor) override;
    SelectStmt(string table, Exp* where_cond = nullptr);
    ~SelectStmt();
    void toDot(std::ostream& out, int& id) const override;

};

class InsertStmt: public Stmt {
public:
    string table_name;
    list<Exp*> values;
    //void accept(Visitor* visitor) override;
    InsertStmt(string i,list<Exp*> v);
    ~InsertStmt();
    void toDot(std::ostream& out, int& id) const override;

};

class DeleteStmt: public Stmt {
public:
    string table;
    Exp* where_cond;
    //void accept(Visitor* visitor) override;
    DeleteStmt(string s,Exp* w);
    ~DeleteStmt();
    void toDot(std::ostream& out, int& id) const override;

};

class CreateIndexStmt: public Stmt {
public:
    IndexType op;
    string indexName;
    string tableName;
    
    //void accept(Visitor* visitor) override;
    CreateIndexStmt(IndexType o,string i,string t);
    ~CreateIndexStmt();
    void toDot(std::ostream& out, int& id) const override;

};

class CreateTableStmt: public Stmt {
public:
    string tabla;
    string path;
    list<pair<string,string>> columns;

    //void accept(Visitor* visitor) override;
    CreateTableStmt(string t,string p, list<pair<string,string>> c);
    ~CreateTableStmt();
    void toDot(std::ostream& out, int& id) const override;
};

#endif // AST_H
