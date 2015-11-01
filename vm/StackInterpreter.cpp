#include <stdarg.h>
#include <math.h>
#include "StackMemory.hpp"
#include "StackInterpreter.hpp"
#include "PreprocessorHacks.hpp"
#include "BytecodeSets.hpp"
#include "Exception.hpp"
#include "Common.hpp"

namespace Lodtalk
{

/**
 * The stack interpreter.
 * This stack interpreter is based in the arquitecture of squeak.
 */
class StackInterpreter
{
public:
	StackInterpreter(StackMemory *stack);
	~StackInterpreter();

	Oop interpretMethod(CompiledMethod *method, Oop receiver, int argumentCount, Oop *arguments);
    Oop interpretBlockClosure(BlockClosure *closure, int argumentCount, Oop *arguments);

	void interpret();

private:
	void error(const char *message)
	{
		fprintf(stdout, "Interpreter error: %s\n", message);
		abort();
	}

	void errorFormat(const char *format, ...)
	{
		char buffer[1024];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, 1024, format, args);
		error(buffer);
		abort();
	}

	int fetchByte()
	{
		return getInstructionBasePointer()[pc++];
	}

	int fetchSByte()
	{
		return reinterpret_cast<int8_t*> (getInstructionBasePointer())[pc++];
	}

	void fetchNextInstructionOpcode()
	{
		nextOpcode = fetchByte();
	}

	size_t getStackSize() const
	{
		return stack->getStackSize();
	}

	size_t getAvailableCapacity() const
	{
		return stack->getAvailableCapacity();
	}

	void pushOop(Oop object)
	{
		stack->pushOop(object);
	}

	void pushPointer(uint8_t *pointer)
	{
		stack->pushPointer(pointer);
	}

	void pushUInt(uintptr_t value)
	{
		stack->pushUInt(value);
	}

    void pushIntegerObject(int64_t value)
    {
        auto oop = signedInt64ObjectFor(value);
        pushOop(oop);

        // The GC could have been triggered?
        if(!oop.isSmallInteger())
            fetchFrameData();
    }

    void pushFloatObject(double value)
    {
        auto oop = floatObjectFor(value);
        pushOop(oop);

        // The GC could have been triggered?
        if(!oop.isSmallFloat())
            fetchFrameData();
    }
    void pushBoolean(bool value)
    {
        pushOop(value ? trueOop() : falseOop());
    }

	void pushPC()
	{
		pushUInt(pc);
	}

	void popPC()
	{
		pc = popUInt();
	}

	uint8_t *getInstructionBasePointer()
	{
		return reinterpret_cast<uint8_t*> (method);
	}

	Oop currentReceiver() const
	{
		return stack->getReceiver();
	}

	Oop popOop()
	{
		return stack->popOop();
	}

    void popMultiplesOops(size_t n)
    {
        stack->popMultiplesOops(n);
    }

	uint8_t *popPointer()
	{
		return stack->popPointer();
	}

	uintptr_t popUInt()
	{
		return stack->popUInt();
	}

	Oop stackTop()
	{
		return stack->stackTop();
	}

	Oop stackOopAtOffset(size_t offset)
	{
		return stack->stackOopAtOffset(offset);
	}

    Oop stackOopAt(size_t index)
	{
		return stackOopAtOffset(index*sizeof(Oop));
	}

	void condJumpOnNotBoolean(bool jumpType)
	{
		LODTALK_UNIMPLEMENTED();
	}

	void nonLocalReturnValue(Oop value)
	{
		LODTALK_UNIMPLEMENTED();
	}

    void localReturnValue(Oop value)
    {
        // Restore the stack into beginning of the frame pointer.
		stack->setStackPointer(stack->getFramePointer());
		stack->setFramePointer(stack->popPointer());

		// Pop the return pc
		popPC();

		// Pop the arguments and the receiver.
		stack->popMultiplesOops(argumentCount + 1);

		// Push the return value.
		pushOop(value);

		// If there is no, then it means that we are returning.
		if(pc)
		{
			// Re fetch the frame data to continue.
			fetchFrameData();

			// Fetch the next instruction
			fetchNextInstructionOpcode();
		}

    }

	void returnValue(Oop value)
	{
		// Special handling for non-local returns.
		if(isBlock)
			nonLocalReturnValue(value);
        else
            localReturnValue(value);
	}

	void blockReturnValue(Oop value)
	{
		localReturnValue(value);
	}

	void activateMethodFrame(CompiledMethod *method);
    void activateBlockClosure(BlockClosure *closure);
	void fetchFrameData();

private:
	// Use the stack memory.
	StackMemory *stack;

