#include <stdarg.h>
#include <math.h>
#include "StackMemory.hpp"
#include "StackInterpreter.hpp"
#include "BytecodeSets.hpp"
#include "Lodtalk/Exception.hpp"
#include "Constants.hpp"

namespace Lodtalk
{
class StackInterpreter;

/**
 * Stack interpreter proxy.
 */
class StackInterpreterProxy: public InterpreterProxy
{
public:
    StackInterpreterProxy(StackInterpreter *interpreter)
        : interpreter(interpreter)
    {}

    virtual VMContext *getContext();

    // Stack manipulation
    virtual void pushOop(Oop oop) override;
    virtual void pushSmallInteger(SmallIntegerValue value) override;

	virtual Oop &stackOopAt(size_t index) override;
	virtual Oop &stackTop() override;
    virtual Oop popOop() override;
    virtual void popMultiplesOops(int count) override;
    virtual void duplicateStackTop() override;

    virtual Oop getMethod() override;

	virtual Oop &receiver() override;
    virtual Oop getReceiver() override;

	virtual Oop &thisContext() override;
    virtual Oop getThisContext() override;

    // Returning
    virtual int returnTop() override;
    virtual int returnReceiver() override;
    virtual int returnSmallInteger(SmallIntegerValue value) override;
    virtual int returnOop(Oop value) override;

    // Temporaries
    virtual size_t getArgumentCount() override;
    virtual size_t getTemporaryCount() override;

    virtual Oop getTemporary(size_t index) override;
    virtual void setTemporary(size_t index, Oop value) override;

    // Primitive return
    virtual int primitiveFailed() override;
    virtual int primitiveFailedWithCode(int errorCode) override;

    // Basic new
    virtual void sendBasicNew() override;
    virtual void sendBasicNewWithSize(size_t size) override;

    // Message send.
    virtual void sendMessage(int argumentCount) override;
    virtual void sendMessageWithSelector(Oop selector, int argumentCount) override;

private:
    StackInterpreter *interpreter;
};

/**
 * The stack interpreter.
 * This stack interpreter is based in the arquitecture of squeak.
 */
class StackInterpreter
{
private:
	// Use the stack memory.
    VMContext *context;
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

public:
	StackInterpreter(VMContext *context, StackMemory *stack);
	~StackInterpreter();

	Oop interpretMethod(CompiledMethod *method, Oop receiver, int argumentCount, Oop *arguments);
    Oop interpretBlockClosure(BlockClosure *closure, int argumentCount, Oop *arguments);

	void interpret();

public:
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
        auto oop = context->signedInt64ObjectFor(value);
        pushOop(oop);

        // The GC could have been triggered?
        if(!oop.isSmallInteger())
            fetchFrameData();
    }

