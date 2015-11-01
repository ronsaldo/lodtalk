#include "MethodBuilder.hpp"
#include "BytecodeSets.hpp"

namespace Lodtalk
{
namespace MethodAssembler
{

const size_t ExtensibleBytecodeSizeMax = 16;

// InstructionNode
uint8_t *InstructionNode::encodeExtA(uint8_t *buffer, uint64_t value)
{
    if(!value)
        return buffer;
    LODTALK_UNIMPLEMENTED();
}

uint8_t *InstructionNode::encodeExtB(uint8_t *buffer, int64_t value)
{
    if(!value)
        return buffer;
    LODTALK_UNIMPLEMENTED();
}

size_t InstructionNode::sizeofExtA(uint64_t value)
{
    size_t res = 0;
    while(value)
    {
        value /= 256;
        res += 2;
    }
    return res;
}

size_t InstructionNode::sizeofExtB(int64_t value)
{
    size_t res = 0;
    while(value)
    {
        value /= 256;
        res += 2;
    }
    return res;
}

// Single bytecode instruction
class SingleBytecodeInstruction: public InstructionNode
{
public:
	SingleBytecodeInstruction(int bc, bool isReturn = false)
		: bytecode(bc), isReturn(isReturn) {}

	virtual bool isReturnInstruction() const
	{
		return isReturn;
	}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		*buffer++ = bytecode;
		return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 1;
	}

private:
	int bytecode;
	bool isReturn;
};

// PushReceiverVariable
class PushReceiverVariable: public InstructionNode
{
public:
	PushReceiverVariable(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		if(index < BytecodeSet::PushReceiverVariableShortRangeSize)
		{
			*buffer++ = BytecodeSet::PushReceiverVariableShortFirst + index;
			return buffer;
		}

        buffer = encodeExtA(buffer, index / 256);

		*buffer++ = BytecodeSet::PushReceiverVariable;
		*buffer++ = index % 256;
		return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		if(index < BytecodeSet::PushReceiverVariableShortRangeSize)
			return 1;
        return 2 + sizeofExtA(index / 256);
	}

private:
	int index;
};

// StoreReceiverVariable
class StoreReceiverVariable: public InstructionNode
{
public:
	StoreReceiverVariable(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        buffer = encodeExtA(buffer, index / 256);
        *buffer++ = BytecodeSet::StoreReceiverVariable;
        *buffer++ = index % 256;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
        return 2 + sizeofExtA(index / 256);
	}

private:
	int index;
};

// PushLiteral
class PushLiteral: public InstructionNode
{
public:
	PushLiteral(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		if(index < BytecodeSet::PushLiteralShortRangeSize)
		{
			*buffer++ = BytecodeSet::PushLiteralShortFirst + index;
			return buffer;
		}

        buffer = encodeExtA(buffer, index / 256);
        *buffer++ = BytecodeSet::PushLiteral;
        *buffer++ = index % 256;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		if(index < BytecodeSet::PushLiteralShortRangeSize)
			return 1;
		return 2 + sizeofExtA(index / 256);
	}

private:
	int index;
};

// PushLiteralVariable
class PushLiteralVariable: public InstructionNode
{
public:
	PushLiteralVariable(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		if(index < BytecodeSet::PushLiteralVariableShortRangeSize)
		{
			*buffer++ = BytecodeSet::PushLiteralVariableShortFirst + index;
			return buffer;
		}

        buffer = encodeExtA(buffer, index / 256);
        *buffer++ = BytecodeSet::PushLiteralVariable;
        *buffer++ = index % 256;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		if(index < BytecodeSet::PushLiteralVariableShortRangeSize)
			return 1;
		return 2 + sizeofExtA(index / 256);
	}

private:
	int index;
};

// StoreLiteralVariable
class StoreLiteralVariable: public InstructionNode
{
public:
	StoreLiteralVariable(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        buffer = encodeExtA(buffer, index / 256);
        *buffer++ = BytecodeSet::StoreLiteralVariable;
        *buffer++ = index % 256;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
        return 2 + sizeofExtA(index / 256);
	}

private:
	int index;
};

// Push temporal
class PushTemporal: public InstructionNode
{
public:
	PushTemporal(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		if(index < 12)
		{
			*buffer++ = BytecodeSet::PushTempShortFirst + index;
			return buffer;
		}

		*buffer++ = BytecodeSet::PushTemporary;
		*buffer++ = index;
		return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return index < 12 ? 1 : 2;
	}

private:
	int index;
};

// StoreTemporal
class StoreTemporal: public InstructionNode
{
public:
	StoreTemporal(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::StoreTemporalVariable;
        *buffer++ = index;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
        return 2;
	}

private:
	int index;
};

// StoreTemporal
class PopStoreTemporal: public InstructionNode
{
public:
	PopStoreTemporal(int index)
		: index(index) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        if(index < BytecodeSet::PopStoreTemporalVariableShortRangeSize)
        {
            *buffer++ = BytecodeSet::PopStoreTemporalVariableShortFirst + index;
            return buffer;
        }