	// Interpreter data.
	size_t pc;
	int nextOpcode;
	int currentOpcode;

	// Instruction decoding.
	uint64_t extendA;
	int64_t extendB;

	// Frame meta data.
	Oop *literalArray;
	CompiledMethod *method;
	bool hasContext;
	bool isBlock;
	size_t argumentCount;


private:
	CompiledMethod *getMethod()
	{
		return method;
	}

	Oop getInstanceVariable(int index)
	{
		return reinterpret_cast<Oop*> (currentReceiver().getFirstFieldPointer())[index];
	}

	void setInstanceVariable(int index, Oop value)
	{
		reinterpret_cast<Oop*> (currentReceiver().getFirstFieldPointer())[index] = value;
	}

	Oop getLiteral(int index)
	{
		return literalArray[index];
	}

	ptrdiff_t getTemporaryOffset(size_t index)
	{
		if(index < argumentCount)
			return InterpreterStackFrame::LastArgumentOffset + ptrdiff_t(argumentCount - index - 1) *sizeof (Oop);
		else
			return InterpreterStackFrame::FirstTempOffset - ptrdiff_t(index - argumentCount) *sizeof (Oop);
	}

	Oop getTemporary(size_t index)
	{
		return *reinterpret_cast<Oop*> (stack->getFramePointer() + getTemporaryOffset(index));
	}

	void setTemporary(size_t index, Oop value)
	{
		*reinterpret_cast<Oop*> (stack->getFramePointer() + getTemporaryOffset(index)) = value;
	}

	void pushReceiverVariable(int receiverVarIndex)
	{
		auto localVar = getInstanceVariable(receiverVarIndex);
		pushOop(localVar);
	}

	void pushLiteralVariable(int literalVarIndex)
	{
		auto literal = getLiteral(literalVarIndex);

		// Cast the literal variable and push the value
		auto literalVar = reinterpret_cast<LiteralVariable*> (literal.pointer);
		pushOop(literalVar->value);
	}

	void pushLiteral(int literalIndex)
	{
		auto literal = getLiteral(literalIndex);
		pushOop(literal);
	}

	void pushTemporary(int temporaryIndex)
	{
		auto temporary = getTemporary(temporaryIndex);
		pushOop(temporary);
	}

    void storeLiteralVariable(int variableIndex, Oop value)
    {
        auto literal = getLiteral(variableIndex);

        // Cast the literal variable and set its value.
        auto literalVar = reinterpret_cast<LiteralVariable*> (literal.pointer);
        literalVar->value = value;
    }

    void backwardJump(int delta)
    {
        pc += delta;
        fetchNextInstructionOpcode();

        // TODO: This is an opportunity to stop myself.
    }

	void sendSelectorArgumentCount(Oop selector, int argumentCount)
	{
		assert((size_t)argumentCount <= CompiledMethodHeader::ArgumentMask);

		// Get the receiver.
		auto newReceiver = stack->stackOopAtOffset(argumentCount * sizeof(Oop));
        auto newReceiverClassIndex = classIndexOf(newReceiver);
		//printf("Send #%s [%s]%p\n", getByteSymbolData(selector).c_str(), getClassNameOfObject(newReceiver).c_str(), newReceiver.pointer);

        // This could be a block context activation.
        if(newReceiverClassIndex == SCI_BlockClosure && selector == getBlockActivationSelector(argumentCount))
        {
            auto blockClosure = reinterpret_cast<BlockClosure*> (newReceiver.pointer);
            if((int)blockClosure->getArgumentCount() == argumentCount)
            {
                // Push the return PC.
    			pushPC();

                // Activate the block closure.
    			activateBlockClosure(blockClosure);
                return;
            }
        }

		// Find the called method
		auto calledMethodOop = lookupMessage(newReceiver, selector);
		if(calledMethodOop.isNil())
		{
			// TODO: Send a DNU
            printf("TODO: Send DNU  #%s [%s]%p\n", getByteSymbolData(selector).c_str(), getClassNameOfObject(newReceiver).c_str(), newReceiver.pointer);
			LODTALK_UNIMPLEMENTED();
		}

		// Get the called method type
		auto methodClassIndex = classIndexOf(calledMethodOop);
		if(methodClassIndex == SCI_CompiledMethod)
		{
			// Push the return PC.
			pushPC();

			// Activate the new compiled method.
			auto compiledMethod = reinterpret_cast<CompiledMethod*> (calledMethodOop.pointer);
			activateMethodFrame(compiledMethod);
		}
		else if(methodClassIndex == SCI_NativeMethod)
		{
			// Reverse the argument order.
			Oop nativeMethodArgs[CompiledMethodHeader::ArgumentMask];
			for(int i = 0; i < argumentCount; ++i)
				nativeMethodArgs[argumentCount - i - 1] = stack->stackOopAtOffset(i*sizeof(Oop));

			// Call the native method
			auto nativeMethod = reinterpret_cast<NativeMethod*> (calledMethodOop.pointer);
			Oop result = nativeMethod->execute(newReceiver, argumentCount, nativeMethodArgs);

			// Pop the arguments and the receiver.
			popMultiplesOops(argumentCount + 1);

			// Push the result in the stack.
			pushOop(result);

			// Re fetch the frame data to continue.
			fetchFrameData();

			// Fetch the next instruction
			fetchNextInstructionOpcode();
		}
		else
		{
			// TODO: Send Send run:with:in:
			LODTALK_UNIMPLEMENTED();
		}
	}

