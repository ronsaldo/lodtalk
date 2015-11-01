#ifndef LODTALK_AST_HPP_
#define LODTALK_AST_HPP_

#include <memory>
#include <vector>
#include <string>
#include "Object.hpp"
#include "Collections.hpp"
#include "Collections.hpp"

namespace Lodtalk
{
class MethodASTHandle;
class VariableLookup;
class TemporalVariableLookup;
typedef std::shared_ptr<VariableLookup> VariableLookupPtr;
typedef std::shared_ptr<TemporalVariableLookup> TemporalVariableLookupPtr;

namespace AST
{
class Node;

class Argument;
class ArgumentList;
class AssignmentExpression;
class BlockExpression;
class IdentifierExpression;
class LiteralNode;
class LocalDeclarations;
class LocalDeclaration;
class MethodAST;
class MethodHeader;
class MessageSendNode;
class ReturnStatement;
class SelfReference;
class SequenceNode;
class SuperReference;
class ThisContextReference;

/**
 * AST visitor
 */
class ASTVisitor
{
public:
	virtual Oop visitArgument(Argument *node) = 0;
	virtual Oop visitArgumentList(ArgumentList *node) = 0;
	virtual Oop visitAssignmentExpression(AssignmentExpression *node) = 0;
	virtual Oop visitBlockExpression(BlockExpression *node) = 0;
	virtual Oop visitIdentifierExpression(IdentifierExpression *node) = 0;
	virtual Oop visitLiteralNode(LiteralNode *node) = 0;
	virtual Oop visitLocalDeclarations(LocalDeclarations *node) = 0;
	virtual Oop visitLocalDeclaration(LocalDeclaration *node) = 0;
	virtual Oop visitMessageSendNode(MessageSendNode *node) = 0;
	virtual Oop visitMethodAST(MethodAST *node) = 0;
	virtual Oop visitMethodHeader(MethodHeader *node) = 0;
	virtual Oop visitReturnStatement(ReturnStatement *node) = 0;
	virtual Oop visitSequenceNode(SequenceNode *node) = 0;
	virtual Oop visitSelfReference(SelfReference *node) = 0;
	virtual Oop visitSuperReference(SuperReference *node) = 0;
	virtual Oop visitThisContextReference(ThisContextReference *node) = 0;
};

/**
 * AST node
 */
class Node
{
public:
	Node();
	virtual ~Node();

	virtual Oop acceptVisitor(ASTVisitor *visitor) = 0;

	virtual bool isIdentifierExpression() const;
    virtual bool isBlockExpression() const;
	virtual bool isReturnStatement() const;
    virtual bool isSuperReference() const;
};

/**
 * Literal node
 */
class LiteralNode: public Node
{
public:
    LiteralNode(Oop value);
	LiteralNode(ProtoObject *value);
	~LiteralNode();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	Oop getValue() const;

private:
	OopRef valueRef;
};

/**
 * Message send
 */
class MessageSendNode: public Node
{
public:
	MessageSendNode(const std::string &selector, Node *receiver);
	MessageSendNode(const std::string &selector, Node *receiver, Node *firstArgument);
	~MessageSendNode();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::string &getSelector() const;
	Oop getSelectorOop() const;

	const std::vector<Node*> &getArguments() const;

	Node *getReceiver() const;
	void setReceiver(Node *newReceiver);

	const std::vector<MessageSendNode*> &getChainedMessages() const;

	void appendSelector(const std::string &selectorExtra);
	void appendArgument(Node *newArgument);
	void appendChained(MessageSendNode *chainedMessage);

private:
	std::string selector;
	Node *receiver;
	std::vector<Node*> arguments;
	std::vector<MessageSendNode*> chainedMessages;
};

/**
 * Identifier expression
 */
class IdentifierExpression: public Node
{
public:
	IdentifierExpression(const std::string &identifier);
	~IdentifierExpression();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	virtual bool isIdentifierExpression() const;

	const std::string &getIdentifier() const;
	Oop getSymbol() const;

    const VariableLookupPtr &getVariable() const;
    void setVariable(const VariableLookupPtr &newVariable);

private:
	std::string identifier;
    VariableLookupPtr variable;
};

/**
 * Assignment expression node
 */
class AssignmentExpression: public Node
{
public:
	AssignmentExpression(Node *reference, Node *value);
	~AssignmentExpression();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	Node *getReference() const;
	Node *getValue() const;

private:
	Node *reference;
	Node *value;
};

/**
 * Sequence node
 */
class SequenceNode: public Node
{
public:
	SequenceNode(Node *first=nullptr);
	~SequenceNode();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	void addStatement(Node *node);

