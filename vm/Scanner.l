%option prefix="Lodtalk_"
%option reentrant bison-bridge bison-locations
%option yylineno
%option noyywrap

%{
#include <string.h>
#include <stdlib.h>
#include "ParserScannerInterface.hpp"
#include "Parser.hpp"

#define YYSTYPE LODTALK_STYPE
#define YYLTYPE LODTALK_LTYPE
#define YY_EXTRA_TYPE ParserScannerExtraData *

#define YY_USER_ACTION \
yylloc->first_line = yylloc->last_line = yylineno; \
yylloc->first_column = yyextra->columnCount; yylloc->last_column = yyextra->columnCount+yyleng-1; \
    yyextra->columnCount += yyleng;
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