    void sendLiteralIndexArgumentCount(int literalIndex, int argumentCount)
    {
        auto selector = getLiteral(literalIndex);
        sendSelectorArgumentCount(selector, argumentCount);
    }

    void sendSpecialArgumentCount(SpecialMessageSelector specialSelectorId, int argumentCount)
    {
        sendSelectorArgumentCount(getSpecialMessageSelector(specialSelectorId), argumentCount);
    }

	// Bytecode instructions
	void interpretPushReceiverVariableShort()
	{
		fetchNextInstructionOpcode();

		// Fetch the variable index.
		auto variableIndex = currentOpcode & 0xF;
		pushReceiverVariable(variableIndex);
	}

	void interpretPushLiteralVariableShort()
	{
		fetchNextInstructionOpcode();

		// Fetch the literal index
		auto literalVarIndex = currentOpcode & 0xF;
		pushLiteralVariable(literalVarIndex);
	}

	void interpretPushLiteralShort()
	{
		fetchNextInstructionOpcode();

		// Fetch the literal index
		auto literalIndex = currentOpcode & 0x1F;
		pushLiteral(literalIndex);
	}

	void interpretPushTempShort()
	{
		fetchNextInstructionOpcode();

		// Fetch the temporal index
		auto tempIndex = currentOpcode - BytecodeSet::PushTempShortFirst;
		pushTemporary(tempIndex);
	}

	void interpretSendShortArgs0()
	{
		// Fetch the literal index
		auto literalIndex = currentOpcode & 0x0F;
		sendLiteralIndexArgumentCount(literalIndex, 0);
	}

	void interpretSendShortArgs1()
	{
		// Fetch the literal index
		auto literalIndex = currentOpcode & 0x0F;
		sendLiteralIndexArgumentCount(literalIndex, 1);
	}

	void interpretSendShortArgs2()
	{
		// Fetch the literal index
		auto literalIndex = currentOpcode & 0x0F;
		sendLiteralIndexArgumentCount(literalIndex, 2);
	}

	void interpretJumpShort()
	{
		auto delta = (currentOpcode & 7) + 1;
		pc += delta;
        fetchNextInstructionOpcode();
	}

	void interpretJumpOnTrueShort()
	{
		// Fetch the condition and the next instruction opcode
		fetchNextInstructionOpcode();
		auto condition = popOop();

		// Perform the branch when requested.
		if(condition == trueOop())
		{
			auto delta = (currentOpcode & 7);
			pc += delta;
			fetchNextInstructionOpcode();
		}
		else if(condition != falseOop())
		{
			// If the condition is not a boolean, trap
            pushOop(condition);
			condJumpOnNotBoolean(true);
		}
	}

	void interpretJumpOnFalseShort()
	{
		// Fetch the condition and the next instruction opcode
		fetchNextInstructionOpcode();
		auto condition = popOop();

		// Perform the branch when requested.
		if(condition == falseOop())
		{
			auto delta = (currentOpcode & 7);
			pc += delta;
			fetchNextInstructionOpcode();
		}
		else if(condition != trueOop())
		{
			// If the condition is not a boolean, trap
            pushOop(condition);
			condJumpOnNotBoolean(false);
		}
	}

	void interpretPopStoreReceiverVariableShort()
	{
        fetchNextInstructionOpcode();
        auto variableIndex = currentOpcode & 7;
        setInstanceVariable(variableIndex, popOop());
	}

	void interpretPopStoreTemporalVariableShort()
	{
        fetchNextInstructionOpcode();
        auto temporalIndex = currentOpcode & 7;

        setTemporary(temporalIndex, popOop());
	}

