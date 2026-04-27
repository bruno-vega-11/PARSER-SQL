#ifndef PARSER_H       
#define PARSER_H

#include "scanner.h"    // Incluye la definición del escáner (provee tokens al parser)
#include "ast.h"        // Incluye las definiciones para construir el Árbol de Sintaxis Abstracta (AST)

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
    Program* parseP();                  // Regla gramatical P
    Program* parseStatementList();      // Regla gramatical
    Stmt* parseStatement();             // Regla gramatical

    Stmt* parseSelectStatement();       // Regla gramatical
    Stmt* parseSelectBody();            // Regla gramatical
    Exp* parseCondicionW();             // Regla gramatical
    Exp* parseCondicionO(Exp* e);       // Regla gramatical
    Exp* parseEntre(Exp* e);            // Regla gramatical
    Exp* parseIn(Exp* e);               // Regla gramatical

    Stmt* parseInsertStatement();       // Regla gramatical
    list<Exp*> parseValueList();        // Regla gramatical
    Exp* parseLiteral();                // Regla gramatical

    Stmt* parseDeleteStatement();       // Regla gramatical

    Stmt* parseCreateStatement();
    Stmt* parseCreateIndex();           // Regla gramatical

    Stmt* parseCreateTable();           // Regla gramatical

};

#endif // PARSER_H      