        *buffer++ = BytecodeSet::PopStoreTemporalVariable;
        *buffer++ = index;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
        if(index < BytecodeSet::PopStoreTemporalVariableShortRangeSize)
            return 1;
        return 2;
	}

private:
	int index;
};

// Push a new array
class PushArray: public InstructionNode
{
public:
	PushArray(int count)
		: count(count) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::PushArrayWithElements;
        *buffer++ = count;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 2;
	}

private:
	int count;
};

// Push temporal in vector
class PushTemporalInVector: public InstructionNode
{
public:
	PushTemporalInVector(int index, int vectorIndex)
		: index(index), vectorIndex(vectorIndex) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::PushTemporaryInVector;
        *buffer++ = index;
        *buffer++ = vectorIndex;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 3;
	}

private:
	int index;
    int vectorIndex;
};

// Store temporal in vector
class StoreTemporalInVector: public InstructionNode
{
public:
	StoreTemporalInVector(int index, int vectorIndex)
		: index(index), vectorIndex(vectorIndex) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::StoreTemporalInVector;
        *buffer++ = index;
        *buffer++ = vectorIndex;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 3;
	}

private:
	int index;
    int vectorIndex;
};

// Pop and store temporal in vector
class PopStoreTemporalInVector: public InstructionNode
{
public:
	PopStoreTemporalInVector(int index, int vectorIndex)
		: index(index), vectorIndex(vectorIndex) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::PopStoreTemporalInVector;
        *buffer++ = index;
        *buffer++ = vectorIndex;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 3;
	}

private:
	int index;
    int vectorIndex;
};

// Push n closure temporals
class PushNClosureTemps: public InstructionNode
{
public:
	PushNClosureTemps(int count)
		: count(count) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        *buffer++ = BytecodeSet::PushNTemps;
        *buffer++ = count;
        return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		return 2;
	}

private:
	int count;
};

// Push closure
class PushClosure: public InstructionNode
{
public:
    PushClosure(int numCopied, int numArgs, Label *blockEnd, int numExtensions)
        : numCopied(numCopied), numArgs(numArgs), blockEnd(blockEnd), numExtensions(numExtensions) {}

        virtual uint8_t *encode(uint8_t *buffer)
    	{
            buffer = encodeExtA(buffer, extendAValue());
            buffer = encodeExtB(buffer, extendBValue());

    		*buffer++ = BytecodeSet::PushClosure;
    		*buffer++ =
                ((numExtensions & BytecodeSet::PushClosure_NumExtensionsMask) << BytecodeSet::PushClosure_NumExtensionsShift) |
                ((numArgs & BytecodeSet::PushClosure_NumArgsMask) << BytecodeSet::PushClosure_NumArgsShift) |
                ((numCopied & BytecodeSet::PushClosure_NumCopiedMask) << BytecodeSet::PushClosure_NumCopiedShift);
            *buffer++ = blockSize() & 0xFF;
    		return buffer;
    	}

protected:
    int extendAValue()
    {
        return numCopied / 8 * 16 + numArgs/8;
    }

