#include "parser.hpp"
#include <variant>
namespace ParserNameSpace
{

// ---------------- Iterator ----------------
Iterator::Iterator() : tokenList(nullptr), index(0) {}

Token* Iterator::peeknext()
{
    if (!tokenList) return nullptr;
    if (index + 1 >= tokenList->size()) return nullptr;
    return &(*tokenList)[index+1];
}

Token* Iterator::getNext()
{
    if (!tokenList || tokenList->empty()) return nullptr;
    if (index + 1 < tokenList->size()) {
        index += 1;
    }
    return &(*tokenList)[index];
}

Token* Iterator::getCurr()
{
    if (!tokenList || tokenList->empty()) return nullptr;
    return &(*tokenList)[index];
}

// ---------------- Parser ----------------
Parser::Parser() = default;

void Parser::parserError(const char* message)
{
    Token* curr = iterator.getCurr();
    const std::string name = curr ? curr->name : "<no-token>";
    std::cout << "PARSER ERROR: UNEXPECTED TOKEN "
              << name << " " << message << std::endl;
}

int Parser::parseCreateArg(AST::CreateTableQuery *queryAST)
{
    AST::CreateArg tempArg;

    if(iterator.getNext()->type != TokenType::IDENTIFIER)
    {
        parserError("Expected a table column name");
        return 1;
    }
    tempArg.column = iterator.getCurr()->name;


    if(iterator.getNext()->type != TokenType::SET)
    {
        parserError("Expected a set sign");
        return 1;
    }


    switch(iterator.getNext()->type)
    {
        case TokenType::CHAR_32:
        case TokenType::CHAR_16:
        case TokenType::CHAR_8:
        case TokenType::INT:
        case TokenType::STRING:
            break;
        default:
            parserError("Given type does not exist");
            return 1;
    }
    tempArg.type = iterator.getCurr()->name;

    queryAST->args.push_back(tempArg);

    iterator.getNext();
    if(iterator.getCurr()->type == TokenType::COMMA)
    {
        return parseCreateArg(queryAST);
    }
    else if(iterator.getCurr()->type == TokenType::CLOSING_PARENTHESIS)
    {
        return 0;
    }
    else
    {
        parserError("Column list not initialized");
        return 1;
    }
}

int Parser::parseCreateTable(AST::CreateTableQuery *queryAST)
{

    if(iterator.getNext()->type != TokenType::IDENTIFIER)
    {
        parserError("Expected table name");
        return 1;
    }
    queryAST->tableName = iterator.getCurr()->name;

    if(iterator.getNext()->type != TokenType::OPENING_PARENTHESIS)
    {
        parserError("Expected ( token");
        return 1;
    }

    return parseCreateArg(queryAST);
}

int Parser::parseCreateIndex(AST::CreateIndexQuery *queryAST)
{
    
    if(iterator.getNext()->type != TokenType::KW_ON)
    {
        parserError("Expected ON keyword");
        return 1;
    }


    if(iterator.getNext()->type != TokenType::IDENTIFIER)
    {
        parserError("Expected table name");
        return 1;
    }
    queryAST->tableName = iterator.getCurr()->name;


    if(iterator.getNext()->type != TokenType::OPENING_PARENTHESIS)
    {
        parserError("Expected opening parenthesis");
        return 1;
        
    }


    if(iterator.getNext()->type != TokenType::IDENTIFIER)
    {
        parserError("Expected column name");
        return 1;
    }
    queryAST->column = iterator.getCurr()->name;

    if(iterator.getNext()->type != TokenType::CLOSING_PARENTHESIS)
    {
        parserError("Expected closing parenthesis");
        return 1;
    }

    return 0;
}

ParserReturn Parser::parseCreate()
{
    switch(iterator.getNext()->type)
    {
        case TokenType::KW_TABLE:
        {
            auto query = std::make_unique<AST::CreateTableQuery>();
            query->type = AST::QueryType::CreateTable;

            return {parseCreateTable(query.get()), std::move(query)};
        }
        case TokenType::KW_INDEX:
        {
            auto query = std::make_unique<AST::CreateIndexQuery>();
            query->type = AST::QueryType::CreateIndex;

            return {parseCreateIndex(query.get()), std::move(query)};
        }
        default:
            parserError("Create option not set (table or index)");

            return {1, nullptr};
    }
}

int Parser::parseInsertArgs(AST::InsertQuery* query)
{
    while (true)
    {
        // consume literal
        Token* tok = iterator.getNext();
        if (tok->type != TokenType::LITERAL)
        {
            parserError("Expected column value");
            return 1;
        }

        AST::InsertArg arg;
        arg.value = tok->name;
        query->args.push_back(arg);

        // check next token
        Token* sep = iterator.getNext();
        if (sep->type == TokenType::COMMA)
        {
            // loop to parse next argument
            continue;
        }
        else if (sep->type == TokenType::CLOSING_PARENTHESIS)
        {
            // finished parsing args
            return 0;
        }
        else
        {
            parserError("Expected comma or closing parenthesis");
            return 1;
        }
    }
}



ParserReturn Parser::parseInsert() 
{
    auto query = std::make_unique<AST::InsertQuery>();
    query->type = AST::QueryType::Insert;

    //expect into kw
    if(iterator.getNext()->type != TokenType::KW_INTO)
    {
        parserError("Expected INTO keyword");
        return {1, nullptr};
    }
    //expect table name
    if(iterator.getNext()->type != TokenType::IDENTIFIER)
    {
        parserError("Expected table name");
        return {1, nullptr};
    }
    query.get()->tableName = iterator.getCurr()->name;
    //expect opening parens
    if(iterator.getNext()->type != TokenType::OPENING_PARENTHESIS)
    {
        parserError("Expected a opening parenthesis");
        return {1, nullptr};
    }

    int output = parseInsertArgs(query.get()); 
    //expect semi colon
    if(iterator.getNext()->type != TokenType::SEMICOLON)
    {
        parserError("Expected a semicolon");
        return {1, nullptr};
    }


    return {output, std::move(query)};
}
std::unique_ptr<AST::Expr> Parser::parse_expr(int min_prec) {
    auto left = parse_primary(); // always consume the left-most atomic expression

    while (true) {
        Token* token = iterator.peeknext(); // lookahead without consuming
        if (!token || token->type == TokenType::SEMICOLON) break;
        if (token->type == TokenType::CLOSING_PARENTHESIS) break;
        if (!AST::is_operator(token)) break; // stop if next token is not an operator

        AST::Op op = AST::token_to_op(*token);
        int prec = AST::precedence(op);
        if (prec < min_prec) break;

        iterator.getNext(); // now consume operator
        auto right = parse_expr(prec + 1);

        auto logical = std::make_unique<AST::LogicalExpr>();
        logical->op = op;
        logical->left = std::move(left);
        logical->right = std::move(right);

        left = std::make_unique<AST::Expr>(std::move(logical));
    }

    return left;
}

std::unique_ptr<AST::Expr> Parser::parse_primary() 
{
    auto tok = iterator.getNext();
    switch(tok->type) {
        case TokenType::IDENTIFIER:
            return std::make_unique<AST::Expr>(AST::Expr{AST::ColumnRef{tok->name}});

        case TokenType::LITERAL:
            return std::make_unique<AST::Expr>(AST::Expr{AST::Literal{tok->name}});

        case TokenType::OPENING_PARENTHESIS: {
            auto expr = parse_expr(0);            // parse inside parentheses
            Token* close = iterator.getNext();    // consume the ')'
            if (!close || close->type != TokenType::CLOSING_PARENTHESIS) {
                parserError("Expected closing parenthesis");
            }
            return expr;
        }
        default:
            parserError("Unexpected token in expression");
            return std::make_unique<AST::Expr>(AST::Expr{AST::exprError{}});
    }
}

ParserReturn Parser::parseFind()
{
    auto query = std::make_unique<AST::FindQuery>();
    query->type = AST::QueryType::Find;

    // Expect table name
    Token* tableToken = iterator.getNext();
    if (tableToken->type != TokenType::IDENTIFIER) {
        parserError("Expected table name after FIND");
        return {1, nullptr};
    }
    query->tableName = tableToken->name;

    // Expect WHERE keyword
    Token* whereToken = iterator.getNext();
    if (whereToken->type != TokenType::KW_WHERE) {
        parserError("Expected WHERE keyword");
        return {1, nullptr};
    }

    // Parse the conditions
    auto condition = std::make_unique<AST::Condition>();
    condition->root = parse_expr(0); // precedence-aware parsing

    query->condition = std::move(*condition);

    return {0, std::move(query)};
}

ParserReturn Parser::parse(std::vector<Token> &tokenList)
{
    iterator.tokenList = &tokenList;
    Token* next = iterator.getCurr();

    switch(next->type)
    {
        case TokenType::KW_CREATE:
            return parseCreate();
        case TokenType::KW_INSERT:
            return parseInsert();
        case TokenType::KW_FIND:
            return parseFind(); 
        default:
            parserError("No such query exists");
            break;
    }

    return {1, nullptr};
}

} // namespace ParserNameSpace
