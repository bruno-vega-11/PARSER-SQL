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
        match(Token::PCOMA);
        if (!isAtEnd()) {
            p->stmtList.push_back(parseStatement());
        }
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
        return new SelectStmt(table,condicionW);
    }
    return new SelectStmt(table);
}

Exp* Parser::parseCondicionW() {
    if (check(Token::ID)) {
        match(Token::ID);
        string id = previous->text;
        Exp *e = new IdExp(id);
        if (check(Token::IN)) {
            match(Token::IN);
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
    // Exp* e = column
    match(Token::LPAREN);
    match(Token::POINT);
    match(Token::LPAREN);
    match(Token::NUM);
    double x = stod(previous->text);
    match(Token::COMA);
    match(Token::NUM);
    double y = stod(previous->text);
    match(Token::RPAREN);

    PointExp* center = new PointExp(x,y);

    match(Token::COMA);
    if (check(Token::RADIUS)) {
        match(Token::RADIUS);
        match(Token::NUM);
        double r = stod(previous->text);
        match(Token::RPAREN);
        return new SpatialRaidusExp(e,center,r);
    } else if (check(Token::K)) {
        match(Token::K);
        match(Token::NUM);
        int k = stoi(previous->text);
        match(Token::RPAREN);
        return new SpatialKnnExp(e,center,k);
    } else {
        throw runtime_error("se esperaba otro token");
    }
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
    IndexType op;
    if (check(Token::EHASH)) {
        match(Token::EHASH);
        op = EHASH;
    }
    else if (check(Token::BTREE)) {
        match(Token::BTREE);
        op = BTREE;
    }
    else if (check(Token::RTREE)) {
        match(Token::RTREE);
        op = RTREE;
    } else throw runtime_error("Indice no reconocido");
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

    match(Token::LPAREN);
    list<pair<string,string>> columns = parseColumn_list();
    match(Token::RPAREN);

    return new CreateTableStmt(table,path,columns);
}

list<pair<string,string>> Parser::parseColumn_list() {
    list<pair<string,string>> lista;

    lista.push_back(parseColumn_def());
    while (check(Token::COMA)) {
        match(Token::COMA);
        lista.push_back(parseColumn_def());
    }
    return lista;
}

pair<string, string> Parser::parseColumn_def() {
    match(Token::ID);
    string name = previous->text;
    string type = parseType();
    string constrains = parseConstrains();

    return {name ,type + constrains};
}

string Parser::parseType() {
    if (check(Token::INT)) {
        match(Token::INT);
        return "INT";
    }
    if (check(Token::FLOAT)) {
        match(Token::FLOAT);
        return "FLOAT";
    }
    if (check(Token::STRING)) {
        match(Token::STRING);
        return "STRING";
    }
    if (check(Token::POINT)) {
        match(Token::POINT);
        return "POINT";
    }
    throw runtime_error("Tipo de dato no reconocido");
}

string Parser::parseConstrains() {
    if (check(Token::PRIMARY)){
        match(Token::PRIMARY);
        match(Token::KEY);
        return " PK";
    } else if (check(Token::INCREMENTAL)) {
        match(Token::INCREMENTAL);
        return " INCREMENTAL";
    }
    return "";
}
