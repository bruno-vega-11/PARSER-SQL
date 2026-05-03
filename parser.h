#ifndef PARSER_H       
#define PARSER_H

#include "scanner.h"    // Incluye la definición del escáner (provee tokens al parser)
#include "ast.h"        // Incluye las definiciones para construir el Árbol de Sintaxis Abstracta (AST)
#include "BTree.h"

class Parser {
private:
    Scanner* scanner;       // Puntero al escáner, de donde se leen los tokens
    Token *current, *previous; // Punteros al token actual y al anterior
    void match(Token::Type ttype);   // Verifica si el token actual coincide con un tipo esperado y avanza si es así
    bool check(Token::Type ttype);   // Comprueba si el token actual es de cierto tipo, sin avanzar
    bool advance();                  // Avanza al siguiente token
    bool isAtEnd();                  // Comprueba si ya se llegó al final de la entrada
public:
    Parser(Scanner* scanner);       
    Program* parseProgram();            // Punto de entrada: analiza un programa completo
    Program* parseP();                  // Regla gramatical Program
    Program* parseStatementList();      // Regla gramatical StmtList
    Stmt* parseStatement();             // Regla gramatical Stmt

    Stmt* parseSelectStatement();       // Regla gramatical Select_Stmt
    Stmt* parseSelectBody();            // Regla gramatical Select_body
    Exp* parseCondicionW();             // Regla gramatical Condicion_w
    Exp* parseCondicionO(Exp* e);       // Regla gramatical Comp_op
    Exp* parseEntre(Exp* e);            // Regla gramatical Entre
    Exp* parseIn(Exp* e);               // Regla gramatical In

    Stmt* parseInsertStatement();       // Regla gramatical Ins_Stmt
    list<Exp*> parseValueList();        // Regla gramatical Value_list
    Exp* parseLiteral();                // Regla gramatical Literal

    Stmt* parseDeleteStatement();       // Regla gramatical Del_Stmt

    Stmt* parseCreateStatement();       // Regla gramatical Create_Stmt
    Stmt* parseCreateIndex();           // Regla gramatical Create_index
    Stmt* parseCreateTable();           // Regla gramatical Create_table

};

#endif // PARSER_H      