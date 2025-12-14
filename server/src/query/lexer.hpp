#pragma once

#include "tokens.hpp"
#include <string>
#include <vector>

namespace Lexer
{

Token classify_token(const std::string& token);

void tokenize(const std::string &input, std::vector<Token> &TokenList);

}; 
