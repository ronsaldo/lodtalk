#include "AST.hpp"
#include "Compiler.hpp"

namespace Lodtalk
{
namespace AST
{
Node::Node()
{
}

Node::~Node()
{
}

bool Node::isIdentifierExpression() const
{
	return false;
}

bool Node::isReturnStatement() const
{
	return false;
}

bool Node::isSuperReference() const
{
    return false;
}

bool Node::isBlockExpression() const
{
    return false;
}

// Identifier expression
IdentifierExpression::IdentifierExpression(const std::string &identifier)
	: identifier(identifier)
{
}

IdentifierExpression::~IdentifierExpression()
{
}

Oop IdentifierExpression::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitIdentifierExpression(this);
}

bool IdentifierExpression::isIdentifierExpression() const
{
	return true;
}

const std::string &IdentifierExpression::getIdentifier() const
{
	return identifier;
}

const VariableLookupPtr &IdentifierExpression::getVariable() const
{
    return variable;
}

void IdentifierExpression::setVariable(const VariableLookupPtr &newVariable)
{
    variable = newVariable;
}

Oop IdentifierExpression::getSymbol() const
{
	return makeByteSymbol(identifier);
}

// Literal node
LiteralNode::LiteralNode(Oop value)
    : valueRef(value)
{
}

LiteralNode::LiteralNode(ProtoObject *value)
    : valueRef(Oop::fromPointer(value))
{
}

LiteralNode::~LiteralNode()
{
}

Oop LiteralNode::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitLiteralNode(this);
}

Oop LiteralNode::getValue() const
{
	return valueRef.oop;
}

// Message send
MessageSendNode::MessageSendNode(const std::string &selector, Node *receiver)
	: selector(selector), receiver(receiver)
{
}

MessageSendNode::MessageSendNode(const std::string &selector, Node *receiver, Node *firstArgument)
	: selector(selector), receiver(receiver)
{
	arguments.push_back(firstArgument);
}

MessageSendNode::~MessageSendNode()
{
	delete receiver;
	for(auto &arg : arguments)
		delete arg;
}

Oop MessageSendNode::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitMessageSendNode(this);
}

const std::string &MessageSendNode::getSelector() const
{
	return selector;
}

Oop MessageSendNode::getSelectorOop() const
{
	return makeSelector(selector);
}

Node *MessageSendNode::getReceiver() const
{
	return receiver;
}

void MessageSendNode::setReceiver(Node *newReceiver)
{
	receiver = newReceiver;
}

const std::vector<Node*> &MessageSendNode::getArguments() const
{
	return arguments;
}

const std::vector<MessageSendNode*> &MessageSendNode::getChainedMessages() const
{
	return chainedMessages;
}

void MessageSendNode::appendSelector(const std::string &selectorExtra)
{
	selector += selectorExtra;
}

void MessageSendNode::appendArgument(Node *newArgument)
{
	arguments.push_back(newArgument);
}

void MessageSendNode::appendChained(MessageSendNode *chainedMessage)
{
	chainedMessages.push_back(chainedMessage);
}

// Assignment expression
AssignmentExpression::AssignmentExpression(Node *reference, Node *value)
	: reference(reference), value(value)
{
}

AssignmentExpression::~AssignmentExpression()
{
	delete reference;
	delete value;
}

Oop AssignmentExpression::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitAssignmentExpression(this);
}

Node *AssignmentExpression::getReference() const
{
	return reference;
}

Node *AssignmentExpression::getValue() const
{
	return value;
}

// Sequence node
SequenceNode::SequenceNode(Node *first)
	: localDeclarations(nullptr)
{
	if(first)
		children.push_back(first);
}

SequenceNode::~SequenceNode()
{
	for(auto &child : children)
		delete child;
}

Oop SequenceNode::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitSequenceNode(this);
}

void SequenceNode::addStatement(Node *node)
{
	children.push_back(node);
}

const std::vector<Node*> &SequenceNode::getChildren() const
{
	return children;
}

LocalDeclarations *SequenceNode::getLocalDeclarations() const
{
	return localDeclarations;
}

void SequenceNode::setLocalDeclarations(LocalDeclarations *newLocals)
{
	localDeclarations = newLocals;
}

// Return statement
ReturnStatement::ReturnStatement(Node *value)
	: value(value)
{
}

ReturnStatement::~ReturnStatement()
{
	delete value;
}

Oop ReturnStatement::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitReturnStatement(this);
}

bool ReturnStatement::isReturnStatement() const
{
	return true;
}

Node *ReturnStatement::getValue() const
{
	return value;
}

// Local declaration
LocalDeclaration::LocalDeclaration(const std::string &name)
	: name(name)
{
}

LocalDeclaration::~LocalDeclaration()
{
}