	const std::vector<Node*> &getChildren() const;

	LocalDeclarations *getLocalDeclarations() const;
	void setLocalDeclarations(LocalDeclarations *newLocals);

private:
	std::vector<Node*> children;
	LocalDeclarations *localDeclarations;
};

/**
 * Return statement
 */
class ReturnStatement: public Node
{
public:
	ReturnStatement(Node *value);
	~ReturnStatement();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	virtual bool isReturnStatement() const;

	Node *getValue() const;

private:
	Node *value;
};

/**
 * Local variable declaration.
 */
class LocalDeclaration: public Node
{
public:
	LocalDeclaration(const std::string &name);
	~LocalDeclaration();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::string &getName() const;

    Oop getSymbolOop();

private:
	std::string name;
};

/**
 * Local declarations.
 */
class LocalDeclarations: public Node
{
public:
	LocalDeclarations();
	~LocalDeclarations();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::vector<LocalDeclaration*> &getLocals() const;

	void appendLocal(LocalDeclaration *local);

private:
	std::vector<LocalDeclaration*> locals;
};

/**
 * Argument
 */
class Argument: public Node
{
public:
	Argument(const std::string &name);
	~Argument();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::string &getName();

	Oop getSymbolOop();

private:
	std::string name;
};

/**
 * Argument list
 */
class ArgumentList: public Node
{
public:
	ArgumentList(Argument *firstArgument);
	~ArgumentList();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::vector<Argument*> &getArguments();

	void appendArgument(Argument *argument);

private:
	std::vector<Argument*> arguments;
};


/**
 * Closure or method node.
 */
class FunctionalNode: public Node
{
public:
    typedef std::vector<TemporalVariableLookupPtr> LocalVariables;

    void setLocalVariables(const LocalVariables &newLocalVariables);
    const LocalVariables &getLocalVariables() const;

    virtual ArgumentList *getArgumentList() const = 0;
    size_t getArgumentCount() const;

private:
    LocalVariables localVariables;
};

/**
 * Block expression
 */
class BlockExpression: public FunctionalNode
{
public:
    typedef std::vector<TemporalVariableLookupPtr> InlineArguments;

	BlockExpression(ArgumentList *argumentList, SequenceNode *body);
	~BlockExpression();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

    virtual bool isBlockExpression() const;

    virtual bool isInlined() const;
    virtual void setInlined(bool newInlined);

	ArgumentList *getArgumentList() const;
	SequenceNode *getBody() const;

    void addInlineArgument(const TemporalVariableLookupPtr &variable);
    const InlineArguments &getInlineArguments() const;
    
private:
	ArgumentList *argumentList;
	SequenceNode *body;
    bool isInlined_;

    InlineArguments inlineArguments;
};

/**
 * Method header
 */
class MethodHeader: public Node
{
public:
	MethodHeader(const std::string &selector, ArgumentList *arguments = nullptr);
	~MethodHeader();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	const std::string getSelector() const;

	ArgumentList *getArgumentList() const;

	void appendSelectorAndArgument(const std::string &selector, Argument *argument);

private:
	std::string selector;
	ArgumentList *arguments;
};

/**
 * Method AST
 */
class MethodAST: public FunctionalNode
{
public:

	MethodAST(MethodHeader *header, Node *pragmas, SequenceNode *body);
	~MethodAST();

	virtual Oop acceptVisitor(ASTVisitor *visitor);

	MethodHeader *getHeader() const;
	SequenceNode *getBody() const;

    virtual ArgumentList *getArgumentList() const;

	const Ref<MethodASTHandle> &getHandle();

private:
	MethodHeader *header;
	SequenceNode *body;
	Ref<MethodASTHandle> astHandle;
};

/**
 * Self reference
 */
class SelfReference: public Node
{
public:
	virtual Oop acceptVisitor(ASTVisitor *visitor);
};

/**
 * Super reference
 */
class SuperReference: public Node
{
public:
	virtual Oop acceptVisitor(ASTVisitor *visitor);

    virtual bool isSuperReference() const;
};

/**
 * ThisContext reference
 */
class ThisContextReference: public Node
{
public:
	virtual Oop acceptVisitor(ASTVisitor *visitor);
};


} // End of namespace AST
} // End of namespace Lodtalk

#endif //LODTALK_AST_HPP_
