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

#ifdef _WIN32
inline char *my_strdup(const char *string)
{
    size_t length = strlen(string);
    char *buffer = (char*)malloc(length + 1);
    strncpy(buffer, string, length);
    buffer[length] = 0;
    return buffer;
}

#define strdup my_strdup
#endif

#endif //LODTALK_PARSER_SCANNER_INTERFACE_HPP