    int blockSize()
    {
        return blockEnd->getLastPosition() - getLastPosition() - getSize();
    }

    int extendBValue()
    {
        return blockSize() / 256;
    }

	virtual size_t computeMaxSize()
	{
		return 3 + sizeofExtA(extendAValue()) + ExtensibleBytecodeSizeMax;
	}

    virtual size_t computeBetterSize()
	{
		return 3 + sizeofExtA(extendAValue()) + sizeofExtB(extendBValue());
	}

private:
    int numCopied;
    int numArgs;
    Label *blockEnd;
    int numExtensions;
};

// Send message
class SendMessage: public InstructionNode
{
public:
	SendMessage(int selectorIndex, int argumentCount)
		: selectorIndex(selectorIndex), argumentCount(argumentCount) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		if(argumentCount == 0 && selectorIndex < BytecodeSet::SendShortArgs0RangeSize)
		{
			*buffer++ = BytecodeSet::SendShortArgs0First + selectorIndex;
			return buffer;
		}
		if(argumentCount == 1 && selectorIndex < BytecodeSet::SendShortArgs0RangeSize)
		{
			*buffer++ = BytecodeSet::SendShortArgs1First + selectorIndex;
			return buffer;
		}
		if(argumentCount == 2 && selectorIndex < BytecodeSet::SendShortArgs0RangeSize)
		{
			*buffer++ = BytecodeSet::SendShortArgs2First + selectorIndex;
			return buffer;
		}

		if(argumentCount > BytecodeSet::Send_ArgumentCountMask)
			LODTALK_UNIMPLEMENTED();

		if(selectorIndex > BytecodeSet::Send_LiteralIndexMask)
			LODTALK_UNIMPLEMENTED();

		*buffer++ = BytecodeSet::Send;
		*buffer++ = (argumentCount & BytecodeSet::Send_ArgumentCountMask) |
			((selectorIndex & BytecodeSet::Send_LiteralIndexMask) << BytecodeSet::Send_LiteralIndexShift);
		return buffer;
	}

protected:
	virtual size_t computeMaxSize()
	{
		if((argumentCount == 0 && selectorIndex < BytecodeSet::SendShortArgs0RangeSize) ||
			(argumentCount == 1 && selectorIndex < BytecodeSet::SendShortArgs1RangeSize) ||
			(argumentCount == 2 && selectorIndex < BytecodeSet::SendShortArgs2RangeSize))
			return 1;

		size_t count = 2;

		if(argumentCount > BytecodeSet::Send_ArgumentCountMask)
			LODTALK_UNIMPLEMENTED();

 		if(selectorIndex > BytecodeSet::Send_LiteralIndexMask)
			LODTALK_UNIMPLEMENTED();
		return count;
	}

private:
	int selectorIndex;
	int argumentCount;
};

// Super Send message
class SuperSendMessage: public InstructionNode
{
public:
	SuperSendMessage(int selectorIndex, int argumentCount)
		: selectorIndex(selectorIndex), argumentCount(argumentCount) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
		LODTALK_UNIMPLEMENTED();
	}

protected:
	virtual size_t computeMaxSize()
	{
		LODTALK_UNIMPLEMENTED();
	}

private:
	int selectorIndex;
	int argumentCount;
};

// UnconditionalJump
class UnconditionalJump: public InstructionNode
{
public:
	UnconditionalJump(Label *destination)
		: destination(destination) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        auto delta = jumpDeltaValue();
        if(delta == 0)
            return buffer;

        if(1 <= delta && delta <= 8)
        {
            *buffer++ = BytecodeSet::JumpShortFirst + delta - 1;
            return buffer;
        }

        buffer = encodeExtB(buffer, delta / 256);
        *buffer++ = BytecodeSet::Jump;
        *buffer++ = delta % 256;
		return buffer;
	}

