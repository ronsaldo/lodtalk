#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#include "ParserScannerInterface.hpp"
#include "Parser.hpp"
#define YYSTYPE LODTALK_STYPE
#define YYLTYPE LODTALK_LTYPE
#define YY_EXTRA_TYPE ParserScannerExtraData *
#include "Scanner.hpp"

namespace Lodtalk
{
namespace AST
{

static Node *doParse(yyscan_t scanner, ParserScannerExtraData *extraData)
{
	auto result = Lodtalk_parse(scanner, extraData);
	Lodtalk_lex_destroy(scanner);
	if(result || extraData->errorCount > 0)
        return nullptr;
        
	return extraData->astResult;
}

Node *parseSourceFromFile(FILE *input)
{
    yyscan_t scanner;
    if(Lodtalk_lex_init(&scanner))
        return nullptr;
        
    ParserScannerExtraData extraData;
    memset(&extraData, 0, sizeof(extraData));
    extraData.startToken = SOURCE_FILE;
    Lodtalk_set_extra(&extraData, scanner);
    Lodtalk_set_in(input, scanner);
	return doParse(scanner, &extraData);
}

Node *parseMethodFromFile(FILE *input)
{
    yyscan_t scanner;
    if(Lodtalk_lex_init(&scanner))
        return nullptr;
        
    ParserScannerExtraData extraData;
    memset(&extraData, 0, sizeof(extraData));
    extraData.startToken = METHOD_DEFINITION;
    Lodtalk_set_extra(&extraData, scanner);
    Lodtalk_set_in(input, scanner);
	return doParse(scanner, &extraData);
}

Node *parseDoItFromFile(FILE *input)
{
    yyscan_t scanner;
    if(Lodtalk_lex_init(&scanner))
        return nullptr;
        
    ParserScannerExtraData extraData;
    memset(&extraData, 0, sizeof(extraData));
    extraData.startToken = DO_IT;
    Lodtalk_set_extra(&extraData, scanner);
    Lodtalk_set_in(input, scanner);
	return doParse(scanner, &extraData);	
}

} // End of namespace AST
} // End of names Lodtalk