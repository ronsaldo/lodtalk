#ifndef LODTALK_PARSER_SCANNER_INTERFACE_HPP
#define LODTALK_PARSER_SCANNER_INTERFACE_HPP

#include "Lodtalk/VMContext.hpp"
#include "AST.hpp"

struct ParserScannerExtraData
{
	int startToken;
	int errorCount;
    int columnCount;
	Lodtalk::AST::Node *astResult;
    Lodtalk::VMContext *context;
};

namespace Lodtalk
{
namespace AST
{
Node *parseSourceFromFile(VMContext *context, FILE *input);
Node *parseMethodFromFile(VMContext *context, FILE *input);
Node *parseDoItFromFile(VMContext *context, FILE *input);

} // End of namespace AST
} // End of names Lodtalk

#endif //LODTALK_PARSER_SCANNER_INTERFACE_HPP
