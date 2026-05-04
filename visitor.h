#ifndef VISITOR_H
#define VISITOR_H
#include "ast.h"
#include "SequentialFile.h"
#include "BPTree.h"

class Visitor {
public:
    virtual void visit(SelectStmt* s) = 0;
    virtual void visit(InsertStmt* s) = 0;
    virtual void visit(CreateIndexStmt* s) = 0;
    virtual void visit(DeleteStmt* s) = 0;
    virtual void visit(CreateTableStmt* s) = 0;
};

class PrintVisitor : public Visitor {
public:
    void imprimir(Exp* program);
};

class EVALVisitor : public Visitor {
public:
    void visit(SelectStmt* s) override;
    void visit(InsertStmt* s) override;
    void visit(CreateIndexStmt* s) override;
    void visit(DeleteStmt* s) override {
        std::cout << "no";

    }
    void visit(CreateTableStmt* s) override;
    void interprete(Exp* program);
};

class AstVisitor : public Visitor {
public:
    ostream out{nullptr};
    int id;
    void arbol(Exp* program);
};

int getTypeSize(const string& tipo);
void serializeField(char* buf, const string& val, const string& tipo);
string deserializeField(const char* buf, const string& tipo);
vector<pair<string,string>> leerSchema(const string& path);
string getIndex(const string& tabla, const string& columna);
string getTipo(const string& raw);
pair<string,string> getIndexInfo(const string& tabla, const string& columna);
string getExpValue(Exp* e);
#endif // VISITOR_H