	void interpretPushReceiver()
	{
		fetchNextInstructionOpcode();
		pushOop(currentReceiver());
	}

	void interpretPushTrue()
	{
		fetchNextInstructionOpcode();
		pushOop(trueOop());
	}

	void interpretPushFalse()
	{
		fetchNextInstructionOpcode();
		pushOop(falseOop());
	}

	void interpretPushNil()
	{
		fetchNextInstructionOpcode();
		pushOop(nilOop());
	}

	void interpretPushZero()
	{
		fetchNextInstructionOpcode();
		pushOop(Oop::encodeSmallInteger(0));
	}

	void interpretPushOne()
	{
		fetchNextInstructionOpcode();
		pushOop(Oop::encodeSmallInteger(1));
	}

	void interpretPushThisContext()
	{
		LODTALK_UNIMPLEMENTED();
	}

	void interpretDuplicateStackTop()
	{
		fetchNextInstructionOpcode();
		pushOop(stackTop());
	}

	void interpretReturnReceiver()
	{
		returnValue(currentReceiver());
	}

	void interpretReturnTrue()
	{
		returnValue(trueOop());
	}

	void interpretReturnFalse()
	{
		returnValue(falseOop());
	}

	void interpretReturnNil()
	{
		returnValue(nilOop());
	}

	void interpretReturnTop()
	{
		returnValue(popOop());
	}

	void interpretBlockReturnNil()
	{
		blockReturnValue(nilOop());
	}

	void interpretBlockReturnTop()
	{
		blockReturnValue(popOop());
	}

	void interpretNop()
	{
		fetchNextInstructionOpcode();

		// Do nothing
	}

	void interpretPopStackTop()
	{
		fetchNextInstructionOpcode();

		// Pop the element from the stack.
		popOop();
	}

	void interpretExtendA()
	{
		extendA = extendA*256 + fetchByte();
		fetchNextInstructionOpcode();
	}

	void interpretExtendB()
	{
		extendB = extendB*256 + fetchSByte();
		fetchNextInstructionOpcode();
	}

	void interpretPushReceiverVariable()
	{
        auto variableIndex = fetchByte() + extendA*256;
        fetchNextInstructionOpcode();
        extendA = 0;

        pushReceiverVariable(variableIndex);
	}

	void interpretPushLiteralVariable()
	{
        auto literalVariableIndex = fetchByte() + extendA*256;
        fetchNextInstructionOpcode();
        extendA = 0;

        pushLiteralVariable(literalVariableIndex);
	}

	void interpretPushLiteral()
	{
        auto literalIndex = fetchByte() + extendA*256;
        fetchNextInstructionOpcode();
        extendA = 0;

        pushLiteral(literalIndex);
	}

	void interpretPushTemporary()
	{
		auto tempIndex = fetchByte();
		fetchNextInstructionOpcode();

		pushTemporary(tempIndex);
	}

	void interpretPushNTemps()
	{
        auto ntemps = fetchByte();
        fetchNextInstructionOpcode();

        for(int i = 0; i < ntemps; ++i)
            pushOop(Oop());
	}

	void interpretPushInteger()
	{
        auto value = fetchSByte() + extendB*256;
        fetchNextInstructionOpcode();
        extendB = 0;

        pushIntegerObject(value);
	}

	void interpretPushCharacter()
	{
        auto value = fetchSByte() + extendB*256;
        fetchNextInstructionOpcode();
        extendB = 0;

        pushOop(Oop::encodeCharacter(value));
	}

	void interpretPushArrayWithElements()
	{
        auto arraySizeAndFlag = fetchByte();
        fetchNextInstructionOpcode();

        // Decode the array size.
        auto arraySize = arraySizeAndFlag & 127;
        auto popElements = arraySizeAndFlag & 128;

        // Refetch the frame data
        auto array = Array::basicNativeNew(arraySize);
        fetchFrameData();

        // Pop elements into the array.
        if(popElements)
        {
            auto data = reinterpret_cast<Oop*> (array->getFirstFieldPointer());
            for(auto i = 0; i < arraySize; ++i)
                data[i] = popOop();
        }

        // Push the array.
        pushOop(Oop::fromPointer(array));
	}

	void interpretSend()
	{
		// Fetch the data.
		int data = fetchByte();

		// Decode the literal index and argument index
		auto argumentCount = (data & BytecodeSet::Send_ArgumentCountMask) + extendB * BytecodeSet::Send_ArgumentCountCount;
		auto literalIndex = ((data >> BytecodeSet::Send_LiteralIndexShift) & BytecodeSet::Send_LiteralIndexMask) + extendA * BytecodeSet::Send_LiteralIndexCount;

		// Clear the extension values.
		extendA = 0;
		extendB = 0;

		// Send the message
		sendLiteralIndexArgumentCount(literalIndex, argumentCount);
	}

