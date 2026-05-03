#include <iostream>
#include <cstring>
#include <fstream>
#include "token.h"
#include "scanner.h"
#include <algorithm>
#include <unordered_map>

#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"
#include "token.h"


using namespace std;


// -----------------------------
// Keywords
// -----------------------------
unordered_map<string,Token::Type> keywords = {
    {"select", Token::Type::SELECT},
    {"insert",Token::Type::INSERT},
    {"into",Token::Type::INTO},
    {"values",Token::Type::VALUES},
    {"delete",Token::Type::DELETE},
    {"table",Token::Type::TABLE},
    {"create",Token::Type::CREATE},
    {"from",Token::Type::FROM},
    {"where",Token::Type::WHERE},
    {"in",Token::Type::IN},
    {"on",Token::Type::ON},
    {"between",Token::Type::BETWEEN},
    {"index",  Token::Type::INDEX },
    {"ehash",Token::Type::EHASH},
    {"btree",Token::Type::BTREE},
    {"rtree",Token::Type::RTREE},
    {"and",Token::Type::AND},
    {"radius",Token::Type::RADIUS},
    {"k",Token::Type::K},
    {"int",Token::Type::INT},
    {"float",Token::Type::FLOAT},
    {"point",Token::Type::POINT},
    {"char",Token::Type::CHAR},
    {"primary",Token::Type::PRIMARY},
    {"key",Token::Type::KEY},
    {"incremental",Token::Type::INCREMENTAL}
};


// -----------------------------
// Constructor
// -----------------------------
Scanner::Scanner(const char* s): input(s), first(0), current(0) { 
    }

// -----------------------------
// Función auxiliar
// -----------------------------

bool is_white_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

// -----------------------------
// nextToken: obtiene el siguiente token
// -----------------------------


Token* Scanner::nextToken() {
    Token* token;

    // Saltar espacios en blanco
    bool buscando = true;
    while (buscando && current < input.length()) {
        if (is_white_space(input[current])) {
            current++;
        }
        else if (input[current] == '/' && current + 1 < input.length() && input[current+1] == '/') {
            while (current < input.length() && input[current] != '\n') {
                current++;
            }
        } else {
            buscando = false;
        }
    }

    // Fin de la entrada
    if (current >= input.length()) 
        return new Token(Token::END);

    char c = input[current];

    first = current;

    // Números
    if (isdigit(c)) {
        bool esFloat = false ;
        current++;
        while (current < input.length() && isdigit(input[current])) {
            current++;
        }
        if (current < input.length() && input[current] == '.') {
            if (current + 1 < input.length() && isdigit(input[current+1])) {
                esFloat = true;
                current++;
                while (current < input.length() && isdigit(input[current])) {
                    current++;
                }
            }
        }
        if (esFloat) {
            token = new Token(Token::FLOAT_LIT,input,first,current-first);
        } else {
            token = new Token(Token::INT_LIT, input, first, current - first);
        }
    }
    // ID
    else if (isalpha(c)) {
        string lexaMin;
        lexaMin += (char)tolower(input[current]);
        current++;
        while (current < input.length() && (isalnum(input[current]) || input[current] == '_')) {
            lexaMin += (char)tolower(input[current]);
            current++;
        }
        auto it = keywords.find(lexaMin);
        if (it != keywords.end()) {
            return new Token(it->second,input,first,current - first);
        }
        return new Token(Token::ID, input, first, current - first);
    }
    // STRING    
    else if (c == '"' || c == '\'') {
        char quote = c;
        current++; 

        first = current;

        while (current < input.length() && input[current] != quote) {
            current++;
        }

        if (current >= input.length()) {
            throw runtime_error("String no cerrado");
        }

        int valor = current;

        current++; 

        return new Token(Token::CHAR_LIT, input,first,valor-first);
    }
    // Operadores
    else if (strchr("*()=><!,;/", c)) {
        switch (c) {
            case '(': token = new Token(Token::LPAREN,c); break;
            case ')': token = new Token(Token::RPAREN,c); break;
            case '*': token = new Token(Token::STAR,c); break;
            case '=': token = new Token(Token::EQUAL, c); break;
            case '>': {
                if (input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::GEQ,input,first,current - first); break;

                }
                token = new Token(Token::GER, c); break;
            }
            case '<': {
                if (input[current + 1] == '=') {
                    current++;
                    token = new Token(Token::LEQ,input,first,(current+1) - first); break;
                }
                token = new Token(Token::LES,c);break;
            }
            case '!': token = new Token(Token::NEG,c); break;
            case ';': token = new Token(Token::PCOMA,c); break;
            case ',': token = new Token(Token::COMA,c); break;
            default: token = new Token(Token::ERR,c);
        }
        current++;
    }

    // Carácter inválido
    else {
        token = new Token(Token::ERR, c);
        current++;
    }

    return token;
}




// -----------------------------
// Destructor
// -----------------------------
Scanner::~Scanner() { }

// -----------------------------
// Función de prueba
// -----------------------------

void ejecutar_scanner(Scanner* scanner, const string& InputFile) {
    Token* tok;

    // Crear nombre para archivo de salida
    string OutputFileName = InputFile;
    size_t pos = OutputFileName.find_last_of(".");
    if (pos != string::npos) {
        OutputFileName = OutputFileName.substr(0, pos);
    }
    OutputFileName += "_tokens.txt";

    ofstream outFile(OutputFileName);
    if (!outFile.is_open()) {
        cerr << "Error: no se pudo abrir el archivo " << OutputFileName << endl;
        return;
    }

    outFile << "Scanner\n" << endl;

    while (true) {
        tok = scanner->nextToken();

        if (tok->type == Token::END) {
            outFile << *tok << endl;
            delete tok;
            outFile << "\nScanner exitoso" << endl << endl;
            outFile.close();
            return;
        }

        if (tok->type == Token::ERR) {
            outFile << *tok << endl;
            delete tok;
            outFile << "Caracter invalido" << endl << endl;
            outFile << "Scanner no exitoso" << endl << endl;
            outFile.close();
            return;
        }

        outFile << *tok << endl;
        delete tok;
    }
}
