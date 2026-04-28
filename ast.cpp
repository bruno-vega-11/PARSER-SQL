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

void BinaryExp::toDot(std::ostream &out, int &id) const {
    int myId = id++;
    out << "  node" << myId << " [label=\"" << Exp::binopToChar(op) << "\", shape=diamond];\n";

    if (left) {
        int leftId = id;
        left->toDot(out, id);
        out << "  node" << myId << " -> node" << leftId << ";\n";
    }
    if (right) {
        int rightId = id;
        right->toDot(out, id);
        out << "  node" << myId << " -> node" << rightId << ";\n";
    }
}
// ------------------ NumberExp ------------------
NumberExp::NumberExp(int v) : value(v) {}

NumberExp::~NumberExp() {}

void NumberExp::toDot(std::ostream &out, int &id) const {
    out << "  node" << id << " [label=\"" << value << "\", shape=ellipse, color=blue];\n";
    id++;
}


// ------------------ IdExp ------------------
IdExp::IdExp(string v):value(v) {}

IdExp::~IdExp() {}

void IdExp::toDot(std::ostream &out, int &id) const {
    out << "  node" << id << " [label=\"ID: " << value << "\", shape=ellipse, color=green];\n";
    id++;
}

// ------------------ BetweenExp ------------------
BetweenEXp::BetweenEXp(Exp* i,Exp* l, Exp* h):id(i),low(l),high(h){}

BetweenEXp::~BetweenEXp() {}

void BetweenEXp::toDot(std::ostream &out, int &id) const {
    int myId = id++;
    out << "  node" << myId << " [label=\"BETWEEN\", shape=box, style=filled, fillcolor=lightblue];\n";

    // Nodo del campo (ID)
    int fieldId = id;
    this->id->toDot(out, id);
    out << "  node" << myId << " -> node" << fieldId << " [label=\"field\"];\n";

    // Nodo Low
    int lowId = id;
    low->toDot(out, id);
    out << "  node" << myId << " -> node" << lowId << " [label=\"low\"];\n";

    // Nodo High
    int highId = id;
    high->toDot(out, id);
    out << "  node" << myId << " -> node" << highId << " [label=\"high\"];\n";
}

// ------------------ StringExp ------------------
StringExp::StringExp(string s):value(s) {}

StringExp::~StringExp() {}

void StringExp::toDot(std::ostream &out, int &id) const {
    out << "  node" << id << " [label=\"\\\"" << value << "\\\"\", shape=ellipse, color=orange];\n";
    id++;
}

// ------------------Program-----------------------
Program::Program() {}

Program::~Program() {}

void Program::toDot(ostream &out, int &id) const {
    int myId = id++;
    // Nodo raíz del programa
    out << "  node" << myId << " [label=\"PROGRAM\", shape=box3d, style=filled, fillcolor=lightgrey];\n";

    // Recorremos la lista de sentencias
    for (Stmt* s : stmtList) {
        if (s) {
            int stmtId = id; // Guardamos el ID que tendrá el siguiente nodo
            s->toDot(out, id);
            // Conectamos la raíz con cada sentencia
            out << "  node" << myId << " -> node" << stmtId << ";\n";
        }
    }
}


// -----------------------Stmt----------------------
Stmt::~Stmt() {}

// ------------------Select_ Stmt----------------------
SelectStmt::SelectStmt(string table, Exp *where_cond):table(table),where_cond(where_cond) {}

SelectStmt::~SelectStmt() {}

void SelectStmt::toDot(std::ostream &out, int &id) const {
    int myId = id++;
    out << "  node" << myId << " [label=\"SELECT *\\nFROM: " << table << "\", shape=rect, style=filled, fillcolor=lightyellow];\n";

    if (where_cond) {
        int condId = id;
        where_cond->toDot(out, id);
        out << "  node" << myId << " -> node" << condId << " [label=\"WHERE\"];\n";
    }
}


// ------------------Insert_Stmt----------------------
InsertStmt::InsertStmt(string i, list<Exp*> v):table_name(i),values(v) {
}

InsertStmt::~InsertStmt() {
    for (Exp* e:values)
        delete e;
}

void InsertStmt::toDot(std::ostream &out, int &id) const {
    int myId = id++;
    out << "  node" << myId << " [label=\"INSERT INTO\\n" << table_name << "\", shape=rect, style=filled, fillcolor=lightgreen];\n";

    int valuesId = id++;
    out << "  node" << valuesId << " [label=\"VALUES\", shape=circle];\n";
    out << "  node" << myId << " -> node" << valuesId << ";\n";

    for (Exp* e : values) {
        int valId = id;
        e->toDot(out, id);
        out << "  node" << valuesId << " -> node" << valId << ";\n";
    }
}


// ------------------Delete_stmt----------------------
DeleteStmt::DeleteStmt(string s, Exp *w):table(s),where_cond(w) {}

DeleteStmt::~DeleteStmt() {}

void DeleteStmt::toDot(std::ostream &out, int &id) const {
    int myId = id++;
    out << "  node" << myId << " [label=\"DELETE FROM\\n" << table << "\", shape=rect, style=filled, fillcolor=lightcoral];\n";

    if (where_cond) {
        int condId = id;
        where_cond->toDot(out, id);
        out << "  node" << myId << " -> node" << condId << " [label=\"WHERE\"];\n";
    }
}

// -----------------------CreateIndexStmt----------------------
CreateIndexStmt::CreateIndexStmt(BinaryOp o,string a,string b):op(o),indexName(a),tableName(b) {}

CreateIndexStmt::~CreateIndexStmt() {}

void CreateIndexStmt::toDot(std::ostream& out, int& id) const {

};
// -----------------------CreateTableStmt----------------------
CreateTableStmt::CreateTableStmt(string t, string p):tabla(t),path(p) {}

CreateTableStmt::~CreateTableStmt() {}

void CreateTableStmt::toDot(std::ostream &out, int &id) const {

}

// -----------------------PointExp----------------------
PointExp::PointExp(double _x, double _y):x(_x),y(_y) {}

PointExp::~PointExp() {}

void PointExp::toDot(std::ostream &out, int &id) const {}

// -----------------------SpatialRadiusExp----------------------
SpatialRaidusExp::SpatialRaidusExp(Exp *co, PointExp *ce, double ra):column(co),center(ce),radius(ra) {}

SpatialRaidusExp::~SpatialRaidusExp() {delete column; delete center;}

void SpatialRaidusExp::toDot(std::ostream &out, int &id) const {}

// -----------------------SpatialKnnExp----------------------
SpatialKnnExp::SpatialKnnExp(Exp *co, PointExp *ce, int _k):column(co),center(ce),k(_k) {}

SpatialKnnExp::~SpatialKnnExp() {delete column; delete center;}

void SpatialKnnExp::toDot(std::ostream &out, int &id) const {}