	void interpretSuperSend()
	{
		LODTALK_UNIMPLEMENTED();
	}

	void interpretTrapOnBehavior()
	{
		LODTALK_UNIMPLEMENTED();
	}

	void interpretJump()
	{
        auto delta = fetchSByte() + extendB*256;
        extendB = 0;

        if(delta < 0)
        {
            backwardJump(delta);
        }
        else
        {
            pc += delta;
            fetchNextInstructionOpcode();
        }
	}

	void interpretJumpOnTrue()
	{
        auto delta = fetchSByte() + extendB*256 - 1;
        extendB = 0;
        fetchNextInstructionOpcode();

        auto condition = popOop();
        if(condition == trueOop())
        {
            if(delta < 0)
            {
                backwardJump(delta);
            }
            else
            {
                pc += delta;
                fetchNextInstructionOpcode();
            }
        }
        else if(condition != trueOop())
		{
			// If the condition is not a boolean, trap
            pushOop(condition);
			condJumpOnNotBoolean(true);
		}
	}

	void interpretJumpOnFalse()
	{
        auto delta = fetchSByte() + extendB*256 - 1;
        extendB = 0;
        fetchNextInstructionOpcode();

        auto condition = popOop();
        if(condition == falseOop())
        {
            if(delta < 0)
            {
                backwardJump(delta);
            }
            else
            {
                pc += delta;
                fetchNextInstructionOpcode();
            }
        }
        else if(condition != trueOop())
		{
			// If the condition is not a boolean, trap
            pushOop(condition);
			condJumpOnNotBoolean(false);
		}
	}

	void interpretPopStoreReceiverVariable()
	{
		LODTALK_UNIMPLEMENTED();
	}

	void interpretPopStoreLiteralVariable()
	{
		LODTALK_UNIMPLEMENTED();
	}

	void interpretPopStoreTemporalVariable()
	{
        // Fetch the instruction data.
        auto temporalIndex = fetchByte();
        fetchNextInstructionOpcode();

        // Set the temporary
        setTemporary(temporalIndex, popOop());
	}

	void interpretStoreReceiverVariable()
	{
        // Fetch the instruction data.
        auto variableIndex = fetchByte() + extendA*256;
        fetchNextInstructionOpcode();
        extendA = 0;

        setInstanceVariable(variableIndex, stackOopAt(0));
	}

	void interpretStoreLiteralVariable()
	{
        // Fetch the instruction data.
        auto literalVariableIndex = fetchByte() + extendA*256;
        fetchNextInstructionOpcode();
        extendA = 0;

        storeLiteralVariable(literalVariableIndex, stackOopAt(0));
	}

	void interpretStoreTemporalVariable()
	{
        auto temporalIndex = fetchByte();
        fetchNextInstructionOpcode();

        setTemporary(temporalIndex, stackOopAt(0));
	}

    void interpretPushTemporaryInVector()
    {
        // Fetch the instruction data.
        auto temporalIndex = fetchByte();
        auto vectorIndex = fetchByte();
        fetchNextInstructionOpcode();

        // Get the temporary vector
        Oop vector = getTemporary(vectorIndex);
        assert(classIndexOf(vector) == SCI_Array);

        // Get the temporary.
        auto vectorData = reinterpret_cast<Oop*> (vector.getFirstFieldPointer());
        pushOop(vectorData[temporalIndex]);
    }

    void interpretStoreTemporalInVector()
    {
        // Fetch the instruction data.
        auto temporalIndex = fetchByte();
        auto vectorIndex = fetchByte();
        fetchNextInstructionOpcode();

        // Get the temporary vector
        Oop vector = getTemporary(vectorIndex);
        assert(classIndexOf(vector) == SCI_Array);

        // Set the temporary.
        auto vectorData = reinterpret_cast<Oop*> (vector.getFirstFieldPointer());
        vectorData[temporalIndex] = stackOopAt(0);
    }

    void interpretPopStoreTemporalInVector()
    {
        // Fetch the instruction data.
        auto temporalIndex = fetchByte();
        auto vectorIndex = fetchByte();
        fetchNextInstructionOpcode();

        // Get the temporary vector
        Oop vector = getTemporary(vectorIndex);
        assert(classIndexOf(vector) == SCI_Array);

        // Set the temporary.
        auto vectorData = reinterpret_cast<Oop*> (vector.getFirstFieldPointer());
        vectorData[temporalIndex] = popOop();
    }

