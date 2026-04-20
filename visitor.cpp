#include <iostream>
#include <fstream>
#include <cmath>
#include "ast.h"
#include "visitor.h"


using namespace std;
unordered_map<std::string, int> memoria;
///////////////////////////////////////////////////////////////////////////////////
int BinaryExp::accept(Visitor* visitor) {
    return visitor->visit(this);
}

int NumberExp::accept(Visitor* visitor) {
    return visitor->visit(this);
}

int SqrtExp::accept(Visitor* visitor) {
    return visitor->visit(this);
}

///////////////////////////////////////////////////////////////////////////////////

int PrintVisitor::visit(BinaryExp* exp) {
    exp->left->accept(this);
    cout << ' ' << Exp::binopToChar(exp->op) << ' ';
    exp->right->accept(this);
    return 0;
}

int PrintVisitor::visit(NumberExp* exp) {
    cout << exp->value;
    return 0;
}

int PrintVisitor::visit(SqrtExp* exp) {
    cout << "sqrt(";
    exp->value->accept(this);
    cout <<  ")";
    return 0;
}


void PrintVisitor::imprimir(Exp* programa){
    if (programa)
    {
        cout << "Codigo:" << endl; 
        programa->accept(this);
        cout << endl;
    }
    return ;
}

///////////////////////////////////////////////////////////////////////////////////
int EVALVisitor::visit(BinaryExp* exp) {
    int result;
    int v1 = exp->left->accept(this);
    int v2 = exp->right->accept(this);
    switch (exp->op) {
        case PLUS_OP:
            result = v1 + v2;
            break;
        case MINUS_OP:
            result = v1 - v2;
            break;
        case MUL_OP:
            result = v1 * v2;
            break;
        case DIV_OP:
            if (v2 != 0)
                result = v1 / v2;
            else {
                cout << "Error: división por cero" << endl;
                result = 0;
            }
            break;
        case POW_OP:
            result = pow(v1,v2);
            break;
        default:
            cout << "Operador desconocido" << endl;
            result = 0;
    }
    return result;
}

int EVALVisitor::visit(NumberExp* exp) {
    return exp->value;
}

int EVALVisitor::visit(SqrtExp* exp) {
    return floor(sqrt( exp->value->accept(this)));
}


void EVALVisitor::interprete(Exp* programa){
    if (programa)
    {
        cout << "Interprete:" << programa->accept(this)<<endl;
    }
    return;

}


///////////////////////////////////////////////////////////////////////////////////
int AstVisitor::visit(BinaryExp* exp) {
    int myId = id++;
            out << "  node" << myId << " [label=\""
                << Exp::binopToChar(exp->op) << "\"];\n";

            if (exp->left) {
                int leftId = id;
                exp->left->accept(this);
                out << "  node" << myId << " -> node" << leftId << ";\n";
            }

            if (exp->right) {
                int rightId = id;
                exp->right->accept(this);
                out << "  node" << myId << " -> node" << rightId << ";\n";
            }

            return 0;
}

int AstVisitor::visit(NumberExp* exp) {
    int myId = id++;
    out << "  node" << myId << " [label=\"" << exp->value << "\"];\n";
    return 0;
}


int AstVisitor::visit(SqrtExp* exp) {
    int myId = id++;
    out << "  node" << myId << " [label=\"sqrt\"];\n";
    if (exp->value) {
        int childId = id;
        exp->value->accept(this);              // genera el nodo hijo
        out << "  node" << myId << " -> node" << childId << ";\n";
    }
    return 0;
}


void AstVisitor::arbol(Exp* programa) {
    ofstream file("ast.dot");
    out.rdbuf(file.rdbuf()); // redirige el ostream interno al archivo
    id = 0;
    out << "digraph AST {\n";
    if (!programa) {
        // Árbol vacío
        out << "  empty [label=\"<arbol vacio>\"];\n";
    } else {
        programa->accept(this);
    }
    out << "}\n";
}
