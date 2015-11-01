#ifndef METHOD_BUILDER_HPP
#define METHOD_BUILDER_HPP

#include "Object.hpp"
#include "Collections.hpp"
#include "Method.hpp"

namespace Lodtalk
{
namespace MethodAssembler
{

/**
 * Method assembler node.
 */
class InstructionNode
{
public:
	InstructionNode()
		: position(0) {}
	virtual ~InstructionNode() {}

	virtual bool isReturnInstruction() const
	{
		return false;
	}

	size_t getPosition()
	{
		return position;
	}

    size_t getLastPosition()
	{
		return lastPosition;
	}

    size_t getLastSize()
    {
        return lastSize;
    }

    size_t getSize()
    {
        return size;
    }

    void commitSize()
    {
        lastSize = size;
        lastPosition = position;
    }

	size_t computeMaxSizeForPosition(size_t newPosition)
	{
		position = newPosition;
		return size = computeMaxSize();
	}

	size_t computeBetterSizeForPosition(size_t newPosition)
	{
		position = newPosition;
		return size = computeBetterSize();
	}

    uint8_t *encodeExtA(uint8_t *buffer, uint64_t value);
    uint8_t *encodeExtB(uint8_t *buffer, int64_t value);

    size_t sizeofExtA(uint64_t value);
    size_t sizeofExtB(int64_t value);

	virtual uint8_t *encode(uint8_t *buffer) = 0;

protected:
	virtual size_t computeMaxSize() = 0;
	virtual size_t computeBetterSize()
	{
		return computeMaxSize();
	}

private:
	size_t position;
	size_t size;
    size_t lastPosition;
    size_t lastSize;
};

class Label: public InstructionNode
{
public:
	virtual uint8_t *encode(uint8_t *buffer)
	{
		return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 0;
	}
};

/**
 * Compiled method builder
 */
class Assembler
{
public:
	Assembler();
	~Assembler();

	InstructionNode *addInstruction(InstructionNode *instruction);
	size_t addLiteral(Oop newLiteral);
	size_t addLiteral(const OopRef &newLiteral);

    size_t addLiteralAlways(Oop newLiteral);
	size_t addLiteralAlways(const OopRef &newLiteral);

	Label *makeLabel();
	Label *makeLabelHere();
	void putLabel(Label *label);

    InstructionNode *getLastInstruction();
	bool isLastReturn();

	CompiledMethod *generate(size_t temporalCount, size_t argumentCount, size_t extraSize = 0);

public:
	InstructionNode *returnReceiver();
	InstructionNode *returnTrue();
	InstructionNode *returnFalse();
	InstructionNode *returnNil();
	InstructionNode *returnTop();
    InstructionNode *blockReturnNil();
    InstructionNode *blockReturnTop();

	InstructionNode *popStackTop();
	InstructionNode *duplicateStackTop();

	InstructionNode *pushLiteral(Oop literal);
	InstructionNode *pushLiteralVariable(Oop literalVariable);

	InstructionNode *pushReceiverVariableIndex(int variableIndex);
	InstructionNode *pushLiteralIndex(int literalIndex);
	InstructionNode *pushLiteralVariableIndex(int literalVariableIndex);
	InstructionNode *pushTemporal(int temporalIndex);

    InstructionNode *storeReceiverVariableIndex(int variableIndex);
	InstructionNode *storeLiteralVariableIndex(int literalVariableIndex);
	InstructionNode *storeTemporal(int temporalIndex);
    InstructionNode *storeLiteralVariable(Oop literalVariable);
    InstructionNode *popStoreTemporal(int temporalIndex);

    InstructionNode *pushNewArray(int arraySize);
    InstructionNode *pushTemporalInVector(int temporalIndex, int vectorIndex);
    InstructionNode *pushNClosureTemps(int temporalCount);
    InstructionNode *storeTemporalInVector(int temporalIndex, int vectorIndex);
    InstructionNode *popStoreTemporalInVector(int temporalIndex, int vectorIndex);

    InstructionNode *pushClosure(int numCopied, int numArgs, Label *blockEnd, int numExtensions);

	InstructionNode *pushReceiver();
	InstructionNode *pushThisContext();
	InstructionNode *pushNil();
	InstructionNode *pushTrue();
	InstructionNode *pushFalse();
	InstructionNode *pushOne();
	InstructionNode *pushZero();

    InstructionNode *sendValue();
    InstructionNode *sendValueWithArg();
	InstructionNode *send(Oop selector, int argumentCount);
	InstructionNode *superSend(Oop selector, int argumentCount);

    InstructionNode *add();
    InstructionNode *greaterEqual();
    InstructionNode *greaterThan();
    InstructionNode *lessEqual();
    InstructionNode *lessThan();
    InstructionNode *equal();
    InstructionNode *notEqual();
    InstructionNode *identityEqual();

    InstructionNode *jump(Label *destination);
    InstructionNode *jumpOnTrue(Label *destination);
    InstructionNode *jumpOnFalse(Label *destination);

private:
	size_t computeInstructionsSize();

	std::vector<OopRef> literals;
	std::vector<InstructionNode*> instructionStream;
};

} // End of namespace Method assembler
} // End of namespace Lodtalk

#endif //METHOD_BUILDER_HPP