    void interpretPushClosure()
    {
        // Fetch some arguments.
        auto firstByte = fetchByte();
        auto blockSize = fetchByte() + extendB * 256;
        auto startPc = pc;
        pc += blockSize;
        fetchNextInstructionOpcode();

        // Decode more of the arguments.
        auto numCopied = ((firstByte >> BytecodeSet::PushClosure_NumCopiedShift) & BytecodeSet::PushClosure_NumCopiedMask)
            + (extendA/16)*8;
        auto numArgs = ((firstByte >> BytecodeSet::PushClosure_NumArgsShift) & BytecodeSet::PushClosure_NumArgsMask) + (extendA%16)*8;

        // Ensure my frame is married.
        stack->ensureFrameIsMarried();

        // Create the block closure.
        BlockClosure *blockClosure = BlockClosure::create(numCopied);
        fetchFrameData(); // For compaction

        // Set the closure data.
        blockClosure->outerContext = stack->getThisContext();
        blockClosure->startpc = Oop::encodeSmallInteger(startPc);
        blockClosure->numArgs = Oop::encodeSmallInteger(numArgs);

        // Copy some elements into the closure.
        auto closureCopiedElements = blockClosure->copiedData;
        for(size_t i = 0; i < numCopied; ++i)
            closureCopiedElements[numCopied - i - 1] = popOop();

        // Push the closure.
        pushOop(Oop::fromPointer(blockClosure));

        // Reset the extension values.
        extendA = 0;
        extendB = 0;
    }

