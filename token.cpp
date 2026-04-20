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
        case Token::STAR:     outs << "TOKEN(*,\"" << tok.text << "\")"; break;
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

        // keywords principales
        // SELECT, INSERT, INTO, VALUES, UPDATE, SET, DELETE,
        case Token::SELECT: outs << "TOKEN(SELECT, \"" << tok.text << "\")"; break;
        case Token::INSERT:   outs << "TOKEN(INSERT, \"" << tok.text << "\")"; break;
        case Token::INTO:   outs << "TOKEN(INTO, \"" << tok.text << "\")"; break;
        case Token::VALUES:   outs << "TOKEN(VALUE, \"" << tok.text << "\")"; break;
        case Token::UPDATE:   outs << "TOKEN(UPDATE, \"" << tok.text << "\")"; break;
        case Token::SET:   outs << "TOKEN(SET, \"" << tok.text << "\")"; break;
        case Token::DELETE:   outs << "TOKEN(DELETE, \"" << tok.text << "\")"; break;

        // keywords de estructura y origen
        // FROM, WHERE, AS, JOIN,
        case Token::FROM: outs << "TOKEN(FROM, \"" << tok.text << "\")"; break;
        case Token::WHERE:   outs << "TOKEN(WHERE, \"" << tok.text << "\")"; break;
        case Token::AS:   outs << "TOKEN(AS, \"" << tok.text << "\")"; break;
        case Token::JOIN:   outs << "TOKEN(JOIN, \"" << tok.text << "\")"; break;

        // Keywords organizacion y agregación
        // GROUP_BY, HAVING,ORDER_BY,ASC,DESC
        case Token::GROUP_BY: outs << "TOKEN(GROUP_BY, \"" << tok.text << "\")"; break;
        case Token::HAVING:   outs << "TOKEN(HAVING, \"" << tok.text << "\")"; break;
        case Token::ORDER_BY:   outs << "TOKEN(ORDER_BY, \"" << tok.text << "\")"; break;
        case Token::ASC:   outs << "TOKEN(ASC, \"" << tok.text << "\")"; break;
        case Token::DESC:   outs << "TOKEN(DESC, \"" << tok.text << "\")"; break;

    }
    return outs;
}

// Para Token puntero
ostream& operator<<(ostream& outs, const Token* tok) {
    if (!tok) return outs << "TOKEN(NULL)";
    return outs << *tok;  // delega al otro
}