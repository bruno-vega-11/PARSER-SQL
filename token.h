#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>
#include "BTree.h"

using namespace std;

class Token {
public:
    // Tipos de token
    enum Type {
        LPAREN,  // (
        RPAREN,  // )
        STAR,    // * del sql
        ERR,     // Error
        ID,      // ID
        PCOMA,   // Punto y coma
        COMA,    // Coma
        BOOL,    // Booleanos
        EQUAL,   // =
        LEQ,     // <=
        LES,     // <
        GEQ,     // >=
        GER,     // >
        NEG,     // !
        AND,     // and
        OR,      // or
        END,      // Fin de entrada

        // Keywords principales
        SELECT, INSERT, INTO, VALUES, DELETE, TABLE, CREATE,

        // New Keywords
        FROM, WHERE, IN, ON, BETWEEN, RADIUS, K,

        // Indices
        INDEX, EHASH, BTREE, RTREE,

        // Tipos de datos
        INT,
        FLOAT,
        POINT,
        CHAR,

        // Literales
        INT_LIT,
        FLOAT_LIT,
        POINT_LIT,
        CHAR_LIT,

        // Constrains
        PRIMARY, KEY, INCREMENTAL
    };

    // Atributos
    Type type;
    string text;

    // Constructores
    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const string& source, int first, int last);

    // Sobrecarga de operadores de salida
    friend ostream& operator<<(ostream& outs, const Token& tok);
    friend ostream& operator<<(ostream& outs, const Token* tok);
};

#endif // TOKEN_H