protected:
    ptrdiff_t jumpDeltaValue()
    {
        return destination->getPosition() - getPosition() - getSize();
    }

    ptrdiff_t lastJumpDeltaValue()
    {
        return destination->getLastPosition() - getLastPosition() - getLastSize();
    }

	virtual size_t computeMaxSize()
	{
		return 2 + ExtensibleBytecodeSizeMax;
	}

    virtual size_t computeBetterSize()
	{
        auto delta = lastJumpDeltaValue();
        if(delta == 0)
            return 0;
        if(1 <= delta && delta <= 8)
            return 1;
        return 2 + sizeofExtB(delta / 256);
	}

private:
	Label *destination;
};

// ConditionalJump
class ConditionalJump: public InstructionNode
{
public:
	ConditionalJump(Label *destination, bool condition)
		: destination(destination), condition(condition) {}

	virtual uint8_t *encode(uint8_t *buffer)
	{
        auto delta = jumpDeltaValue();
        if(delta == 0)
            return buffer;

        if(1 <= delta && delta <= 8)
        {
            if(condition)
                *buffer++ = BytecodeSet::JumpOnTrueShortFirst + delta - 1;
            else
                *buffer++ = BytecodeSet::JumpOnFalseShortFirst + delta - 1;
            return buffer;
        }

        buffer = encodeExtB(buffer, delta / 256);
        *buffer++ = condition ? BytecodeSet::JumpOnTrue : BytecodeSet::JumpOnFalse;
        *buffer++ = delta % 256;
		return buffer;
	}

protected:
    ptrdiff_t jumpDeltaValue()
    {
        return destination->getPosition() - getPosition() - getSize();
    }

    ptrdiff_t lastJumpDeltaValue()
    {
        return destination->getLastPosition() - getLastPosition() - getLastSize();
    }

	virtual size_t computeMaxSize()
	{
		return 2 + ExtensibleBytecodeSizeMax;
	}

    virtual size_t computeBetterSize()
	{
        auto delta = lastJumpDeltaValue();
        if(delta == 0)
            return 0;
        if(1 <= delta && delta <= 8)
            return 1;
        return 2 + sizeofExtB(delta / 256);
	}

private:
	Label *destination;
    bool condition;
};

// The assembler
Assembler::Assembler()
{
}

Assembler::~Assembler()
{
}

InstructionNode *Assembler::addInstruction(InstructionNode *instruction)
{
	instructionStream.push_back(instruction);
    return instruction;
}

size_t Assembler::addLiteral(Oop newLiteral)
{
	for (size_t i = 0; i < literals.size(); ++i)
	{
		if(literals[i] == newLiteral)
			return i;
	}

	auto ret = literals.size();
	literals.push_back(newLiteral);
	return ret;
}

size_t Assembler::addLiteral(const OopRef &newLiteral)
{
	return addLiteral(newLiteral.oop);
}

size_t Assembler::addLiteralAlways(Oop newLiteral)
{
    auto ret = literals.size();
	literals.push_back(newLiteral);
	return ret;
}

size_t Assembler::addLiteralAlways(const OopRef &newLiteral)
{
    return addLiteralAlways(newLiteral.oop);
}

Label *Assembler::makeLabel()
{
	return new Label();
}

Label *Assembler::makeLabelHere()
{
	auto label = makeLabel();
	putLabel(label);
	return label;
}

void Assembler::putLabel(Label *label)
{
	instructionStream.push_back(label);
}

size_t Assembler::computeInstructionsSize()
{
	// Compute the max size.
	size_t maxSize = 0;
	for(auto &instr : instructionStream)
    {
		maxSize += instr->computeMaxSizeForPosition(maxSize);
        instr->commitSize();
    }

	// Compute the optimal iteratively.
	size_t oldSize = maxSize;
	size_t currentSize = maxSize;
	do
	{
		oldSize = currentSize;
		currentSize = 0;
		for(auto &instr : instructionStream)
			currentSize += instr->computeBetterSizeForPosition(currentSize);
        for(auto &instr : instructionStream)
            instr->commitSize();
	} while(currentSize < oldSize);

	return currentSize;
}