    void pushFloatObject(double value)
    {
        auto oop = context->floatObjectFor(value);
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

	const Oop &currentReceiver() const
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

	Oop &stackTop()
	{
		return stack->stackTop();
	}

	Oop &stackOopAtOffset(size_t offset)
	{
		return stack->stackOopAtOffset(offset);
	}

    Oop &stackOopAt(size_t index)
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
		popMultiplesOops(argumentCount + 1);

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

    void returnTop()
    {
        returnValue(popOop());
    }

    void returnReceiver()
    {
        returnValue(currentReceiver());
    }

    void callNativeMethod(NativeMethod *method, size_t argumentCount);
	void activateMethodFrame(CompiledMethod *method);
    void activateBlockClosure(BlockClosure *closure);
	void fetchFrameData();

    VMContext *getContext()
    {
        return context;
    }

    StackMemory *getStack()
    {
        return stack;
    }

    CompiledMethod *getMethod()
	{
		return method;
	}

	Oop getInstanceVariable(size_t index)
	{
		return reinterpret_cast<Oop*> (currentReceiver().getFirstFieldPointer())[index];
	}

	void setInstanceVariable(size_t index, Oop value)
	{
		reinterpret_cast<Oop*> (currentReceiver().getFirstFieldPointer())[index] = value;
	}

	Oop getLiteral(size_t index)
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

	void pushReceiverVariable(size_t receiverVarIndex)
	{
		auto localVar = getInstanceVariable(receiverVarIndex);
		pushOop(localVar);
	}

	void pushLiteralVariable(size_t literalVarIndex)
	{
		auto literal = getLiteral(literalVarIndex);

		// Cast the literal variable and push the value
		auto literalVar = reinterpret_cast<LiteralVariable*> (literal.pointer);
		pushOop(literalVar->value);
	}

	void pushLiteral(size_t literalIndex)
	{
		auto literal = getLiteral(literalIndex);
		pushOop(literal);
	}

	void pushTemporary(size_t temporaryIndex)
	{
		auto temporary = getTemporary(temporaryIndex);
		pushOop(temporary);
	}

    void storeLiteralVariable(size_t variableIndex, Oop value)
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

    Oop lookupMessage(Oop receiver, Oop selector)
    {
    	auto classIndex = classIndexOf(receiver);
    	auto classOop = context->getClassFromIndex(classIndex);
    	assert(!isNil(classOop));

    	// Lookup the method
    	auto behavior = reinterpret_cast<Behavior*> (classOop.pointer);
    	return behavior->lookupSelector(selector);
    }

	void sendSelectorArgumentCount(Oop selector, size_t argumentCount)
	{
		assert(argumentCount <= CompiledMethodHeader::ArgumentMask);

		// Get the receiver.
		auto newReceiver = stack->stackOopAtOffset(argumentCount * sizeof(Oop));
        auto newReceiverClassIndex = classIndexOf(newReceiver);
		//printf("Send #%s [%s]%p\n", context->getByteSymbolData(selector).c_str(), context->getClassNameOfObject(newReceiver).c_str(), newReceiver.pointer);

        // This could be a block context activation.
        if(newReceiverClassIndex == SCI_BlockClosure && selector == context->getBlockActivationSelector(argumentCount))
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
            printf("TODO: Send DNU  #%s [%s]%p\n", context->getByteSymbolData(selector).c_str(), context->getClassNameOfObject(newReceiver).c_str(), newReceiver.pointer);
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
            // Push the return PC.
            pushPC();

			// Call the native method
			auto nativeMethod = reinterpret_cast<NativeMethod*> (calledMethodOop.pointer);
            callNativeMethod(nativeMethod, argumentCount);
		}
		else
		{
			// TODO: Send Send run:with:in:
			LODTALK_UNIMPLEMENTED();
		}
	}

    void sendMessage(int argumentCount)
    {
        sendSelectorArgumentCount(popOop(), argumentCount);
    }

    void sendBasicNew()
    {
        sendSelectorArgumentCount(context->getSpecialMessageSelector(SpecialMessageSelector::BasicNew), 0);
    }

    void sendBasicNewWithSize(size_t size)
    {
        pushUInt(size);
        sendSelectorArgumentCount(context->getSpecialMessageSelector(SpecialMessageSelector::BasicNewSize), 1);
    }

private:
    void sendLiteralIndexArgumentCount(size_t literalIndex, size_t argumentCount)
    {
        auto selector = getLiteral(literalIndex);
        sendSelectorArgumentCount(selector, argumentCount);
    }

    void sendSpecialArgumentCount(SpecialMessageSelector specialSelectorId, int argumentCount)
    {
        sendSelectorArgumentCount(context->getSpecialMessageSelector(specialSelectorId), argumentCount);
    }

	// Bytecode instructions
	void interpretPushReceiverVariableShort(int variableIndex)
	{
		fetchNextInstructionOpcode();

		// Fetch the variable index.
		pushReceiverVariable(variableIndex);
	}

	void interpretPushLiteralVariableShort(int literalVarIndex)
	{
		fetchNextInstructionOpcode();

		// Fetch the literal index
		pushLiteralVariable(literalVarIndex);
	}