    // Arithmetic messages.
    void interpretSpecialMessageAdd()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushIntegerObject(ia + ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushOop(Oop::encodeCharacter(ca + cb));
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(fa + fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Add, 1);
        }
    }

    void interpretSpecialMessageMinus()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushIntegerObject(ia - ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushOop(Oop::encodeCharacter(ca + cb));
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(fa + fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Minus, 1);
        }
    }

    void interpretSpecialMessageLessThan()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia < ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca < cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa < fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::LessThan, 1);
        }
    }

    void interpretSpecialMessageGreaterThan()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia > ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca > cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa > fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::GreaterThan, 1);
        }
    }

    void interpretSpecialMessageLessEqual()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia <= ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca <= cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa <= fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::LessEqual, 1);
        }
    }

    void interpretSpecialMessageGreaterEqual()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia >= ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca >= cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa >= fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::GreaterEqual, 1);
        }
    }

    void interpretSpecialMessageEqual()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia == ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca == cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa == fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Equal, 1);
        }
    }

    void interpretSpecialMessageNotEqual()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushBoolean(ia != ib);
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushBoolean(ca != cb);
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushBoolean(fa != fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::NotEqual, 1);
        }
    }

    void interpretSpecialMessageMultiply()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();

            if(!ia || !ib)
            {
                pushOop(Oop::encodeSmallInteger(0));
            }
            else
            {
                auto signA = ia < 0 ? -1 : 1;
                auto signB = ib < 0 ? -1 : 1;
                if(signA == signB)
                {
                    // Positive result
                    if(ia > SmallIntegerMax / ib)
                    {
                        // Overflow
                        LODTALK_UNIMPLEMENTED();
                    }
                    else
                    {
                        pushOop(Oop::encodeSmallInteger(ia*ib));
                    }
                }
                else if(signA == -1)
                {
                    // A is negative, B is positive
                    if(ia < SmallIntegerMin / ib)
                    {
                        // Underflow
                        LODTALK_UNIMPLEMENTED();
                    }
                    else
                    {
                        pushOop(Oop::encodeSmallInteger(ia*ib));
                    }
                }
                else
                {
                    // A is positive, B is negative
                    if(ib < SmallIntegerMin / ia)
                    {
                        // Underflow
                        LODTALK_UNIMPLEMENTED();
                    }
                    else
                    {
                        pushOop(Oop::encodeSmallInteger(ia*ib));
                    }
                }
            }
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushOop(Oop::encodeCharacter(ca * cb));
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(fa * fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Multiply, 1);
        }
    }

    void interpretSpecialMessageDivide()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            if(ia % ib == 0)
            {
                pushOop(Oop::encodeSmallInteger(ia / ib));
            }
            else
            {
                // TODO: Make a fraction.
                LODTALK_UNIMPLEMENTED();
            }
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(fa / fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Divide, 1);
        }
    }

    void interpretSpecialMessageRemainder()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            if(ia % ib == 0)
            {
                pushOop(Oop::encodeSmallInteger(ia % ib));
            }
            else
            {
                // TODO: Make a fraction.
                LODTALK_UNIMPLEMENTED();
            }
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(fa - floor(fa/fb)*fb);
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::Remainder, 1);
        }
    }

    void interpretSpecialMessageMakePoint()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::MakePoint, 1);
    }

    void interpretSpecialMessageBitShift()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            auto result = ib > 0 ? ia << ib : ia >> (-ib);
            pushOop(Oop::encodeSmallInteger(result));
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::BitShift, 1);
        }

    }

    void interpretSpecialMessageIntegerDivision()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            auto div = ia / ib;
            if(ib * div > ia)
                --div;
            pushOop(Oop::encodeSmallInteger(div));
        }
        else if(a.isSmallFloat() && b.isSmallFloat())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto fa = a.decodeSmallFloat();
            auto fb = b.decodeSmallFloat();
            pushFloatObject(floor(fa/fb));
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::IntegerDivision, 1);
        }
    }

    void interpretSpecialMessageBitAnd()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushOop(Oop::encodeSmallInteger(ia & ib));
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushOop(Oop::encodeCharacter(ca & cb));
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::BitAnd, 1);
        }
    }

    void interpretSpecialMessageBitOr()
    {
        Oop a = stackOopAt(1);
        Oop b = stackOopAt(0);

        if(a.isSmallInteger() && b.isSmallInteger())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ia = a.decodeSmallInteger();
            auto ib = b.decodeSmallInteger();
            pushOop(Oop::encodeSmallInteger(ia | ib));
        }
        else if(a.isCharacter() && b.isCharacter())
        {
            fetchNextInstructionOpcode();
            popMultiplesOops(2);

            auto ca = a.decodeCharacter();
            auto cb = b.decodeCharacter();
            pushOop(Oop::encodeCharacter(ca | cb));
        }
        else
        {
            sendSpecialArgumentCount(SpecialMessageSelector::BitAnd, 1);
        }
    }

    // Object accessing
    void interpretSpecialMessageAt()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::At, 1);
    }

    void interpretSpecialMessageAtPut()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::AtPut, 2);
    }

    void interpretSpecialMessageSize()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::Size, 0);
    }

    void interpretSpecialMessageNext()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::Next, 0);
    }

    void interpretSpecialMessageNextPut()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::NextPut, 1);
    }

    void interpretSpecialMessageAtEnd()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::AtEnd, 0);
    }

    void interpretSpecialMessageIdentityEqual()
    {
        fetchNextInstructionOpcode();
        Oop b = popOop();
        Oop a = popOop();
        pushBoolean(a == b);
    }

    void interpretSpecialMessageClass()
    {
        fetchNextInstructionOpcode();
        Oop clazz = getClassFromOop(popOop());
        pushOop(clazz);
    }

    // Block evaluation
    void interpretSpecialMessageValue()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::Value, 0);
    }

    void interpretSpecialMessageValueArg()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::ValueArg, 1);
    }

    void interpretSpecialMessageDo()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::Do, 1);
    }

    // Object instantiation
    void interpretSpecialMessageNew()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::New, 0);
    }

    void interpretSpecialMessageNewArray()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::NewArray, 1);
    }

    void interpretSpecialMessageX()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::X, 0);
    }

    void interpretSpecialMessageY()
    {
        sendSpecialArgumentCount(SpecialMessageSelector::Y, 0);
    }
};

StackInterpreter::StackInterpreter(StackMemory *stack)
	: stack(stack)
{
}

StackInterpreter::~StackInterpreter()
{
}

Oop StackInterpreter::interpretMethod(CompiledMethod *newMethod, Oop receiver, int argumentCount, Oop *arguments)
{
	// Check the argument count
	if(argumentCount != (int)newMethod->getArgumentCount())
		nativeError("invalid suplied argument count.");

	// Push the receiver and the arguments
	pushOop(receiver);
	for(int i = 0; i < argumentCount; ++i)
		pushOop(arguments[i]);

	// Make the method frame.
	pushUInt(0); // Return instruction PC.
	activateMethodFrame(newMethod);

	interpret();

	auto returnValue = popOop();
	return returnValue;
}

Oop StackInterpreter::interpretBlockClosure(BlockClosure *closure, int argumentCount, Oop *arguments)
{
	// Check the argument count
	if(argumentCount != (int)closure->getArgumentCount())
		nativeError("invalid suplied argument count.");

	// Push the closure and the arguments
	pushOop(Oop::fromPointer(closure));
	for(int i = 0; i < argumentCount; ++i)
		pushOop(arguments[i]);

	// Make the closure frame.
	pushUInt(0); // Return instruction PC.
	activateBlockClosure(closure);

	interpret();

	auto returnValue = popOop();
	return returnValue;
}