CompiledMethod *Assembler::generate(size_t temporalCount, size_t argumentCount, size_t extraSize)
{
	// Compute the method sizes.
	auto instructionsSize = computeInstructionsSize();
	auto literalCount = literals.size();
	auto methodSize = literalCount*sizeof(void*) + instructionsSize + extraSize;

	// Create the method header
	auto methodHeader = CompiledMethodHeader::create(literalCount, temporalCount, argumentCount);

	// Create the compiled method
	auto compiledMethod = CompiledMethod::newMethodWithHeader(methodSize, methodHeader);

	// Set the compiled method literals
	auto literalData = compiledMethod->getFirstLiteralPointer();
	for(size_t i = 0; i < literals.size(); ++i)
		literalData[i] = literals[i].oop;

	// Encode the bytecode instructions.
	auto instructionBuffer = compiledMethod->getFirstBCPointer();
	auto instructionBufferEnd = instructionBuffer + instructionsSize;
	for(auto &instr : instructionStream)
	{
		instructionBuffer = instr->encode(instructionBuffer);
		if(instructionBuffer > instructionBufferEnd)
		{
			fprintf(stderr, "Fatal error: Memory corruption\n");
			abort();
		}
	}

    if(instructionBuffer < instructionBufferEnd)
        printf("extra size %zu\n", instructionBufferEnd - instructionBuffer);

	return compiledMethod;
}

InstructionNode *Assembler::getLastInstruction()
{
    if(instructionStream.empty())
        return nullptr;
    return instructionStream.back();
}

bool Assembler::isLastReturn()
{
	return !instructionStream.empty() && instructionStream.back()->isReturnInstruction();
}

InstructionNode *Assembler::returnReceiver()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::ReturnReceiver, true));
}

InstructionNode *Assembler::returnTrue()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::ReturnTrue, true));
}

InstructionNode *Assembler::returnFalse()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::ReturnFalse, true));
}

InstructionNode *Assembler::returnNil()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::ReturnNil, true));
}

InstructionNode *Assembler::returnTop()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::ReturnTop, true));
}

InstructionNode *Assembler::blockReturnNil()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::BlockReturnNil, true));
}

InstructionNode *Assembler::blockReturnTop()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::BlockReturnTop, true));
}

InstructionNode *Assembler::popStackTop()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PopStackTop));
}

InstructionNode *Assembler::duplicateStackTop()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::DuplicateStackTop));
}

InstructionNode *Assembler::pushLiteral(Oop literal)
{
    if(literal == nilOop())
        return pushNil();
    if(literal == trueOop())
        return pushTrue();
    if(literal == falseOop())
        return pushFalse();
    if(literal == Oop::encodeSmallInteger(1))
        return pushOne();
    if(literal == Oop::encodeSmallInteger(0))
        return pushZero();

	return pushLiteralIndex(addLiteral(literal));
}

InstructionNode *Assembler::pushLiteralVariable(Oop literalVariable)
{
	return pushLiteralVariableIndex(addLiteral(literalVariable));
}

InstructionNode *Assembler::pushReceiverVariableIndex(int variableIndex)
{
	return addInstruction(new PushReceiverVariable(variableIndex));
}

InstructionNode *Assembler::pushLiteralIndex(int literalIndex)
{
	return addInstruction(new PushLiteral(literalIndex));
}

InstructionNode *Assembler::pushLiteralVariableIndex(int literalVariableIndex)
{
	return addInstruction(new PushLiteralVariable(literalVariableIndex));
}

InstructionNode *Assembler::pushTemporal(int temporalIndex)
{
	return addInstruction(new PushTemporal(temporalIndex));
}

InstructionNode *Assembler::pushNewArray(int arraySize)
{
    return addInstruction(new PushArray(arraySize));
}

InstructionNode *Assembler::pushTemporalInVector(int temporalIndex, int vectorIndex)
{
    return addInstruction(new PushTemporalInVector(temporalIndex, vectorIndex));
}

InstructionNode *Assembler::storeReceiverVariableIndex(int variableIndex)
{
    return addInstruction(new StoreReceiverVariable(variableIndex));
}

