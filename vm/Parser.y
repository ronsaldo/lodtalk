%define api.pure full
%define api.prefix {Lodtalk_}
%locations
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner } { ParserScannerExtraData *extraData }

%{
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#include "ParserScannerInterface.hpp"
#include "Parser.hpp"
#include "Object.hpp"
#include "Collections.hpp"

#include <string>
#include <string.h>
#include <stdio.h>

using namespace Lodtalk;
using namespace Lodtalk::AST;

extern int yylex(LODTALK_STYPE * yylval_param, LODTALK_LTYPE * yylloc_param , yyscan_t yyscanner);
extern void yyerror(LODTALK_LTYPE * yylloc_param , yyscan_t scanner, ParserScannerExtraData *, const char *message);

inline std::string readStringValue(char* stringValue)
{
    std::string result = stringValue;
    free(stringValue);
    return result;
}

%}

%union {
	int integerValue;
	double floatingValue;
	char *stringValue;
    Lodtalk::AST::Node *node;
    Lodtalk::AST::SequenceNode *sequenceNode;
    Lodtalk::AST::MessageSendNode *messageSendNode;
    Lodtalk::AST::LocalDeclarations *localDeclarations;
    Lodtalk::AST::ArgumentList *argumentList;
    Lodtalk::AST::MethodHeader *methodHeader;
}

%token LBRACKET RBRACKET
%token LCBRACKET RCBRACKET
%token LPARENT RPARENT
%token LIT_BEGIN
%token BYTE_ARRAY_BEGIN
%token DOT SEMICOLON
%token RETURN
%token ASSIGNMENT
%token VERTICAL_BAR

%token KNIL KTRUE KFALSE
%token KSELF KSUPER KTHIS_CONTEXT

%token METHOD_DEFINITION DO_IT SOURCE_FILE

%token<integerValue> INTEGER
%token<floatingValue> REAL
%token<stringValue> STRING
%token<integerValue> CHARACTER
%token<stringValue> IDENTIFIER
%token<stringValue> SYMBOL
%token<stringValue> MESSAGE_KEYWORD
%token<stringValue> BINARY_SELECTOR
%token<stringValue> BLOCK_ARGUMENT

%type<stringValue> binarySelector

%type<node> statement expression operand literal top assignment referenceExpression
%type<node> unaryMessage binaryMessage returnStatement block specialIdentifiers
%type<node> method methodPragmas
%type<messageSendNode> messageChainElement messageChain
%type<sequenceNode> statementList statementListNonEmpty blockContent

%type<localDeclarations> localList localDeclarations
%type<argumentList> blockArguments blockArgumentList

%type<methodHeader> methodHeader keywordMethodHeader

%type<node> sourceFileStatement
%type<sequenceNode> sourceFile sourceFileStatementList

%%

top: DO_IT blockContent { extraData->astResult = $2; }
   | METHOD_DEFINITION method { extraData->astResult = $2; }
   | SOURCE_FILE sourceFile { extraData->astResult = $2; }
   ;

sourceFile: sourceFileStatementList {$$ = $1; }
          | sourceFileStatementList DOT {$$ = $1; }
          |                         {$$ = new SequenceNode(); }
          ;

sourceFileStatementList: sourceFileStatement                                {$$ = new SequenceNode($1); }
                       | sourceFileStatementList DOT sourceFileStatement    {
                            $$ = $1;
                            $$->addStatement($3);
                       }
                       ;

sourceFileStatement: statement  { $$ = $1; }
                   | unaryMessage IDENTIFIER LBRACKET method RBRACKET  { $$ = new MessageSendNode(readStringValue($2) + ":", $1, $4); }
                   ;

method: methodHeader methodPragmas blockContent { $$ = new MethodAST($1, $2, $3); }
      ;

methodHeader: IDENTIFIER                    { $$ = new MethodHeader(readStringValue($1)); }
            | binarySelector IDENTIFIER    { $$ = new MethodHeader(readStringValue($1), new ArgumentList(new Argument(readStringValue($2)))); }
            | keywordMethodHeader           { $$ = $1; }
            ;

keywordMethodHeader: MESSAGE_KEYWORD IDENTIFIER                         { $$ = new MethodHeader(readStringValue($1), new ArgumentList(new Argument(readStringValue($2)))); }
                   | keywordMethodHeader MESSAGE_KEYWORD IDENTIFIER     {
                        $$ =  $1;
                        $$->appendSelectorAndArgument(readStringValue($2), new Argument(readStringValue($3)));
                   }
                   ;

methodPragmas: { $$ = nullptr; }
             ;

block: LBRACKET blockArguments blockContent RBRACKET  { $$ = new BlockExpression($2, $3); }
     ;