void StackInterpreter::activateMethodFrame(CompiledMethod *newMethod)
{
	// Get the method header
	auto header = *newMethod->getHeader();
	auto numArguments = header.getArgumentCount();
	auto numTemporals = header.getTemporalCount();

	// Get the receiver
	auto receiver = stackOopAtOffset((1 + numArguments)*sizeof(Oop));

	// Push the frame pointer.
	pushPointer(stack->getFramePointer()); // Return frame pointer.

	// Set the new frame pointer.
	stack->setFramePointer(stack->getStackPointer());

	// Push the method object.
	pushOop(Oop::fromPointer(newMethod));
	this->method = newMethod;

	// Encode frame metadata
	pushUInt(encodeFrameMetaData(false, false, numArguments));

	// Push the nil this context.
	pushOop(Oop());

	// Push the receiver oop.
	pushOop(receiver);

	// Push the nil temporals.
	for(size_t i = 0; i < numTemporals; ++i)
		pushOop(Oop());

	// Fetch the frame data.
	fetchFrameData();

	// Set the instruction pointer.
	pc = newMethod->getFirstPCOffset();

	// Fetch the first instruction opcode
	fetchNextInstructionOpcode();
}

void StackInterpreter::activateBlockClosure(BlockClosure *closure)
{
    int numArguments = closure->numArgs.decodeSmallInteger();

    // Get the receiver and the method.
    auto outerContext = closure->getOuterContext();
    auto newMethod = outerContext->method;
    auto receiver = outerContext->receiver;

    // Push the frame pointer.
	pushPointer(stack->getFramePointer()); // Return frame pointer.

	// Set the new frame pointer.
	stack->setFramePointer(stack->getStackPointer());

	// Push the method object.
	pushOop(Oop::fromPointer(newMethod));
	this->method = newMethod;

    // Encode frame metadata
	pushUInt(encodeFrameMetaData(false, true, numArguments));

	// Push the nil this context.
	pushOop(Oop());

	// Push the receiver oop.
	pushOop(receiver);

    // Copy the elements
    auto copiedElements = closure->getNumberOfElements();
    for(size_t i = 0; i < copiedElements; ++i)
        pushOop(closure->copiedData[i]);

    // Fetch the frame data.
	fetchFrameData();

    // Set the initial pc
    pc = closure->startpc.decodeSmallInteger();

    // Fetch the first instruction opcode
	fetchNextInstructionOpcode();
}

void StackInterpreter::fetchFrameData()
{
	// Decode the frame metadata.
	decodeFrameMetaData(stack->getMetadata(), this->hasContext, this->isBlock, argumentCount);

	// Get the method and the literal array
	method = stack->getMethod();
	literalArray = method->getFirstLiteralPointer();
}

void StackInterpreter::interpret()
{
	// Reset the extensions values
	extendA = 0;
	extendB = 0;
	while(pc != 0)
	{
		currentOpcode = nextOpcode;

		switch(currentOpcode)
		{
#define MAKE_RANGE_CASE(i, from, to, name) \
case i + from:\
	interpret ## name(); \
	break;

#define SISTAV1_INSTRUCTION_RANGE(name, rangeFirst, rangeLast) \
	LODTALK_EVAL(LODTALK_FROM_TO_INCLUSIVE(rangeFirst, rangeLast, MAKE_RANGE_CASE, name))

#define SISTAV1_INSTRUCTION(name, opcode) \
case opcode:\
	interpret ## name(); \
	break;
#include "SistaV1BytecodeSet.inc"
#undef SISTAV1_INSTRUCTION_RANGE
#undef SISTAV1_INSTRUCTION
#undef MAKE_RANGE_CASE

		default:
			errorFormat("unsupported bytecode %d", currentOpcode);
		}
	}
}

Oop interpretCompiledMethod(CompiledMethod *method, Oop receiver, int argumentCount, Oop *arguments)
{
	Oop result;
	withStackMemory([&](StackMemory *stack) {
		StackInterpreter interpreter(stack);
		result = interpreter.interpretMethod(method, receiver, argumentCount, arguments);
	});
	return result;
}

Oop interpretBlockClosure(BlockClosure *closure, int argumentCount, Oop *arguments)
{
	Oop result;
	withStackMemory([&](StackMemory *stack) {
		StackInterpreter interpreter(stack);
		result = interpreter.interpretBlockClosure(closure, argumentCount, arguments);
	});
	return result;
}

} // End of namespace Lodtalk
