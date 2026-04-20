#include <iostream>
#include "token.h"

using namespace std;

// -----------------------------
// Constructores
// -----------------------------

Token::Token(Type type) 
    : type(type), text("") { }

Token::Token(Type type, char c) 
    : type(type), text(string(1, c)) { }

Token::Token(Type type, const string& source, int first, int last) 
    : type(type), text(source.substr(first, last)) { }

// -----------------------------
// Sobrecarga de operador <<
// -----------------------------

// Para Token por referencia
ostream& operator<<(ostream& outs, const Token& tok) {
    switch (tok.type) {
        case Token::LPAREN:    outs << "TOKEN(LPAREN, \""    << tok.text << "\")"; break;
        case Token::RPAREN:    outs << "TOKEN(RPAREN, \""    << tok.text << "\")"; break;
        case Token::STRING:    outs << "TOKEN(STRING, \""    << tok.text << "\")"; break;
        case Token::ID:    outs << "TOKEN(ID, \""    << tok.text << "\")"; break;
        case Token::COMA:   outs << "TOKEN(COMA, \"" << tok.text << "\""; break;
        case Token::PCOMA:  outs << "TOKEN(PCOMA, \"" << tok.text << "\""; break;
        case Token::STAR:     outs << "TOKEN(*,\"" << tok.text << "\""; break;
        case Token::NUM:    outs << "TOKEN(NUM, \""    << tok.text << "\")"; break;
        case Token::BOOL:   outs << "TOKEN(BOOL, \"" << tok.text << "\")"; break;
        case Token::EQUAL:   outs << "TOKEN(EQUAL, \"" << tok.text << "\")"; break;
        case Token::LEQ:   outs << "TOKEN(<=, \"" << tok.text << "\")"; break;
        case Token::LES:   outs << "TOKEN(<, \"" << tok.text << "\")"; break;
        case Token::GEQ:   outs << "TOKEN(>=, \"" << tok.text << "\")"; break;
        case Token::GER:   outs << "TOKEN(>, \"" << tok.text << "\")"; break;
        case Token::NEG:   outs << "TOKEN(!, \"" << tok.text << "\")"; break;
        case Token::AND:   outs << "TOKEN(AND, \"" << tok.text << "\")"; break;
        case Token::OR:    outs << "TOKEN(OR,  \"" << tok.text << "\")" ; break;
        case Token::ERR:    outs << "TOKEN(ERR, \""    << tok.text << "\")"; break;
        case Token::END:    outs << "TOKEN(END)"; break;
    }
    return outs;
}

// Para Token puntero
ostream& operator<<(ostream& outs, const Token* tok) {
    if (!tok) return outs << "TOKEN(NULL)";
    return outs << *tok;  // delega al otro
}