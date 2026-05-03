#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cout << "Número incorrecto de argumentos.\n";
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "No se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    Scanner scanner(input.c_str());   
    Parser parser(&scanner);        

    Program* ast = nullptr;
    ofstream out("ast.dot");
    out << "digraph AST {\n";
    try {
        ast = parser.parseProgram();
        int id = 0;
        ast->toDot(out, id);
    } catch (const std::exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        ast = nullptr;
        out << "    empty [label=\"AST vacío\"];\n";
    }
    out << "}\n";
    out.close();

    if (ast != nullptr) {
        EVALVisitor eval;             
        for (Stmt* s : ast->stmtList) {
            s->accept(&eval);
        }
        delete ast;
    }

    return 0;
}