Oop LocalDeclaration::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitLocalDeclaration(this);
}

const std::string &LocalDeclaration::getName() const
{
	return name;
}

Oop LocalDeclaration::getSymbolOop()
{
    return makeByteSymbol(name);
}

// Local declarations
LocalDeclarations::LocalDeclarations()
{
}

LocalDeclarations::~LocalDeclarations()
{
}

Oop LocalDeclarations::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitLocalDeclarations(this);
}

const std::vector<LocalDeclaration*> &LocalDeclarations::getLocals() const
{
	return locals;
}

void LocalDeclarations::appendLocal(LocalDeclaration *local)
{
	locals.push_back(local);
}

// Argument
Argument::Argument(const std::string &name)
	: name(name)
{
}

Argument::~Argument()
{
}

Oop Argument::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitArgument(this);
}

const std::string &Argument::getName()
{
	return name;
}

Oop Argument::getSymbolOop()
{
	return makeByteSymbol(name);
}

// Argument list
ArgumentList::ArgumentList(Argument *firstArgument)
{
	arguments.push_back(firstArgument);
}

ArgumentList::~ArgumentList()
{
	for(auto &child : arguments)
		delete child;
}

Oop ArgumentList::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitArgumentList(this);
}

const std::vector<Argument*> &ArgumentList::getArguments()
{
	return arguments;
}

void ArgumentList::appendArgument(Argument *argument)
{
	arguments.push_back(argument);
}

// Block expression
BlockExpression::BlockExpression(ArgumentList *argumentList, SequenceNode *body)
	: argumentList(argumentList), body(body), isInlined_(false)
{
}

BlockExpression::~BlockExpression()
{
	delete argumentList;
	delete body;
}

Oop BlockExpression::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitBlockExpression(this);
}

ArgumentList *BlockExpression::getArgumentList() const
{
	return argumentList;
}

SequenceNode *BlockExpression::getBody() const
{
	return body;
}

bool BlockExpression::isBlockExpression() const
{
    return true;
}

bool BlockExpression::isInlined() const
{
    return isInlined_;
}

void BlockExpression::setInlined(bool newInlined)
{
    isInlined_ = newInlined;
}

void BlockExpression::addInlineArgument(const TemporalVariableLookupPtr &variable)
{
    inlineArguments.push_back(variable);
}

const BlockExpression::InlineArguments &BlockExpression::getInlineArguments() const
{
    return inlineArguments;
}

// FunctionalNode
void FunctionalNode::setLocalVariables(const LocalVariables &newLocalVariables)
{
    localVariables = newLocalVariables;
}

const FunctionalNode::LocalVariables &FunctionalNode::getLocalVariables() const
{
    return localVariables;
}

size_t FunctionalNode::getArgumentCount() const
{
    auto arguments = getArgumentList();
    return arguments ? arguments->getArguments().size() : 0;
}

// Method header
MethodHeader::MethodHeader(const std::string &selector, ArgumentList *arguments)
	: selector(selector), arguments(arguments)
{
}

MethodHeader::~MethodHeader()
{
	delete arguments;
}

Oop MethodHeader::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitMethodHeader(this);
}

const std::string MethodHeader::getSelector() const
{
	return selector;
}

ArgumentList *MethodHeader::getArgumentList() const
{
	return arguments;
}

void MethodHeader::appendSelectorAndArgument(const std::string &selectorExtra, Argument *argument)
{
	selector += selectorExtra;
	arguments->appendArgument(argument);
}

// Method AST
MethodAST::MethodAST(MethodHeader *header, Node *pragmas, SequenceNode *body)
	: header(header), body(body)
{
}

MethodAST::~MethodAST()
{
	delete header;
	delete body;
}

Oop MethodAST::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitMethodAST(this);
}

MethodHeader *MethodAST::getHeader() const
{
	return header;
}

SequenceNode *MethodAST::getBody() const
{
	return body;
}

ArgumentList *MethodAST::getArgumentList() const
{
    return header ? header->getArgumentList() : nullptr;
}

const Ref<MethodASTHandle> &MethodAST::getHandle()
{
	if(astHandle.isNil())
	{
		astHandle.reset(reinterpret_cast<MethodASTHandle*> (MethodASTHandle::ClassObject->basicNativeNew(sizeof(void*))));
		if(!astHandle.isNil())
			astHandle->ast = this;
	}
	return astHandle;
}

// Self reference
Oop SelfReference::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitSelfReference(this);
}

// Super reference
Oop SuperReference::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitSuperReference(this);
}

bool SuperReference::isSuperReference() const
{
    return true;
}

// This context reference
Oop ThisContextReference::acceptVisitor(ASTVisitor *visitor)
{
	return visitor->visitThisContextReference(this);
}

} // End of namespace AST
} // End of namespace Lodtalk
