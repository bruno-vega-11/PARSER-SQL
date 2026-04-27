#include <iostream>
#include <stdexcept>
#include "token.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"

using namespace std;

// =============================
// Métodos de la clase Parser
// =============================

Parser::Parser(Scanner* sc) : scanner(sc) {
    previous = nullptr;
    current = scanner->nextToken();
    if (current->type == Token::ERR) {
        throw runtime_error("Error léxico");
    }
}

void Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
    } else {
        throw runtime_error("Se esperaba otro token");
    };
}

bool Parser::check(Token::Type ttype) {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp = current;
        if (previous) delete previous;
        current = scanner->nextToken();
        previous = temp;

        if (check(Token::ERR)) {
            throw runtime_error("Error lexico");
        }
        return true;
    }
    return false;
}

bool Parser::isAtEnd() {
    return (current->type == Token::END);
}


// =============================
// Reglas gramaticales
// =============================

Program* Parser::parseProgram() {
    Program* ast = parseP(); // *
    if (!isAtEnd()) {
        throw runtime_error("Error sintáctico");
    }
    cout << "Parseo exitoso" << endl;
    return ast;
}


Program* Parser::parseP() {
    Program* p = parseStatementList();
    return p;
}

Program* Parser::parseStatementList() {
    Program* p = new Program();
    p->stmtList.push_back(parseStatement());
    while (check(Token::PCOMA)) {
        match(Token::COMA);
        p->stmtList.push_back(parseStatement());
    }
    return p;
}

Stmt *Parser::parseStatement() {
    Stmt* s;
    if (check(Token::INSERT)) {
        match(Token::INSERT);
        s = parseInsertStatement();
        return s;
    }
    else if (check(Token::DELETE)) {
        match(Token::DELETE);
        s = parseDeleteStatement();
        return s;
    }
    else if (check(Token::CREATE)) {
        match(Token::CREATE);
        s = parseCreateStatement();
        return s;
    } else if (check(Token::SELECT)) {
        match(Token::SELECT);
        s = parseSelectStatement();
        return s;
    } else{
        throw runtime_error("no token esperado");
    }
}

Stmt* Parser::parseSelectStatement() {
    Stmt* s = parseSelectBody();
    return s;
}

Stmt* Parser::parseSelectBody() {
    match(Token::STAR);
    match(Token::FROM);
    match(Token::ID);
    string table = previous->text;
    Exp* condicionW;
    if (check(Token::WHERE)) {
        match(Token::WHERE);
        condicionW = parseCondicionW();
    }
    return new SelectStmt(table,condicionW);
}

Exp* Parser::parseCondicionW() {
    if (check(Token::ID)) {
        match(Token::ID);
        string id = previous->text;
        Exp *e = new IdExp(id);
        if (check(Token::IN)) {
            match(Token::ID);
            return parseIn(e);
        } else if (check(Token::BETWEEN)) {
            match(Token::BETWEEN);
            return parseEntre(e);
        } else {
            return parseCondicionO(e);
        }
    }
    throw runtime_error("?");
}

Exp* Parser::parseCondicionO(Exp* l) {
    BinaryOp op;
    if (check(Token::EQUAL)) {
        match(Token::EQUAL);
        op = BinaryOp::EQUAL_OP;
    } else if (check(Token::LEQ)) {
        match(Token::LEQ);
        op = BinaryOp::LEQ_OP;
    } else if (check(Token::LES)) {
        match(Token::LES);
        op = BinaryOp::LES_OP;
    } else if (check(Token::GEQ)) {
        match(Token::GEQ);
        op = BinaryOp::GEQ_OP;
    } else if (check(Token::GER)) {
        match(Token::GER);
        op = BinaryOp::GER_OP;
    } else {
        throw runtime_error("Condicion no reconocida");
    }

    Exp* r;
    if (check(Token::NUM)) {
        match(Token::NUM);
        r = new NumberExp(stoi(previous->text));
    }
    else {
        throw runtime_error("Se esperaba un numero"); // no implementaod la subconsulta xd
    }
    return new BinaryExp(l,r,op);
}

Exp *Parser::parseEntre(Exp* e) {
    match(Token::NUM);
    Exp* low = new NumberExp(stoi(previous->text));
    match(Token::AND);
    match(Token::NUM);
    Exp* high = new NumberExp(stoi(previous->text));
    return new BetweenEXp(e,low,high);
}

Exp *Parser::parseIn(Exp* e) {
    throw runtime_error("todavia no impelementado xd");
}


Stmt *Parser::parseInsertStatement() {
    match(Token::INTO);
    match(Token::ID);
    string table = previous->text;


    match(Token::VALUES);
    match(Token::LPAREN);

    list<Exp*> values = parseValueList();
    match(Token::RPAREN);
    return new InsertStmt(table,values);
}

list<Exp*> Parser::parseValueList() {
    list<Exp*> list;

    list.push_back(parseLiteral());

    while (check(Token::COMA)) {
        match(Token::COMA);
        list.push_back(parseLiteral());
    }
    return list;
}

Exp *Parser::parseLiteral() {
    if (check(Token::STRING)) {
        match(Token::STRING);
        string val = previous->text;
        return new StringExp(val);
    } else if (check(Token::NUM)) {
        match(Token::NUM);
        int val = stoi(previous->text);
        return new NumberExp(val);
    } else {
        throw runtime_error("Se esperaba un string o un numero");
    }
}

Stmt *Parser::parseDeleteStatement() {
    match(Token::FROM);
    match(Token::ID);
    string table = previous->text;
    match(Token::WHERE);
    Exp* condicionW = parseCondicionW();
    return new DeleteStmt(table,condicionW);
}

Stmt *Parser::parseCreateStatement() {
    Exp* e;
    if (check(Token::INDEX)) {
        match(Token::INDEX);
        return parseCreateIndex();
    } else if (check(Token::TABLE)) {
        match(Token::TABLE);
        return parseCreateTable();
    } else {
        throw runtime_error("Se esperaba un INDEX O TABLE despues del create");
    }
}

Stmt *Parser::parseCreateIndex() {
    BinaryOp op;
    if (check(Token::EHASH)) {
        match(Token::EHASH);
        op = EHASH;
    }
    if (check(Token::BTREE)) {
        match(Token::BTREE);
        op = BTREE;
    }
    if (check(Token::RTREE)) {
        match(Token::RTREE);
        op = RTREE;
    }
    match(Token::ID);
    string colum = previous->text;
    match(Token::ON);
    match(Token::ID);
    string table = previous->text;

    return new CreateIndexStmt(op,colum,table);
}

Stmt *Parser::parseCreateTable() {
    match(Token::ID);
    string table = previous->text;
    match(Token::FROM);
    match(Token::LPAREN);

    match(Token::STRING);
    string path = previous->text;

    match(Token::RPAREN);

    return new CreateTableStmt(table,path);
}

