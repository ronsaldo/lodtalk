#include <map>
#include <stdio.h>
#include <stdarg.h>
#include "Compiler.hpp"
#include "Method.hpp"
#include "MethodBuilder.hpp"
#include "ParserScannerInterface.hpp"
#include "Exception.hpp"
#include "FileSystem.hpp"
#include "RAII.hpp"

namespace Lodtalk
{

using namespace AST;

// Variable lookup description
class VariableLookup
{
public:
	VariableLookup() {}
	virtual ~VariableLookup() {}

    virtual bool isTemporal() const
    {
        return false;
    }

	virtual bool isMutable() const = 0;

	virtual Oop getValue() = 0;
	virtual void setValue(Oop newValue) = 0;

	virtual void generateLoad( MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const = 0;
    virtual void generateStore( MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const = 0;
};

// Evaluation scope class
class EvaluationScope;
typedef std::shared_ptr<EvaluationScope> EvaluationScopePtr;

class EvaluationScope
{
public:
	EvaluationScope(const EvaluationScopePtr &parentScope);
	virtual ~EvaluationScope();

	virtual VariableLookupPtr lookSymbol(Oop symbol) = 0;

	VariableLookupPtr lookSymbolRecursively(Oop symbol);

	const EvaluationScopePtr &getParent() const
	{
		return parentScope;
	}

private:
	EvaluationScopePtr parentScope;
};

EvaluationScope::EvaluationScope(const EvaluationScopePtr &parentScope)
	: parentScope(parentScope)
{
}

EvaluationScope::~EvaluationScope()
{
}

VariableLookupPtr EvaluationScope::lookSymbolRecursively(Oop symbol)
{
	auto result = lookSymbol(symbol);
	if(result || !parentScope)
		return result;
	return parentScope->lookSymbolRecursively(symbol);
}

// Literal variable lookup
class LiteralVariableLookup: public VariableLookup
{
public:
	LiteralVariableLookup(Oop literalVariable)
		: variable(reinterpret_cast<LiteralVariable*> (literalVariable.pointer)) {}
	~LiteralVariableLookup() {}

	virtual bool isMutable() const
	{
		return true;
	}

	virtual Oop getValue()
	{
		return variable->value;
	}

	virtual void setValue(Oop newValue)
	{
		variable->value = newValue;
	}

	virtual void generateLoad(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
	{
		gen.pushLiteralVariable(variable.getOop());
	}

    virtual void generateStore(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
	{
		gen.storeLiteralVariable(variable.getOop());
	}

	Ref<LiteralVariable> variable;
};

// Instance variable lookup
class InstanceVariableLookup: public VariableLookup
{
public:
	InstanceVariableLookup(int instanceVariableIndex)
		: instanceVariableIndex(instanceVariableIndex) {}
	~InstanceVariableLookup() {}

	virtual bool isMutable() const
	{
		return true;
	}

	virtual Oop getValue()
	{
		abort();
	}

	virtual void setValue(Oop newValue)
	{
		abort();
	}

	virtual void generateLoad(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
	{
		gen.pushReceiverVariableIndex(instanceVariableIndex);
	}

    virtual void generateStore(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
	{
		gen.storeReceiverVariableIndex(instanceVariableIndex);
	}

	int instanceVariableIndex;
};

// Temporal variable lookup
class TemporalVariableLookup: public VariableLookup
{
public:
	TemporalVariableLookup(Node *localContext, bool isMutable_)
		: temporalIndex(-1), temporalVectorIndex(-1), localContext(localContext), isMutable_(isMutable_), isCapturedInClosure_(false) {}
	~TemporalVariableLookup() {}

    virtual bool isTemporal() const
    {
        return true;
    }

	virtual bool isMutable() const
	{
		return true;
	}

	virtual Oop getValue()
	{
		abort();
	}

	virtual void setValue(Oop newValue)
	{
		abort();
	}

	virtual void generateLoad(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
	{
        assert(temporalIndex >= 0);
        if(temporalVectorIndex >= 0)
        {
            gen.pushTemporalInVector(temporalIndex, temporalVectorIndex + functionalContext->getArgumentCount());
        }
        else
        {
            gen.pushTemporal(temporalIndex);
        }
	}

    virtual void generateStore(MethodAssembler::Assembler &gen, FunctionalNode *functionalContext) const
    {
        assert(isMutable());
        assert(temporalIndex >= 0);
        if(temporalVectorIndex >= 0)
        {
            gen.storeTemporalInVector(temporalIndex, temporalVectorIndex + functionalContext->getArgumentCount());
        }
        else
        {
            gen.storeTemporal(temporalIndex);
        }
    }

    void setTemporalIndex(int newIndex)
    {
        temporalIndex = newIndex;
    }

    int getTemporalIndex()
    {
        return temporalIndex;
    }

    void setTemporalVectorIndex(int newTemporalVectorIndex)
    {
        temporalVectorIndex = newTemporalVectorIndex;
    }

    int getTemporalVectorIndex()
    {
        return temporalVectorIndex;
    }

    Node *getLocalContext()
    {
        return localContext;
    }

    bool isCapturedInClosure()
    {
        return isCapturedInClosure_;
    }

    void setCapturedInClosure(bool newValue)
    {
        isCapturedInClosure_ = newValue;
    }

private:

	int temporalIndex;
    int temporalVectorIndex;
    Node *localContext;
	bool isMutable_;
    bool isCapturedInClosure_;
};
typedef std::shared_ptr<TemporalVariableLookup> TemporalVariableLookupPtr;

// Global variable evaluation scope
class GlobalEvaluationScope: public EvaluationScope
{
public:
	GlobalEvaluationScope()
		: EvaluationScope(EvaluationScopePtr()) {}

	virtual VariableLookupPtr lookSymbol(Oop symbol);
};

VariableLookupPtr GlobalEvaluationScope::lookSymbol(Oop symbol)
{
	auto globalVar = getGlobalFromSymbol(symbol);
	if(globalVar.isNil())
		return VariableLookupPtr();

	return std::make_shared<LiteralVariableLookup> (globalVar);
}

// Instance variable scope
class InstanceVariableScope: public EvaluationScope
{
public:
	InstanceVariableScope(const EvaluationScopePtr &parent, ClassDescription *classDesc)
		: EvaluationScope(parent), classDesc(classDesc) {}

	virtual VariableLookupPtr lookSymbol(Oop symbol);

private:
	std::pair<int, int> findInstanceVariable(ClassDescription *pos, Oop symbol);

	Ref<ClassDescription> classDesc;
	std::map<OopRef, VariableLookupPtr> lookupCache;
};

VariableLookupPtr InstanceVariableScope::lookSymbol(Oop symbol)
{
	auto it = lookupCache.find(symbol);
	if(it != lookupCache.end())
		return it->second;

	auto res = findInstanceVariable(classDesc.get(), symbol);
	if(res.first < 0)
		return VariableLookupPtr();

	auto instanceVariable = std::make_shared<InstanceVariableLookup> (res.first);
	lookupCache[symbol] = instanceVariable;
	return instanceVariable;
}

std::pair<int, int> InstanceVariableScope::findInstanceVariable(ClassDescription *pos, Oop symbol)
{
	// Find the superclass first
	auto instanceCount = 0;
	auto super = pos->superclass;
	if(!isNil(super))
	{
		auto superInstance = findInstanceVariable(reinterpret_cast<ClassDescription*> (super), symbol);
		if(superInstance.first >= 0)
			return superInstance;
		instanceCount = superInstance.second;
	}

	// Find the symbol here
	auto instanceVarNames = pos->instanceVariables;
	auto instanceVarIndex = -1;
	if(!instanceVarNames.isNil())
	{
		size_t count = instanceVarNames.getNumberOfElements();
		auto nameArray = reinterpret_cast<Oop*> (instanceVarNames.getFirstFieldPointer());
		for(size_t i = 0; i < count; ++i)
		{
			if(nameArray[i] == symbol)
			{
				instanceVarIndex = i + instanceCount;
				break;
			}
		}
		instanceCount += count;
	}

	return std::make_pair(instanceVarIndex, instanceCount);
}

// Local variable scope
class LocalScope: public EvaluationScope
{
public:
	LocalScope(const EvaluationScopePtr &parent)
		: EvaluationScope(parent) {}

	TemporalVariableLookupPtr addArgument(Oop symbol, Node *localContext);
	TemporalVariableLookupPtr addTemporal(Oop symbol, Node *localContext);

	virtual VariableLookupPtr lookSymbol(Oop symbol);

private:
	std::map<OopRef, VariableLookupPtr> variables;
};

TemporalVariableLookupPtr LocalScope::addArgument(Oop symbol, Node *localContext)
{
	auto variable = std::make_shared<TemporalVariableLookup> (localContext, false);
	if(variables.insert(std::make_pair(symbol, variable)).second)
        return variable;
    return TemporalVariableLookupPtr();
}

TemporalVariableLookupPtr LocalScope::addTemporal(Oop symbol, Node *localContext)
{
	auto variable = std::make_shared<TemporalVariableLookup> (localContext, true );
	if(variables.insert(std::make_pair(symbol, variable)).second)
        return variable;
    return TemporalVariableLookupPtr();
}

VariableLookupPtr LocalScope::lookSymbol(Oop symbol)
{
	auto it = variables.find(symbol);
	if(it != variables.end())
		return it->second;
	return VariableLookupPtr();
}

class AbstractASTVisitor: public ASTVisitor
{
public:
	void error(Node *location, const char *format, ...);
};

void AbstractASTVisitor::error(Node *location, const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);

	fputs(buffer, stderr);
	fputs("\n", stderr);
	abort();
}

// Scoped interpreter
class ScopedInterpreter: public AbstractASTVisitor
{
public:
	ScopedInterpreter(const EvaluationScopePtr &initialScope = EvaluationScopePtr())
		: currentScope(initialScope) {}
	~ScopedInterpreter() {}

	void pushScope(EvaluationScopePtr newScope)
	{
		currentScope = newScope;
	}

	void popScope()
	{
		currentScope = currentScope->getParent();
	}


protected:
	EvaluationScopePtr currentScope;
};

// ASTInterpreter visitor
class ASTInterpreter: public ScopedInterpreter
{
public:
	ASTInterpreter(const EvaluationScopePtr &initialScope, Oop currentSelf)
		: ScopedInterpreter(initialScope), currentSelf(currentSelf) {}

	virtual Oop visitArgument(Argument *node);
	virtual Oop visitArgumentList(ArgumentList *node);
	virtual Oop visitAssignmentExpression(AssignmentExpression *node);
	virtual Oop visitBlockExpression(BlockExpression *node);
	virtual Oop visitIdentifierExpression(IdentifierExpression *node);
	virtual Oop visitLiteralNode(LiteralNode *node);
	virtual Oop visitLocalDeclarations(LocalDeclarations *node);
	virtual Oop visitLocalDeclaration(LocalDeclaration *node);
	virtual Oop visitMessageSendNode(MessageSendNode *node);
	virtual Oop visitMethodAST(MethodAST *node);
	virtual Oop visitMethodHeader(MethodHeader *node);
	virtual Oop visitReturnStatement(ReturnStatement *node);
	virtual Oop visitSequenceNode(SequenceNode *node);
	virtual Oop visitSelfReference(SelfReference *node);
	virtual Oop visitSuperReference(SuperReference *node);
	virtual Oop visitThisContextReference(ThisContextReference *node);

private:
	OopRef currentSelf;
};

Oop ASTInterpreter::visitArgument(Argument *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitArgumentList(ArgumentList *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitAssignmentExpression(AssignmentExpression *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitBlockExpression(BlockExpression *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitIdentifierExpression(IdentifierExpression *node)
{
	VariableLookupPtr variable;

	// Find in the current scope.
	if(currentScope)
		variable = currentScope->lookSymbolRecursively(node->getSymbol());

	// Ensure it was found.
	if(!variable)
		error(node, "undeclared identifier '%s'.", node->getIdentifier().c_str());

	// Read the variable.
	return variable->getValue();
}

Oop ASTInterpreter::visitLiteralNode(LiteralNode *node)
{
	return node->getValue();
}

Oop ASTInterpreter::visitLocalDeclarations(LocalDeclarations *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitLocalDeclaration(LocalDeclaration *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitMessageSendNode(MessageSendNode *node)
{
	OopRef result;
	std::vector<OopRef> argumentValueRefs;
	std::vector<Oop> argumentValues;

	// Evaluate the receiver.
	OopRef receiver = node->getReceiver()->acceptVisitor(this);
	auto &chained = node->getChainedMessages();

	// Send each message in the chain
	for(int i = -1; i < (int)chained.size(); ++i)
	{
		auto message = i < 0 ? node : chained[i];
		auto selector = message->getSelectorOop();

		// Evaluate the arguments.
		auto &arguments = message->getArguments();
		argumentValues.clear();
		for(auto &arg : arguments)
		{
			argumentValueRefs.push_back(arg->acceptVisitor(this));
			argumentValues.push_back(argumentValueRefs.back().oop);
		}

		// Send the message.
		result = sendMessage(receiver.oop, selector, argumentValues.size(), &argumentValues[0]);
	}

	// Return the result.
	return result.oop;
}

Oop ASTInterpreter::visitMethodAST(MethodAST *node)
{
	return node->getHandle().getOop();
}

Oop ASTInterpreter::visitMethodHeader(MethodHeader *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitReturnStatement(ReturnStatement *node)
{
	assert(0 && "unimplemented");
	abort();
}

Oop ASTInterpreter::visitSequenceNode(SequenceNode *node)
{
	Oop result;
	if(node->getLocalDeclarations())
	{
		// TODO: Create the new scope
		assert(0 && "unimplemented");
		abort();
	}

	for(auto &child : node->getChildren())
		result = child->acceptVisitor(this);
	return result;
}

Oop ASTInterpreter::visitSelfReference(SelfReference *node)
{
	return currentSelf.oop;
}

Oop ASTInterpreter::visitSuperReference(SuperReference *node)
{
	return currentSelf.oop;
}

Oop ASTInterpreter::visitThisContextReference(ThisContextReference *node)
{
	error(node, "this context is not supported");
	abort();
}

// Method compiler semantic analysis
class MethodSemanticAnalysis: public ScopedInterpreter
{
public:
	MethodSemanticAnalysis(const EvaluationScopePtr &initialScope)
		: ScopedInterpreter(initialScope) {}

    virtual Oop visitArgument(Argument *node);
    virtual Oop visitArgumentList(ArgumentList *node);
    virtual Oop visitAssignmentExpression(AssignmentExpression *node);
    virtual Oop visitBlockExpression(BlockExpression *node);
    virtual Oop visitIdentifierExpression(IdentifierExpression *node);
    virtual Oop visitLiteralNode(LiteralNode *node);
    virtual Oop visitLocalDeclarations(LocalDeclarations *node);
    virtual Oop visitLocalDeclaration(LocalDeclaration *node);
    virtual Oop visitMessageSendNode(MessageSendNode *node);
    virtual Oop visitMethodAST(MethodAST *node);
    virtual Oop visitMethodHeader(MethodHeader *node);
    virtual Oop visitReturnStatement(ReturnStatement *node);
    virtual Oop visitSequenceNode(SequenceNode *node);
    virtual Oop visitSelfReference(SelfReference *node);
    virtual Oop visitSuperReference(SuperReference *node);
    virtual Oop visitThisContextReference(ThisContextReference *node);

private:
    bool optimizeMessage(MessageSendNode *node, CompilerOptimizedSelector selectorId);
    void inlineBlock(Node *node, int argumentCount);

    MethodAST::LocalVariables localVariables;
    Node *localContext;
};

void MethodSemanticAnalysis::inlineBlock(Node *node, int argumentCount)
{
    if(node->isBlockExpression())
    {
        auto block = reinterpret_cast<BlockExpression*> (node);
        if(argumentCount >= 0 && (int)block->getArgumentCount() != argumentCount)
            error(node, "this block has an invalid number of parameters.");

        if(block)
            block->setInlined(true);
    }

    node->acceptVisitor(this);
}

Oop MethodSemanticAnalysis::visitArgument(Argument *node)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodSemanticAnalysis::visitArgumentList(ArgumentList *node)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodSemanticAnalysis::visitAssignmentExpression(AssignmentExpression *node)
{
    // Visit the identifier.
    node->getReference()->acceptVisitor(this);

    // Visit the value.
    node->getValue()->acceptVisitor(this);

    // Ensure the reference is an identifier expression.
    auto reference = node->getReference();
    if(!reference->isIdentifierExpression())
    {
        LODTALK_UNIMPLEMENTED();
    }

    // Ensure the variable is mutable.
    auto identExpr = static_cast<AST::IdentifierExpression*> (reference);
    auto variable = identExpr-> getVariable();
    if(!variable->isMutable())
        error(node, "cannot perform an assignment into an immutable variable.");

    return Oop();
}

Oop MethodSemanticAnalysis::visitBlockExpression(BlockExpression *node)
{
    auto argumentList = node->getArgumentList();

    if(node->isInlined())
    {
        if(argumentList)
        {
            auto &arguments = argumentList->getArguments();
            auto argumentCount = arguments.size();

            // Create the arguments scope.
            auto argumentScope = std::make_shared<LocalScope> (currentScope);
            for(size_t i = 0; i < argumentCount; ++i)
            {
                auto &arg = arguments[i];
                auto res = argumentScope->addArgument(arg->getSymbolOop(), localContext);
                if(!res)
                    error(arg, "the argument has the same name as another argument.");
                node->addInlineArgument(res);
                localVariables.push_back(res);
            }

            pushScope(argumentScope);
        }

        // Visit the method body
        node->getBody()->acceptVisitor(this);

        if(argumentList)
            popScope();
        return Oop();
    }

    // Store the local context.
    auto oldLocalContext = localContext;
    localContext = node;
    FunctionalNode::LocalVariables oldLocalVariables;
    oldLocalVariables.swap(localVariables);

    // Process the arguments
	if(argumentList)
	{
		auto &arguments = argumentList->getArguments();
        auto argumentCount = arguments.size();

		// Create the arguments scope.
		auto argumentScope = std::make_shared<LocalScope> (currentScope);
		for(size_t i = 0; i < argumentCount; ++i)
		{
			auto &arg = arguments[i];
			auto res = argumentScope->addArgument(arg->getSymbolOop(), localContext);
			if(!res)
				error(arg, "the argument has the same name as another argument.");
            localVariables.push_back(res);
		}

		pushScope(argumentScope);
	}

    // Visit the method body
	node->getBody()->acceptVisitor(this);

	if(argumentList)
		popScope();

    // Store the block local variables.
    node->setLocalVariables(localVariables);

    // Restore the local context.
    localContext = oldLocalContext;
    oldLocalVariables.swap(localVariables);

    return Oop();
}

Oop MethodSemanticAnalysis::visitIdentifierExpression(IdentifierExpression *node)
{
    // Find the variable
    auto variable = currentScope->lookSymbolRecursively(node->getSymbol());
    if(!variable)
        error(node, "undeclared identifier '%s'.", node->getIdentifier().c_str());

    // Check the variable context, for marking the closure.
    if(variable->isTemporal())
    {
        auto temporal = std::static_pointer_cast<TemporalVariableLookup> (variable);
        if(temporal->getLocalContext() != localContext)
            temporal->setCapturedInClosure(true);
    }

    node->setVariable(variable);

    return Oop();
}

Oop MethodSemanticAnalysis::visitLiteralNode(LiteralNode *node)
{
    return Oop();
}

Oop MethodSemanticAnalysis::visitLocalDeclarations(LocalDeclarations *node)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodSemanticAnalysis::visitLocalDeclaration(LocalDeclaration *node)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodSemanticAnalysis::visitMessageSendNode(MessageSendNode *node)
{
	// Visit the arguments in reverse order.
	auto &chained = node->getChainedMessages();

    // Only try to optimize some message when they aren't in a chain.
    if(chained.empty())
    {
        auto selector = node->getSelectorOop();

        // If this is an optimized message, implement inlined.
        auto optimizedSelector = getCompilerOptimizedSelectorId(selector);
        if(optimizedSelector != CompilerOptimizedSelector::Invalid)
        {
            if(optimizeMessage(node, optimizedSelector))
                return Oop();
        }
    }

    // Visit the receiver.
	node->getReceiver()->acceptVisitor(this);

	// Send each message in the chain
	for(int i = -1; i < (int)chained.size(); ++i)
	{
		auto message = i < 0 ? node : chained[i];

		// Visit the arguments.
		auto &arguments = message->getArguments();
		for(auto &arg : arguments)
			arg->acceptVisitor(this);
	}

	return Oop();
}

bool MethodSemanticAnalysis::optimizeMessage(MessageSendNode *node, CompilerOptimizedSelector selectorId)
{
    auto receiver = node->getReceiver();
    auto arguments = node->getArguments();

    switch(selectorId)
    {
    case CompilerOptimizedSelector::IfTrue:
    case CompilerOptimizedSelector::IfFalse:
    case CompilerOptimizedSelector::IfNil:
    case CompilerOptimizedSelector::IfNotNil:
        {
            auto thenBlock = arguments[0];
            receiver->acceptVisitor(this);
            inlineBlock(thenBlock, 0);
        }
        break;
    case CompilerOptimizedSelector::IfTrueIfFalse:
    case CompilerOptimizedSelector::IfFalseIfTrue:
    case CompilerOptimizedSelector::IfNotNilIfNil:
    case CompilerOptimizedSelector::IfNilIfNotNil:

        {
            auto thenBlock = arguments[0];
            auto elseBlock = arguments[1];
            receiver->acceptVisitor(this);
            inlineBlock(thenBlock, 0);
            inlineBlock(elseBlock, 0);
        }
        break;
    case CompilerOptimizedSelector::WhileTrue:
    case CompilerOptimizedSelector::WhileFalse:
        {
            if(!receiver->isBlockExpression())
                return false;
            auto conditionBlock = receiver;
            auto bodyBlock = arguments[0];
            inlineBlock(conditionBlock, 0);
            inlineBlock(bodyBlock, 0);
        }
        break;
    case CompilerOptimizedSelector::ToDo:
        {
            auto startValue = receiver;
            auto stopValue = arguments[0];
            auto bodyBlock = arguments[1];
            if(!bodyBlock->isBlockExpression())
                return false;

            startValue->acceptVisitor(this);
            stopValue->acceptVisitor(this);
            inlineBlock(bodyBlock, 1);
        }
        break;
    case CompilerOptimizedSelector::ToByDo:
        {
            auto startValue = receiver;
            auto stopValue = arguments[0];
            auto stepValue = arguments[1];
            auto bodyBlock = arguments[2];
            if(!bodyBlock->isBlockExpression())
                return false;

            startValue->acceptVisitor(this);
            stopValue->acceptVisitor(this);
            stepValue->acceptVisitor(this);
            inlineBlock(bodyBlock, 1);
        }
        break;
    default:
        LODTALK_UNIMPLEMENTED();
    }

    return true;
}

Oop MethodSemanticAnalysis::visitMethodAST(MethodAST *node)
{
    // Set the local context.
    localContext = node;

    // Get the method header.
	auto header = node->getHeader();

	// Process the arguments
	auto argumentList = header->getArgumentList();
	if(argumentList)
	{
		auto &arguments = argumentList->getArguments();
        auto argumentCount = arguments.size();

		// Create the arguments scope.
		auto argumentScope = std::make_shared<LocalScope> (currentScope);
		for(size_t i = 0; i < argumentCount; ++i)
		{
			auto &arg = arguments[i];
			auto res = argumentScope->addArgument(arg->getSymbolOop(), localContext);
			if(!res)
				error(arg, "the argument has the same name as another argument.");
            localVariables.push_back(res);
		}

		pushScope(argumentScope);
	}

    // Visit the method body
	node->getBody()->acceptVisitor(this);

	if(argumentList)
		popScope();

    // Store the local variables in the ast.
    node->setLocalVariables(localVariables);

    return Oop();
}

Oop MethodSemanticAnalysis::visitMethodHeader(MethodHeader *node)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodSemanticAnalysis::visitReturnStatement(ReturnStatement *node)
{
    // Visit the value.
	node->getValue()->acceptVisitor(this);
    return Oop();
}

Oop MethodSemanticAnalysis::visitSequenceNode(SequenceNode *node)
{
    auto decls = node->getLocalDeclarations();
    if(decls)
	{
        auto newScope = std::make_shared<LocalScope> (currentScope);
        for(auto &localDecl : decls->getLocals())
        {
            auto res = newScope->addArgument(localDecl->getSymbolOop(), localContext);
			if(!res)
				error(localDecl, "the argument has the same name as another argument.");
            localVariables.push_back(res);
        }

        pushScope(newScope);
	}

	for(auto &child : node->getChildren())
		child->acceptVisitor(this);

    if(decls)
        popScope();

    return Oop();
}

Oop MethodSemanticAnalysis::visitSelfReference(SelfReference *node)
{
    return Oop();
}

Oop MethodSemanticAnalysis::visitSuperReference(SuperReference *node)
{
    return Oop();
}

Oop MethodSemanticAnalysis::visitThisContextReference(ThisContextReference *node)
{
    return Oop();
}

// Method compiler
class MethodCompiler: public AbstractASTVisitor
{
public:
	MethodCompiler(Oop classBinding)
		: classBinding(classBinding) {}

	virtual Oop visitArgument(Argument *node);
	virtual Oop visitArgumentList(ArgumentList *node);
	virtual Oop visitAssignmentExpression(AssignmentExpression *node);
	virtual Oop visitBlockExpression(BlockExpression *node);
	virtual Oop visitIdentifierExpression(IdentifierExpression *node);
	virtual Oop visitLiteralNode(LiteralNode *node);
	virtual Oop visitLocalDeclarations(LocalDeclarations *node);
	virtual Oop visitLocalDeclaration(LocalDeclaration *node);
	virtual Oop visitMessageSendNode(MessageSendNode *node);
	virtual Oop visitMethodAST(MethodAST *node);
	virtual Oop visitMethodHeader(MethodHeader *node);
	virtual Oop visitReturnStatement(ReturnStatement *node);
	virtual Oop visitSequenceNode(SequenceNode *node);
	virtual Oop visitSelfReference(SelfReference *node);
	virtual Oop visitSuperReference(SuperReference *node);
	virtual Oop visitThisContextReference(ThisContextReference *node);

private:
    bool generateOptimizedMessage(MessageSendNode *node, CompilerOptimizedSelector optimizedSelector);
    void generateIf(MessageSendNode *node, Oop trueValue, Node *receiver, Node *trueBranch, bool negated = false);
    void generateIfElse(MessageSendNode *node, Oop trueValue, Node *receiver, Node *trueBranch, Node *falseBranch, bool negated = false);
    void generateWhile(MessageSendNode *node, Oop trueValue, Node *receiver, Node *bodyNode);
    void generateToDo(MessageSendNode *node, Node *receiver, Node *stopNode, Node *bodyNode);
    void generateToByDo(MessageSendNode *node, Node *receiver, Node *stopNode, Node *stepNode, Node *bodyNode);

	OopRef selector;
	OopRef classBinding;
	MethodAssembler::Assembler gen;
    FunctionalNode *localContext;
    int temporalVectorCount;
};

// Method compiler.
Oop MethodCompiler::visitArgument(Argument *node)
{
	LODTALK_UNIMPLEMENTED();
}

Oop MethodCompiler::visitArgumentList(ArgumentList *node)
{
	LODTALK_UNIMPLEMENTED();
}

Oop MethodCompiler::visitAssignmentExpression(AssignmentExpression *node)
{
    // Generate the value
    auto value = node->getValue();
    value->acceptVisitor(this);

    // Ensure the reference is an identifier expression.
    auto reference = node->getReference();
    if(!reference->isIdentifierExpression())
        LODTALK_UNIMPLEMENTED();

    // Perform the assignment.
    auto identExpr = static_cast<AST::IdentifierExpression*> (reference);
    auto variable = identExpr->getVariable();
    variable->generateStore(gen, localContext);

    return Oop();
}

Oop MethodCompiler::visitBlockExpression(BlockExpression *node)
{
    if(node->isInlined())
    {
        // Visit the method body
        return node->getBody()->acceptVisitor(this);
    }

    // Store the local context
    auto oldLocalContext = localContext;
    localContext = node;

    auto blockEnd = gen.makeLabel();

    // Process the arguments
    auto argumentList = node->getArgumentList();
    size_t argumentCount = 0;
    if(argumentList)
        argumentCount = argumentList->getArguments().size();

    // Count the captured variables.
    auto &blockLocals = node->getLocalVariables();
    auto capturedCount = 0;
    for(size_t i = 0; i < blockLocals.size(); ++i)
    {
        auto &localVar = blockLocals[i];
        if(localVar->isCapturedInClosure())
        {
            localVar->setTemporalVectorIndex(temporalVectorCount);
            localVar->setTemporalIndex(capturedCount++);
        }
    }

    // Push the captured temporal vectors.
    size_t numCopied = temporalVectorCount;
    size_t numLocals = 0;
    if(temporalVectorCount)
    {
        auto oldArgumentCount = oldLocalContext->getArgumentCount();
        for(auto i = 0; i < temporalVectorCount; ++i)
            gen.pushTemporal(oldArgumentCount + i);

    }

    if(capturedCount)
    {
        ++temporalVectorCount;

        // Reserve space for the inner temporal vector.
        ++numLocals;
        ++numCopied;
    }

    // Prepare the local variables of the block.
    for(size_t i = 0; i < blockLocals.size(); ++i)
    {
        auto &localVar = blockLocals[i];
        if(localVar->isCapturedInClosure())
            continue;

        if(i < argumentCount)
        {
            localVar->setTemporalIndex(i);
        }
        else
        {
            localVar->setTemporalIndex(argumentCount + numCopied);
            ++numLocals;
            ++numCopied;
        }
    }

    // Reserve space for the locals.
    if(numLocals)
        gen.pushNClosureTemps(numLocals);

    // Push the block.
    gen.pushClosure(numCopied, argumentCount, blockEnd, 0);

    // Generate the inner temporal vector.
    if(capturedCount)
    {
        gen.pushNewArray(capturedCount);
        gen.popStoreTemporal(argumentCount + temporalVectorCount - 1);

        // Copy the captured arguments into the temp vector
        for(size_t i = 0; i < argumentCount; ++i)
        {
            auto &localVar = blockLocals[i];
            if(localVar->isCapturedInClosure())
            {
                gen.pushTemporal(i);
                gen.popStoreTemporalInVector(localVar->getTemporalIndex(), localVar->getTemporalVectorIndex());
            }
        }
    }

    auto closureBeginInstruction = gen.getLastInstruction();

    // Generate the block body.
    node->getBody()->acceptVisitor(this);

    // Always return
	if(!gen.isLastReturn())
    {
        if(gen.getLastInstruction() == closureBeginInstruction)
            gen.blockReturnNil();
        else
            gen.blockReturnTop();
    }

    // Finish the block.
    gen.putLabel(blockEnd);

    // Restore the local context
    localContext = oldLocalContext;

    if(capturedCount)
        --temporalVectorCount;

	return Oop();
}

Oop MethodCompiler::visitIdentifierExpression(IdentifierExpression *node)
{
	auto &variable = node->getVariable();

	// Ensure it was found.
	if(!variable)
		error(node, "undeclared identifier '%s'.", node->getIdentifier().c_str());

	// Generate the load.
	variable->generateLoad(gen, localContext);

	return Oop();
}

Oop MethodCompiler::visitLiteralNode(LiteralNode *node)
{
	gen.pushLiteral(node->getValue());
	return Oop();
}

Oop MethodCompiler::visitLocalDeclarations(LocalDeclarations *node)
{
	LODTALK_UNIMPLEMENTED();
}

Oop MethodCompiler::visitLocalDeclaration(LocalDeclaration *node)
{
	LODTALK_UNIMPLEMENTED();
}

bool MethodCompiler::generateOptimizedMessage(MessageSendNode *node, CompilerOptimizedSelector optimizedSelector)
{
    auto arguments = node->getArguments();
    auto receiver = node->getReceiver();

    switch(optimizedSelector)
    {
    case CompilerOptimizedSelector::IfTrue:
        generateIf(node, trueOop(), receiver, arguments[0]);
        break;
    case CompilerOptimizedSelector::IfFalse:
        generateIf(node, falseOop(), receiver, arguments[0]);
        break;
    case CompilerOptimizedSelector::IfNil:
        generateIf(node, nilOop(), receiver, arguments[0]);
        break;
    case CompilerOptimizedSelector::IfNotNil:
        generateIf(node, nilOop(), receiver, arguments[0], true);
        break;
    case CompilerOptimizedSelector::IfTrueIfFalse:
        generateIfElse(node, trueOop(), receiver, arguments[0], arguments[1]);
        break;
    case CompilerOptimizedSelector::IfFalseIfTrue:
        generateIfElse(node, falseOop(), receiver, arguments[0], arguments[1]);
        break;
    case CompilerOptimizedSelector::IfNilIfNotNil:
        generateIfElse(node, nilOop(), receiver, arguments[0], arguments[1]);
        break;
    case CompilerOptimizedSelector::IfNotNilIfNil:
        generateIfElse(node, nilOop(), receiver, arguments[0], arguments[1], true);
        break;
    case CompilerOptimizedSelector::WhileTrue:
        if(!receiver->isBlockExpression())
            return false;
        generateWhile(node, trueOop(), receiver, arguments[0]);
        break;
    case CompilerOptimizedSelector::WhileFalse:
        if(!receiver->isBlockExpression())
            return false;
        generateWhile(node, falseOop(), receiver, arguments[0]);
        break;
    case CompilerOptimizedSelector::ToDo:
        if(!arguments[1]->isBlockExpression())
            return false;
        generateToDo(node, receiver, arguments[0], arguments[1]);
        break;
    case CompilerOptimizedSelector::ToByDo:
        if(!arguments[2]->isBlockExpression())
            return false;
        generateToByDo(node, receiver, arguments[0], arguments[1], arguments[2]);
        break;
    default:
        LODTALK_UNIMPLEMENTED();
    }

    return true;
}

void MethodCompiler::generateIf(MessageSendNode *node, Oop trueValue, Node *receiver, Node *trueBranch, bool negated)
{
    generateIfElse(node, trueValue, receiver, trueBranch, nullptr, negated);
}

void MethodCompiler::generateIfElse(MessageSendNode *node, Oop trueValue, Node *receiver, Node *trueBranch, Node *falseBranch, bool negated)
{
    auto elseLabel = gen.makeLabel();
    auto mergeLabel = gen.makeLabel();

    // Evaluate the condition.
    receiver->acceptVisitor(this);

    // Compare the condition to the true value.
    if(trueValue == trueOop())
    {
        gen.jumpOnFalse(elseLabel);
    }
    else if(trueValue == falseOop())
    {
        gen.jumpOnTrue(elseLabel);
    }
    else
    {
        gen.pushLiteral(trueValue);
        gen.identityEqual();
        if(negated)
            gen.jumpOnTrue(elseLabel);
        else
            gen.jumpOnFalse(elseLabel);
    }

    // Generate the true branch.
    trueBranch->acceptVisitor(this);
    if(!trueBranch->isBlockExpression())
        gen.sendValue();
    gen.jump(mergeLabel);

    // Generate the false branch.
    gen.putLabel(elseLabel);
    if(falseBranch)
    {
        falseBranch->acceptVisitor(this);
        if(!falseBranch->isBlockExpression())
            gen.sendValue();
    }
    else
        gen.pushNil();

    // Merge the control flow.
    gen.putLabel(mergeLabel);
}

void MethodCompiler::generateWhile(MessageSendNode *node, Oop trueValue, Node *receiver, Node *bodyNode)
{
    // Enter into the loop.
    auto *entry = gen.makeLabelHere();
    auto *exit = gen.makeLabel();

    // Evaluate the condition.
    receiver->acceptVisitor(this);

    // Compare the condition to the true value.
    if(trueValue == trueOop())
    {
        gen.jumpOnFalse(exit);
    }
    else if(trueValue == falseOop())
    {
        gen.jumpOnTrue(exit);
    }
    else
    {
        gen.pushLiteral(trueValue);
        gen.identityEqual();
        gen.jumpOnTrue(exit);
    }

    // Enter into the loop body.
    bodyNode->acceptVisitor(this);

    // Pop the last value.
    gen.popStackTop();

    // Go back.
    gen.jump(entry);

    // Continue after the loop.
    gen.putLabel(exit);
    gen.pushNil();
}

void MethodCompiler::generateToDo(MessageSendNode *node, Node *receiver, Node *stopNode, Node *bodyNode)
{
    // Get the data from the body.
    assert(bodyNode->isBlockExpression());
    auto bodyBlock = reinterpret_cast<BlockExpression*> (bodyNode);
    auto &bodyArguments = bodyBlock->getInlineArguments();
    assert(bodyBlock->getArgumentCount() == 1);
    assert(bodyArguments.size() == 1);
    auto &iterationVariable = bodyArguments[0];

    // Generate the starting value.
    receiver->acceptVisitor(this);
    iterationVariable->generateStore(gen, localContext);

    // Generate the end value.
    stopNode->acceptVisitor(this);

    // The loop condition.
    auto loopCondition = gen.makeLabelHere();
    auto loopEnd = gen.makeLabel();

    // Check the loop condition.
    gen.duplicateStackTop();
    iterationVariable->generateLoad(gen, localContext);
    gen.greaterEqual();
    gen.jumpOnFalse(loopEnd);

    // The loop body.
    bodyNode->acceptVisitor(this);
    if(!bodyNode->isBlockExpression())
        gen.sendValueWithArg();
    gen.popStackTop();

    // Increase the value by one.
    iterationVariable->generateLoad(gen, localContext);
    gen.pushOne();
    gen.add();
    iterationVariable->generateStore(gen, localContext);
    gen.popStackTop();
    gen.jump(loopCondition);

    // End of the loop.
    gen.putLabel(loopEnd);
    gen.popStackTop(); // The stop value
}

void MethodCompiler::generateToByDo(MessageSendNode *node, Node *receiver, Node *stopNode, Node *stepNode, Node *bodyNode)
{
    LODTALK_UNIMPLEMENTED();
}

Oop MethodCompiler::visitMessageSendNode(MessageSendNode *node)
{
	// Visit the receiver.
	bool isSuper = node->getReceiver()->isSuperReference();

	// Visit the arguments in reverse order.
	auto &chained = node->getChainedMessages();

    // Only try to optimize some message when they aren't in a chain.
    if(chained.empty())
    {
        auto selector = node->getSelectorOop();

        // If this is an optimized message, try to optimize
        auto optimizedSelector = getCompilerOptimizedSelectorId(selector);
        if(optimizedSelector != CompilerOptimizedSelector::Invalid)
        {
            if(generateOptimizedMessage(node, optimizedSelector))
                return Oop();
        }
    }

    node->getReceiver()->acceptVisitor(this);

	// Send each message in the chain
	bool first = true;
    int lastIndex = (int)chained.size() - 1;
	for(int i = -1; i < (int)chained.size(); ++i)
	{
		auto message = i < 0 ? node : chained[i];
		auto selector = message->getSelectorOop();

		if(first)
			first = false;
		else
			gen.popStackTop();
        if(i != lastIndex)
            gen.duplicateStackTop();

		// Evaluate the arguments.
		auto &arguments = message->getArguments();
		for(auto &arg : arguments)
			arg->acceptVisitor(this);

		// Send the message.
        if(isSuper)
            gen.superSend(selector, arguments.size());
        else
		    gen.send(selector, arguments.size());
	}

	return Oop();
}

Oop MethodCompiler::visitMethodAST(MethodAST *node)
{
    size_t temporalCount = 0;
    size_t argumentCount = 0;

    // Set the local context
    localContext = node;
    temporalVectorCount = 0;

	// Get the method selector.
	auto header = node->getHeader();
	selector = makeSelector(header->getSelector());

	// Process the arguments
	auto argumentList = header->getArgumentList();
	if(argumentList)
        argumentCount = argumentList->getArguments().size();

    // Count the captured variables.
    auto &blockLocals = node->getLocalVariables();
    size_t capturedCount = 0;
    for(size_t i = 0; i < blockLocals.size(); ++i)
    {
        auto &localVar = blockLocals[i];
        if(localVar->isCapturedInClosure())
        {
            localVar->setTemporalVectorIndex(temporalVectorCount);
            localVar->setTemporalIndex(capturedCount++);
        }
    }

    if(capturedCount)
    {
        ++temporalVectorCount;
        ++temporalCount;
    }

    // Compute the local variables indices.
    auto &localVars = node->getLocalVariables();
    for(size_t i = 0; i < localVars.size(); ++i)
    {
        auto &localVar = localVars[i];
        if(!localVar->isCapturedInClosure())
        {
            if(i < argumentCount)
                localVar->setTemporalIndex(i);
            else
            {
                localVar->setTemporalIndex(i + temporalVectorCount);
                ++temporalCount;
            }
        }
    }

    // Create the temporal vector
    if(capturedCount)
    {
        gen.pushNewArray(capturedCount);
        gen.popStoreTemporal(argumentCount);

        // Copy the captured arguments into the temp vector
        for(size_t i = 0; i < argumentCount; ++i)
        {
            auto &localVar = blockLocals[i];
            if(localVar->isCapturedInClosure())
            {
                gen.pushTemporal(i);
                gen.popStoreTemporalInVector(localVar->getTemporalIndex(), argumentCount + localVar->getTemporalVectorIndex());
            }
        }
    }

	// Visit the method body
	node->getBody()->acceptVisitor(this);

	// Always return
	if(!gen.isLastReturn())
		gen.returnReceiver();

	// Set the method selector
	gen.addLiteralAlways(selector);

	// Set the class binding.
	gen.addLiteralAlways(classBinding);
	return Oop::fromPointer(gen.generate(temporalCount, argumentCount));
}

Oop MethodCompiler::visitMethodHeader(MethodHeader *node)
{
	LODTALK_UNIMPLEMENTED();
}

Oop MethodCompiler::visitReturnStatement(ReturnStatement *node)
{
	// Visit the value.
	node->getValue()->acceptVisitor(this);

	// Return it
	gen.returnTop();
	return Oop();
}

Oop MethodCompiler::visitSequenceNode(SequenceNode *node)
{
	bool first = true;
	for(auto &child : node->getChildren())
	{
		if(first)
			first = false;
		else
			gen.popStackTop();

		child->acceptVisitor(this);
	}

	return Oop();
}

Oop MethodCompiler::visitSelfReference(SelfReference *node)
{
	gen.pushReceiver();
	return Oop();
}

Oop MethodCompiler::visitSuperReference(SuperReference *node)
{
    gen.pushReceiver();
	return Oop();
}

Oop MethodCompiler::visitThisContextReference(ThisContextReference *node)
{
	gen.pushThisContext();
	return Oop();
}

// Compiler interface
CompiledMethod *compileMethod(const EvaluationScopePtr &scope, ClassDescription *clazz, Node *ast)
{
    // Perform the semantic analysis
    MethodSemanticAnalysis semanticAnalyzer(scope);
    ast->acceptVisitor(&semanticAnalyzer);

    // Generate the actual method
	MethodCompiler compiler(clazz->getBinding());
    return reinterpret_cast<CompiledMethod*> (ast->acceptVisitor(&compiler).pointer);
}

Oop executeDoIt(const std::string &code)
{
	// TODO: implement this
	abort();
}

Oop executeScript(const std::string &code, const std::string &name, const std::string &basePath)
{
	// TODO: implement this
	abort();
}

Oop executeScriptFromFile(FILE *file, const std::string &name, const std::string &basePath)
{
	// Create the script context
	Ref<ScriptContext> context(reinterpret_cast<ScriptContext*> (ScriptContext::ClassObject->basicNativeNew()));
	if(context.isNil())
		return Oop();
	context->globalContextClass = Oop::fromPointer(GlobalContext::MetaclassObject);
	context->basePath = makeByteString(basePath);

	// Parse the script.
	auto ast = Lodtalk::AST::parseSourceFromFile(file);
	if(!ast)
		return Oop();

	// Create the global scope
	auto scope = std::make_shared<GlobalEvaluationScope> ();

	// Interpret the script.
	ASTInterpreter interpreter(scope, context.getOop());
	auto result = ast->acceptVisitor(&interpreter);
	return result;
}

Oop executeScriptFromFileNamed(const std::string &filename)
{
	StdFile file(filename, "r");
	if(!file)
		nativeErrorFormat("Failed to open file '%s'", filename.c_str());

	std::string basePathString = dirname(filename);

	return executeScriptFromFile(file, filename, basePathString);
}

// ScriptContext
Oop ScriptContext::setCurrentCategory(Oop category)
{
	currentCategory = category;
	return selfOop();
}

Oop ScriptContext::setCurrentClass(Oop classObject)
{
	currentClass = classObject;
	return selfOop();
}

Oop ScriptContext::executeFileNamed(Oop fileNameOop)
{
	std::string basePathString;
	if(fileNameOop.isNil())
		nativeError("expected a file name.");

	if(!basePath.isNil())
		basePathString = getByteStringData(basePath);
	std::string fileNameString = getByteStringData(fileNameOop);
	std::string fullFileName = joinPath(basePathString, fileNameString);
	return executeScriptFromFileNamed(fullFileName);
}

Oop ScriptContext::addFunction(Oop methodAstHandle)
{
	if(isNil(methodAstHandle))
		nativeError("cannot add method with nil ast.");
	if(classIndexOf(methodAstHandle) != SCI_MethodASTHandle)
		nativeError("expected a method AST handle.");

	// Check the class
	if(!isClassOrMetaclass(globalContextClass))
		nativeError("a global context class is needed");
	auto clazz = reinterpret_cast<ClassDescription*> (globalContextClass.pointer);

	// Get the ast
	MethodASTHandle *handle = reinterpret_cast<MethodASTHandle*> (methodAstHandle.pointer);
	auto ast = handle->ast;

	// Create the global scope
	auto globalScope = std::make_shared<GlobalEvaluationScope> ();

	// TODO: Create the class global variables scope.

	// Create the class instance variables scope.
	auto instanceVarScope = std::make_shared<InstanceVariableScope> (globalScope, clazz);

	// Compile the method
	Ref<CompiledMethod> compiledMethod = compileMethod(instanceVarScope, clazz, ast);

	// Register the method in the global context class side
	auto selector = compiledMethod->getSelector();
	clazz->methodDict->atPut(selector, compiledMethod.getOop());

	// Return self
	return selfOop();
}

Oop ScriptContext::addMethod(Oop methodAstHandle)
{
	if(isNil(methodAstHandle))
		nativeError("cannot add method with nil ast.");
	if(classIndexOf(methodAstHandle) != SCI_MethodASTHandle)
		nativeError("expected a method AST handle.");

	// Check the class
	if(!isClassOrMetaclass(globalContextClass))
		nativeError("a class is needed for adding a method.");
	auto clazz = reinterpret_cast<ClassDescription*> (currentClass.pointer);

	// Get the ast
	MethodASTHandle *handle = reinterpret_cast<MethodASTHandle*> (methodAstHandle.pointer);
	auto ast = handle->ast;

	// Create the global scope
	auto globalScope = std::make_shared<GlobalEvaluationScope> ();

	// TODO: Create the class variables scope.

	// Create the class instance variables scope.
	auto instanceVarScope = std::make_shared<InstanceVariableScope> (globalScope, clazz);

	// Compile the method
	Ref<CompiledMethod> compiledMethod = compileMethod(instanceVarScope, clazz, ast);

	// Register the method in the current class
	auto selector = compiledMethod->getSelector();
	clazz->methodDict->atPut(selector, compiledMethod.getOop());

	// Return self
	return selfOop();
}

// The script context
LODTALK_BEGIN_CLASS_SIDE_TABLE(ScriptContext)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ScriptContext)
	LODTALK_METHOD("category:", &ScriptContext::setCurrentCategory)
	LODTALK_METHOD("class:", &ScriptContext::setCurrentClass)
	LODTALK_METHOD("function:", &ScriptContext::addFunction)
	LODTALK_METHOD("method:", &ScriptContext::addMethod)
	LODTALK_METHOD("executeFileNamed:", &ScriptContext::executeFileNamed)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(ScriptContext, Object, OF_FIXED_SIZE, 4,
"currentCategory currentClass globalContextClass basePath");

// The method ast handle
LODTALK_BEGIN_CLASS_SIDE_TABLE(MethodASTHandle)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(MethodASTHandle)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(MethodASTHandle, Object, OF_INDEXABLE_8, 0);

} // End of namespace Lodtalk