InstructionNode *Assembler::storeLiteralVariableIndex(int literalVariableIndex)
{
    return addInstruction(new StoreLiteralVariable(literalVariableIndex));
}

InstructionNode *Assembler::storeTemporal(int temporalIndex)
{
    return addInstruction(new StoreTemporal(temporalIndex));
}

InstructionNode *Assembler::popStoreTemporal(int temporalIndex)
{
    return addInstruction(new PopStoreTemporal(temporalIndex));
}

InstructionNode *Assembler::storeLiteralVariable(Oop literalVariable)
{
    return storeLiteralVariableIndex(addLiteral(literalVariable));
}

InstructionNode *Assembler::pushNClosureTemps(int temporalCount)
{
    return addInstruction(new PushNClosureTemps(temporalCount));
}

InstructionNode *Assembler::storeTemporalInVector(int temporalIndex, int vectorIndex)
{
    return addInstruction(new StoreTemporalInVector(temporalIndex, vectorIndex));
}

InstructionNode *Assembler::popStoreTemporalInVector(int temporalIndex, int vectorIndex)
{
    return addInstruction(new PopStoreTemporalInVector(temporalIndex, vectorIndex));
}

InstructionNode *Assembler::pushClosure(int numCopied, int numArgs, Label *blockEnd, int numExtensions)
{
    return addInstruction(new PushClosure(numCopied, numArgs, blockEnd, numExtensions));
}

InstructionNode *Assembler::pushReceiver()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushReceiver, false));
}

InstructionNode *Assembler::pushThisContext()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushThisContext, false));
}

InstructionNode *Assembler::pushNil()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushNil, false));
}

InstructionNode *Assembler::pushTrue()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushTrue, false));
}

InstructionNode *Assembler::pushFalse()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushFalse, false));
}

InstructionNode *Assembler::pushOne()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushOne, false));
}

InstructionNode *Assembler::pushZero()
{
	return addInstruction(new SingleBytecodeInstruction(BytecodeSet::PushZero, false));
}

InstructionNode *Assembler::sendValue()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageValue, false));
}

InstructionNode *Assembler::sendValueWithArg()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageValueArg, false));
}

InstructionNode *Assembler::add()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageAdd, false));
}

InstructionNode *Assembler::greaterThan()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageGreaterThan, false));
}

InstructionNode *Assembler::greaterEqual()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageGreaterEqual, false));
}

InstructionNode *Assembler::lessEqual()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageLessEqual, false));
}

InstructionNode *Assembler::lessThan()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageLessThan, false));
}

InstructionNode *Assembler::equal()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageEqual, false));
}

InstructionNode *Assembler::notEqual()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageNotEqual, false));
}

InstructionNode *Assembler::identityEqual()
{
    return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageIdentityEqual, false));
}

InstructionNode *Assembler::send(Oop selector, int argumentCount)
{
    for(int i = 0; i < (int)SpecialMessageSelector::SpecialMessageCount; ++i)
    {
        auto specialSelector = getSpecialMessageSelector(SpecialMessageSelector(i));
        if(selector == specialSelector)
            return addInstruction(new SingleBytecodeInstruction(BytecodeSet::SpecialMessageAdd + i, false));
    }
	return addInstruction(new SendMessage(addLiteral(selector), argumentCount));
}

InstructionNode *Assembler::superSend(Oop selector, int argumentCount)
{
	return addInstruction(new SuperSendMessage(addLiteral(selector), argumentCount));
}

InstructionNode *Assembler::jump(Label *destination)
{
    return addInstruction(new UnconditionalJump(destination));
}

InstructionNode *Assembler::jumpOnTrue(Label *destination)
{
    return addInstruction(new ConditionalJump(destination, true));
}

InstructionNode *Assembler::jumpOnFalse(Label *destination)
{
    return addInstruction(new ConditionalJump(destination, false));
}
} // End of namespace MethodAssembler
} // End of namespace Lodtalk