blockArguments: blockArgumentList VERTICAL_BAR
              | { $$ = nullptr; }
              ;

blockArgumentList: BLOCK_ARGUMENT                       { $$ = new ArgumentList(new Argument(readStringValue($1))); }
                 | blockArgumentList BLOCK_ARGUMENT     {
                    $$ = $1;
                    $$->appendArgument(new Argument(readStringValue($2)));
                 }
                 ;

blockContent: statementList { $$ = $1; }
            | localDeclarations statementList {
                $$ = $2;
                $$->setLocalDeclarations($1);
            }
            ;

localDeclarations: VERTICAL_BAR localList VERTICAL_BAR  { $$ = $2; }
                 ;

localList: localList IDENTIFIER {
            $$ = $1;
            $$->appendLocal(new LocalDeclaration(readStringValue($2)));
         }
         |  { $$ = new LocalDeclarations(); }
         ;

statementListNonEmpty: statementListNonEmpty DOT statement {
        $$ = $1;
        $1->addStatement($3);
    }
    | statement { $$ = new SequenceNode($1);}
    ;

statementList: statementListNonEmpty { $$ = $1; }
             | statementListNonEmpty DOT { $$ = $1; }
             |                       {$$ = new SequenceNode(); }
             ;

statement: expression { $$ = $1; }
         | returnStatement { $$ = $1; }
         ;

expression: assignment      { $$ = $1; }
          | binaryMessage   { $$ = $1; }
          | messageChain    { $$ = $1; }
          ;

assignment: referenceExpression ASSIGNMENT expression { $$ = new AssignmentExpression($1, $3); }
          ;

operand: literal { $$ = $1; }
       | specialIdentifiers { $$ = $1; }
       | referenceExpression { $$ = $1; }
       | LPARENT statementList RPARENT { $$ = $2; }
       | block                         { $$ = $1; }
       ;

unaryMessage: operand                   { $$ = $1; }
            | unaryMessage IDENTIFIER   { $$ = new MessageSendNode(readStringValue($2), $1); }
            ;

binaryMessage: unaryMessage                   { $$ = $1; }
            | binaryMessage binarySelector unaryMessage   { $$ = new MessageSendNode(readStringValue($2), $1, $3); }
            ;

messageChainElement: MESSAGE_KEYWORD binaryMessage { $$ = new MessageSendNode(readStringValue($1), nullptr, $2); }
            | messageChainElement MESSAGE_KEYWORD binaryMessage  {
                $$ = $1;
                $$->appendSelector(readStringValue($2));
                $$->appendArgument($3);
            }
            ;

messageChain: binaryMessage messageChainElement {
                $$ = $2;
                $$->setReceiver($1);
            }
            | messageChain SEMICOLON messageChainElement {
                $$ = $1;
                $$->appendChained($3);
            }
            | messageChain SEMICOLON IDENTIFIER {
                $$ = $1;
                $$->appendChained(new MessageSendNode(readStringValue($3), nullptr));
            }
            ;

referenceExpression: IDENTIFIER { $$ = new IdentifierExpression(readStringValue($1)); }
                   ;

returnStatement: RETURN expression { $$ = new ReturnStatement($2); }
               ;

specialIdentifiers: KSELF           { $$ = new SelfReference(); }
                  | KSUPER          { $$ = new SuperReference(); }
                  | KTRUE           { $$ = new LiteralNode(&TrueObject); }
                  | KFALSE          { $$ = new LiteralNode(&FalseObject); }
                  | KNIL            { $$ = new LiteralNode(&NilObject); }
                  | KTHIS_CONTEXT   { $$ = new ThisContextReference(); }
                  ;

literal: INTEGER    { $$ = new LiteralNode(signedInt64ObjectFor($1)); }
    | REAL          { $$ = new LiteralNode(floatObjectFor($1)); }
    | STRING        { $$ = new LiteralNode(ByteString::fromNative(readStringValue($1)).getOop()); }
    | CHARACTER     { $$ = new LiteralNode(Oop::encodeCharacter($1)); }
    | SYMBOL        { $$ = new LiteralNode(ByteSymbol::fromNative(readStringValue($1))); }
    ;

binarySelector: BINARY_SELECTOR { $$ = $1; }
              | VERTICAL_BAR    { $$ = strdup("|"); }
              ;

%%

void yyerror(LODTALK_LTYPE *yylloc_param , yyscan_t scanner, ParserScannerExtraData *extraData, const char *message)
{
    ++extraData->errorCount;
    fprintf(stderr, "%d.%d-%d.%d: %s\n", yylloc_param->first_line, yylloc_param->first_column, yylloc_param->last_line, yylloc_param->last_column, message);
}
