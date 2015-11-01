#ifndef LODEN_STACK_MEMORY_HPP
#define LODEN_STACK_MEMORY_HPP

#include <vector>
#include <functional>
#include "Object.hpp"

namespace Lodtalk
{
static const size_t StackMemoryPageSize = 4096; // 4 KB

class CompiledMethod;

namespace InterpreterStackFrame
{
	static const int WordSize = sizeof(void*);

	static const int LastArgumentOffset = 2*WordSize;
	static const int ReturnInstructionPointerOffset = WordSize;
	static const int PrevFramePointerOffset = 0;
	static const int MethodOffset = -WordSize;
	static const int MetadataOffset = -2*WordSize;
	static const int ThisContextOffset = -3*WordSize;
	static const int ReceiverOffset = -4*WordSize;
	static const int FirstTempOffset = -5*WordSize;
}

inline uintptr_t encodeFrameMetaData(bool hasContext, bool isBlock, size_t numArguments)
{
	return (numArguments & 0xFF) |
		  ((isBlock & 0xFF) << 8) |
		  ((hasContext & 0xFF) << 16);
}

inline void decodeFrameMetaData(uintptr_t metadata, bool &hasContext, bool &isBlock, size_t &numArguments)
{
	numArguments = metadata & 0xFF;
	isBlock = (metadata >> 8) & 0xFF;
	hasContext = (metadata >> 16) & 0xFF;
}


/**
 * Stack memory commited page
 */
class StackMemoryCommitedPage: public Object
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * Stack frame
 */
class StackFrame
{
public:
	StackFrame(uint8_t *framePointer = nullptr, uint8_t *stackPointer = nullptr)
		: framePointer(framePointer), stackPointer(stackPointer) {}
	~StackFrame() {}

	inline uint8_t *getPrevFramePointer()
	{
		return *reinterpret_cast<uint8_t**> (framePointer + InterpreterStackFrame::PrevFramePointerOffset);
	}

	inline CompiledMethod *getMethod()
	{
		return *reinterpret_cast<CompiledMethod**> (framePointer + InterpreterStackFrame::MethodOffset);
	}

	inline Oop getReceiver()
	{
		return *reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ReceiverOffset);
	}

	inline Oop getThisContext()
	{
		return *reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ThisContextOffset);
	}

    inline void setThisContext(Oop newContext)
    {
        *reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ThisContextOffset) = newContext;
    }

    inline Oop getArgumentAtReverseIndex(size_t index)
    {
        return reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::LastArgumentOffset)[index];
    }

	inline uintptr_t getMetadata()
	{
		return *reinterpret_cast<uintptr_t*> (framePointer + InterpreterStackFrame::MetadataOffset);
	}

    inline void setMetadata(uintptr_t newMetadata)
    {
        *reinterpret_cast<uintptr_t*> (framePointer + InterpreterStackFrame::MetadataOffset) = newMetadata;
    }

	inline StackFrame getPreviousFrame()
	{
		return StackFrame(getPrevFramePointer(), framePointer + InterpreterStackFrame::LastArgumentOffset);
	}

	inline Oop stackOopAtOffset(size_t offset)
	{
		return *reinterpret_cast<Oop*> (stackPointer + offset);
	}

	inline Oop stackTop()
	{
		return *reinterpret_cast<Oop*> (stackPointer);
	}

	template<typename FT>
	inline void oopElementsDo(const FT &f)
	{
		// This method
		f(*reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::MethodOffset));

		// This context
		f(*reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ThisContextOffset));

		// Frame elements
		Oop *frameElementsStart = reinterpret_cast<Oop*> (stackPointer);
		Oop *frameElementsEnd = reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ThisContextOffset);
		for(Oop *pos = frameElementsStart; pos <= frameElementsEnd; ++pos)
			f(*pos);
	}

    void marryFrame();

    inline int getArgumentCount()
    {
        return getMetadata() & 0xFF;
    }

    inline bool isBlockActivation()
    {
        return getMetadata() & 0xFF00;
    }

    inline bool hasContext()
    {
        return getMetadata() & 0xFF000;
    }

    inline void ensureFrameIsMarried()
    {
        if(!hasContext())
            marryFrame();
    }

	uint8_t *framePointer;
	uint8_t *stackPointer;
};

