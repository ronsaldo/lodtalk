#ifndef LODTALK_PARSER_SCANNER_INTERFACE_HPP
#define LODTALK_PARSER_SCANNER_INTERFACE_HPP

#include "AST.hpp"

struct ParserScannerExtraData
{
	int startToken;
	int errorCount;
    int columnCount;
	Lodtalk::AST::Node *astResult;
};

namespace Lodtalk
{
namespace AST
{
Node *parseSourceFromFile(FILE *input);
Node *parseMethodFromFile(FILE *input);
Node *parseDoItFromFile(FILE *input);

} // End of namespace AST
} // End of names Lodtalk

#endif //LODTALK_PARSER_SCANNER_INTERFACE_HPP