	void interpretPushLiteralShort(int literalIndex)
	{
		fetchNextInstructionOpcode();

		// Fetch the literal index
		pushLiteral(literalIndex);
	}

	void interpretPushTempShort(int tempIndex)
	{
		fetchNextInstructionOpcode();

		// Fetch the temporal index
		pushTemporary(tempIndex);
	}

	void interpretSendShortArgs0(int literalIndex)
	{
		// Fetch the literal index
		sendLiteralIndexArgumentCount(literalIndex, 0);
	}

	void interpretSendShortArgs1(int literalIndex)
	{
		// Fetch the literal index
		sendLiteralIndexArgumentCount(literalIndex, 1);
	}

	void interpretSendShortArgs2(int literalIndex)
	{
		// Fetch the literal index
		sendLiteralIndexArgumentCount(literalIndex, 2);
	}

	void interpretJumpShort(int delta)
	{
        ++delta;
		pc += delta;
        fetchNextInstructionOpcode();
	}

	void interpretJumpOnTrueShort(int delta)
	{
		// Fetch the condition and the next instruction opcode
		fetchNextInstructionOpcode();
		auto condition = popOop();

		// Perform the branch when requested.
		if(condition == trueOop())
		{
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

	void interpretJumpOnFalseShort(int delta)
	{
		// Fetch the condition and the next instruction opcode
		fetchNextInstructionOpcode();
		auto condition = popOop();

		// Perform the branch when requested.
		if(condition == falseOop())
		{
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

	void interpretPopStoreReceiverVariableShort(int variableIndex)
	{
        fetchNextInstructionOpcode();
        setInstanceVariable(variableIndex, popOop());
	}

	void interpretPopStoreTemporalVariableShort(int temporalIndex)
	{
        fetchNextInstructionOpcode();

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
		returnReceiver();
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
		returnTop();
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

        pushOop(Oop::encodeCharacter((int)value));
	}

	void interpretPushArrayWithElements()
	{
        auto arraySizeAndFlag = fetchByte();
        fetchNextInstructionOpcode();

        // Decode the array size.
        auto arraySize = arraySizeAndFlag & 127;
        auto popElements = arraySizeAndFlag & 128;

        // Refetch the frame data
        auto array = Array::basicNativeNew(context, arraySize);
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
            backwardJump((int)delta);
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
                backwardJump((int)delta);
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
                backwardJump((int)delta);
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
        BlockClosure *blockClosure = BlockClosure::create(context, (int)numCopied);
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
        Oop clazz = context->getClassFromOop(popOop());
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

StackInterpreter::StackInterpreter(VMContext *context, StackMemory *stack)
	: context(context), stack(stack), pc(0), nextOpcode(0), currentOpcode(0)
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

void StackInterpreter::callNativeMethod(NativeMethod *nativeMethod, size_t argumentCount)
{
    // Get the receiver
	auto receiver = stackOopAtOffset((1 + argumentCount)*sizeof(Oop));

	// Push the frame pointer.
	pushPointer(stack->getFramePointer()); // Return frame pointer.

	// Set the new frame pointer.
	stack->setFramePointer(stack->getStackPointer());

	// Push the method object.
	pushOop(Oop::fromPointer(nativeMethod));
	this->method = nullptr;

	// Encode frame metadata
	pushUInt(encodeFrameMetaData(false, false, argumentCount));

	// Push the nil this context.
	pushOop(Oop());

	// Push the receiver oop.
	pushOop(receiver);

	// Fetch the frame data.
	fetchFrameData();

    // Set the new pc.
    pc = 0;

    // Call the primitive
    StackInterpreterProxy proxy(this);
    nativeMethod->primitive(&proxy);
}

void StackInterpreter::activateBlockClosure(BlockClosure *closure)
{
    int numArguments = (int)closure->numArgs.decodeSmallInteger();

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
#define BYTECODE_DISPATCH_NAME(name) interpret ## name
#include "BytecodeSetDispatchTable.inc"
#undef BYTECODE_DISPATCH_NAME

		default:
			errorFormat("unsupported bytecode %d", currentOpcode);
		}
	}
}

/**
 * Stack interpreter proxy
 */
VMContext *StackInterpreterProxy::getContext()
{
    return interpreter->getContext();
}

void StackInterpreterProxy::pushOop(Oop oop)
{
    interpreter->pushOop(oop);
}

void StackInterpreterProxy::pushSmallInteger(SmallIntegerValue value)
{
    pushOop(Oop::encodeSmallInteger(value));
}

Oop &StackInterpreterProxy::stackOopAt(size_t index)
{
    return interpreter->stackOopAt(index);
}

Oop &StackInterpreterProxy::stackTop()
{
    return interpreter->stackTop();
}

Oop StackInterpreterProxy::popOop()
{
    return interpreter->popOop();
}

void StackInterpreterProxy::popMultiplesOops(int count)
{
    interpreter->popMultiplesOops(count);
}

void StackInterpreterProxy::duplicateStackTop()
{
    interpreter->pushOop(stackTop());
}

Oop StackInterpreterProxy::getMethod()
{
    return Oop::fromPointer(interpreter->getMethod());
}

Oop &StackInterpreterProxy::receiver()
{
    return interpreter->getStack()->receiver();
}

Oop StackInterpreterProxy::getReceiver()
{
    return interpreter->currentReceiver();
}

Oop &StackInterpreterProxy::thisContext()
{
    return interpreter->getStack()->thisContext();
}

Oop StackInterpreterProxy::getThisContext()
{
    return interpreter->getStack()->getThisContext();
}

// Returning
int StackInterpreterProxy::returnTop()
{
    interpreter->returnTop();
    return 0;
}

int StackInterpreterProxy::returnReceiver()
{
    interpreter->returnReceiver();
    return 0;
}

int StackInterpreterProxy::returnSmallInteger(SmallIntegerValue value)
{
    interpreter->returnValue(Oop::encodeSmallInteger(value));
    return 0;
}

int StackInterpreterProxy::returnOop(Oop value)
{
    interpreter->returnValue(value);
    return 0;
}

// Temporaries
size_t StackInterpreterProxy::getArgumentCount()
{
    return interpreter->getStack()->getArgumentCount();
}

size_t StackInterpreterProxy::getTemporaryCount()
{
    auto method = interpreter->getStack()->getMethod();
    if(!isNil(method))
        return method->getTemporalCount();
    return 0;
}

Oop StackInterpreterProxy::getTemporary(size_t index)
{
    return interpreter->getTemporary(index);
}

void StackInterpreterProxy::setTemporary(size_t index, Oop value)
{
    interpreter->setTemporary(index, value);
}

// Primitive return
int StackInterpreterProxy::primitiveFailed()
{
    return primitiveFailedWithCode(-1);
}

int StackInterpreterProxy::primitiveFailedWithCode(int errorCode)
{
    LODTALK_UNIMPLEMENTED();
}

// Basic new
void StackInterpreterProxy::sendBasicNew()
{
    interpreter->sendBasicNew();
}

void StackInterpreterProxy::sendBasicNewWithSize(size_t size)
{
    interpreter->sendBasicNewWithSize(size);
}

// Message send.
void StackInterpreterProxy::sendMessage(int argumentCount)
{
    interpreter->sendMessage(argumentCount);
    interpreter->interpret();
}

void StackInterpreterProxy::sendMessageWithSelector(Oop selector, int argumentCount)
{
    interpreter->sendSelectorArgumentCount(selector, argumentCount);
    interpreter->interpret();
}

void VMContext::withInterpreter(const WithInterpreterBlock &block)
{
    withStackMemory(this, [&](StackMemory *stack) {
		StackInterpreter interpreter(this, stack);
        StackInterpreterProxy proxy(&interpreter);
		block(&proxy);
	});

}
} // End of namespace Lodtalk
