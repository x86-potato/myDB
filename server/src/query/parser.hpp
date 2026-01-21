#pragma once

#include "tokens.hpp"
#include "ast.hpp"

#include <memory>
#include <vector>
#include <iostream>

namespace ParserNameSpace
{

struct ParserReturn
{
    int code = 0;
    std::unique_ptr<AST::Query> queryPointer;
};

struct Iterator
{
public:
    std::vector<Token> *tokenList;
    Iterator();
    Token* peeknext();
    Token* getNext();
    Token* getCurr();

private:
    size_t index = 0;
};

class Parser
{
private:
    Iterator iterator;
    ParserReturn parserReturn;

    void parserError(const char* message);

    int parseCreateArg(AST::CreateTableQuery *queryAST);
    int parseCreateTable(AST::CreateTableQuery *queryAST);
    int parseCreateIndex(AST::CreateIndexQuery *queryAST);

    int parseInsertArgs(AST::InsertQuery *query);

    std::unique_ptr<AST::Expr> parse_primary(const std::string& table_name);
    std::unique_ptr<AST::Expr> parse_expr(int min_prec,const std::string& table_name);
    AST::UpdateExpr parseValue();

    ParserReturn parseShow();
    ParserReturn parseCreate();
    ParserReturn parseInsert();
    ParserReturn parseUpdate();
    ParserReturn parseSelect();
    ParserReturn parseDelete();
    ParserReturn parseLoad();
    ParserReturn parseRun();


public:
    Parser();
    ParserReturn parse(std::vector<Token> &tokenList);
};

} // namespace ParserNameSpace