/**
 * Stack memory for a single thread.
 */
class StackMemory
{
public:
	StackMemory();
	~StackMemory();

	void setStorage(uint8_t *storage, size_t storageSize);

public:
	inline size_t getStackSize() const
	{
		return stackPageHighest - stackFrame.stackPointer;
	}

	inline size_t getAvailableCapacity() const
	{
		return stackFrame.stackPointer - stackPageLowest;
	}

	inline void pushOop(Oop oop)
	{
		stackFrame.stackPointer -= sizeof(Oop);
		*reinterpret_cast<Oop*> (stackFrame.stackPointer) = oop;
	}

	inline void pushPointer(uint8_t *pointer)
	{
		stackFrame.stackPointer -= sizeof(pointer);
		*reinterpret_cast<uint8_t**> (stackFrame.stackPointer) = pointer;
	}

	inline void pushUInt(uintptr_t value)
	{
		stackFrame.stackPointer -= sizeof(value);
		*reinterpret_cast<uintptr_t*> (stackFrame.stackPointer) = value;
	}

	inline void pushInt(intptr_t value)
	{
		stackFrame.stackPointer -= sizeof(value);
		*reinterpret_cast<intptr_t*> (stackFrame.stackPointer) = value;
	}

	inline uint8_t *stackPointerAt(size_t offset)
	{
		return *reinterpret_cast<uint8_t**> (stackFrame.stackPointer + offset);
	}

	inline Oop stackOopAtOffset(size_t offset)
	{
		return stackFrame.stackOopAtOffset(offset);
	}

	inline Oop stackTop()
	{
		return stackFrame.stackTop();
	}

	inline uintptr_t stackUIntTop(size_t offset)
	{
		return *reinterpret_cast<uintptr_t*> (stackFrame.stackPointer + offset);
	}

	inline Oop popOop()
	{
		auto res = stackOopAtOffset(0);
		stackFrame.stackPointer += sizeof(Oop);
		return res;
	}

	inline uintptr_t popUInt()
	{
		auto res = stackUIntTop(0);
		stackFrame.stackPointer += sizeof(uintptr_t);
		return res;
	}

	inline void popMultiplesOops(int count)
	{
		stackFrame.stackPointer += count * sizeof(Oop);
	}

	inline uint8_t *popPointer()
	{
		auto result = stackPointerAt(0);
		stackFrame.stackPointer += sizeof(uint8_t*);
		return result;
	}

	inline uint8_t *getStackPointer()
	{
		return stackFrame.stackPointer;
	}

	inline uint8_t *getFramePointer()
	{
		return stackFrame.framePointer;
	}

	inline void setFramePointer(uint8_t *newPointer)
	{
		stackFrame.framePointer = newPointer;
	}

	inline void setStackPointer(uint8_t *newPointer)
	{
		stackFrame.stackPointer = newPointer;
	}

	inline uint8_t *getPrevFramePointer()
	{
		return stackFrame.getPrevFramePointer();
	}

	inline StackFrame getCurrentFrame()
	{
		return stackFrame;
	}

	inline CompiledMethod *getMethod()
	{
		return stackFrame.getMethod();
	}

	inline Oop getReceiver()
	{
		return stackFrame.getReceiver();
	}

	inline Oop getThisContext()
	{
		return stackFrame.getThisContext();
	}

	inline uintptr_t getMetadata()
	{
		return stackFrame.getMetadata();;
	}

    void ensureFrameIsMarried()
    {
        stackFrame.ensureFrameIsMarried();
    }

	template<typename FT>
	inline void stackFramesDo(const FT &function)
	{
		auto currentFrame = stackFrame;

		while(currentFrame.framePointer)
		{
			function(currentFrame);
			currentFrame = currentFrame.getPreviousFrame();
		}
	}
private:
	uint8_t *stackPageLowest;
	uint8_t *stackPageHighest;
	size_t stackPageSize;

	StackFrame stackFrame;

};

typedef std::function<void (StackMemory*) > StackMemoryEntry;

void withStackMemory(const StackMemoryEntry &entryPoint);
std::vector<StackMemory*> getAllStackMemories();
StackMemory *getCurrentStackMemory();

} // End of namespace Lodtalk

#endif //LODEN_STACK_MEMORY_HPP
