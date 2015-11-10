%option prefix="Lodtalk_"
%option reentrant bison-bridge bison-locations
%option yylineno
%option noyywrap

%{
#ifdef _WIN32

#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4505 )

#undef INT8_MIN
#undef INT16_MIN
#undef INT32_MIN
#undef UINT8_MIN
#undef UINT16_MIN
#undef UINT32_MIN
#undef INT8_MAX
#undef INT16_MAX
#undef INT32_MAX
#undef UINT8_MAX
#undef UINT16_MAX
#undef UINT32_MAX

#define YY_NO_UNISTD_H

inline int my_isatty(int f)
{
    return 0;
}

inline int my_fileno(FILE *f)
{
    return -1;
}

#define isatty my_isatty
#define fileno my_fileno

#endif 

#include <string.h>
#include <stdlib.h>
#include "ParserScannerInterface.hpp"
#include "Parser.hpp"

#define YYSTYPE LODTALK_STYPE
#define YYLTYPE LODTALK_LTYPE
#define YY_EXTRA_TYPE ParserScannerExtraData *

#define YY_USER_ACTION \
yylloc->first_line = yylloc->last_line = (int)yylineno; \
yylloc->first_column = yyextra->columnCount; yylloc->last_column = int(yyextra->columnCount+yyleng-1); \
    yyextra->columnCount += (int)yyleng;


%}

%%
%{
    yyextra->columnCount = 1;
    if (yyextra->startToken)
    {
        int t = yyextra->startToken;
        yyextra->startToken = 0;
        return t;
    }

%}

\.                              { return DOT; }
\;                              { return SEMICOLON; }
\^                              { return RETURN; }

[ \t]                           {}
[\r]?[\n]                       { yyextra->columnCount = 0; }
[\r]                            { yyextra->columnCount = 0; }
\"[^\"]*\"                      {}

\{                              { return LCBRACKET; }
\}                              { return RCBRACKET; }

\[                              { return LBRACKET; }
\]                              { return RBRACKET; }

\(                              { return LPARENT; }
\)                              { return RPARENT; }
#\(                             { return LIT_BEGIN; }
#\[                             { return BYTE_ARRAY_BEGIN; }
\:=                             { return ASSIGNMENT; }

\|                              { return VERTICAL_BAR; }

nil                             { return KNIL; }
true                            { return KTRUE; }
false                           { return KFALSE; }
self                            { return KSELF; }
super                           { return KSUPER; }
thisContext                     { return KTHIS_CONTEXT; }

\:[_a-zA-Z][_a-zA-Z0-9]*       {
    yylval->stringValue = strdup(yytext + 1);
    return BLOCK_ARGUMENT;
}

#(?:[_a-zA-Z][_a-zA-Z0-9]*\:)+   {
    yylval->stringValue = strdup(yytext + 1);
    return SYMBOL;
}

#[_a-zA-Z][_a-zA-Z0-9]*          {
    yylval->stringValue = strdup(yytext + 1);
    return SYMBOL;
}

#'(?:[^']|'')*'                        {
    size_t symbolLength = strlen(yytext) - 3;
    yylval->stringValue = (char*)malloc(symbolLength + 1);
    strncpy(yylval->stringValue, yytext + 2, symbolLength);
    yylval->stringValue[symbolLength] = 0;
    return SYMBOL;
}

'(?:[^']|'')*'                        {
    size_t symbolLength = strlen(yytext) - 2;
    yylval->stringValue = (char*)malloc(symbolLength + 1);
    strncpy(yylval->stringValue, yytext + 1, symbolLength);
    yylval->stringValue[symbolLength] = 0;
    return STRING;
}

[_a-zA-Z][_a-zA-Z0-9]*       {
    yylval->stringValue = strdup(yytext);
    return IDENTIFIER;
}

[_a-zA-Z][_a-zA-Z0-9]*\:     {
    yylval->stringValue = strdup(yytext);
    return MESSAGE_KEYWORD;
}

-?[0-9]+                      {
    yylval->integerValue = atoi(yytext);
    return INTEGER;
}

-?[0-9]+\.[0-9]+              {
    yylval->floatingValue = atof(yytext);
    return REAL;
}

$.                          {
    yylval->integerValue = yytext[1];
    return CHARACTER;
}

[+\-*/~|,<>=&´?\\?%]+    {
    yylval->stringValue = strdup(yytext);
    return BINARY_SELECTOR;
}

%%
