#ifndef LODEN_STACK_MEMORY_HPP
#define LODEN_STACK_MEMORY_HPP

#include "Lodtalk/Object.hpp"
#include "Method.hpp"
#include <vector>
#include <functional>
#include <mutex>

namespace Lodtalk
{
static const size_t StackMemoryPageSize = 4096*2; // 4 KB

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

    static const int FrameFixedDataSize = 6 * WordSize;

}

inline uintptr_t encodeFrameMetaData(bool hasContext, bool isBlock, size_t numArguments)
{
	return (numArguments & 0xFF) |
		  ((isBlock ? 1 : 0) << 8) |
		  ((hasContext ? 1 : 0) << 16);
}

inline void decodeFrameMetaData(uintptr_t metadata, bool &hasContext, bool &isBlock, size_t &numArguments)
{
	numArguments = metadata & 0xFF;
	isBlock = ((metadata >> 8) & 0xFF) != 0;
	hasContext = ((metadata >> 16) & 0xFF) != 0;
}

inline size_t getContextFrameSize(Context *context)
{
    assert(!context->isMarriedOrWidowed());
    size_t result = InterpreterStackFrame::FrameFixedDataSize;
    result += (context->stackp.decodeSmallInteger() + 1) * InterpreterStackFrame::WordSize;
    return result;
}

/**
 * Stack memory commited page
 */
class StackMemoryCommitedPage: public Object
{
public:
    static SpecialNativeClassFactory Factory;
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

    inline void setPrevFramePointer(uint8_t *newPointer)
    {
        *reinterpret_cast<uint8_t**> (framePointer + InterpreterStackFrame::PrevFramePointerOffset) = newPointer;
    }

    inline intptr_t getReturnPointer()
	{
		return *reinterpret_cast<intptr_t*> (framePointer + InterpreterStackFrame::ReturnInstructionPointerOffset);
	}

	inline CompiledMethod *getMethod()
	{
		return *reinterpret_cast<CompiledMethod**> (framePointer + InterpreterStackFrame::MethodOffset);
	}

    inline Oop &receiver()
	{
		return *reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ReceiverOffset);
	}

	inline Oop &thisContext()
	{
		return *reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::ThisContextOffset);
	}

	inline const Oop &getReceiver()
	{
		return receiver();
	}

	inline const Oop &getThisContext()
	{
		return thisContext();
	}

    inline void setThisContext(Oop newContext)
    {
        thisContext() = newContext;
    }

    inline uint8_t *getEndPointer()
    {
        return framePointer + InterpreterStackFrame::LastArgumentOffset + (getArgumentCount() + 1)*sizeof(Oop);
    }

    inline uint8_t *getBeginPointer()
    {
        return stackPointer;
    }

    inline Oop &argumentAtReverseIndex(size_t index)
    {
        return reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::LastArgumentOffset)[index];
    }

    inline const Oop &getArgumentAtReverseIndex(size_t index)
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
		return StackFrame(getPrevFramePointer(), getEndPointer());
	}

	inline Oop &stackOopAtOffset(size_t offset)
	{
		return *reinterpret_cast<Oop*> (stackPointer + offset);
	}

	inline Oop &stackTop()
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
		for(Oop *pos = frameElementsStart; pos < frameElementsEnd; ++pos)
			f(*pos);

        // The arguments.
        auto argumentCount = getArgumentCount() + 1; // Plus the receiver argument.
        Oop *arguments = reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::LastArgumentOffset);
        for (size_t i = 0; i < argumentCount; ++i)
            f(arguments[i]);
	}

    void marryFrame(VMContext *context);
    void updateMarriedSpouseState();

    inline int getArgumentCount()
    {
        return getMetadata() & 0xFF;
    }

    inline bool isBlockActivation()
    {
        return (getMetadata() & 0xFF00) != 0;
    }

    inline bool hasContext()
    {
        return (getMetadata() & 0xFF0000) != 0;
    }

    inline void ensureFrameIsMarried(VMContext *context)
    {
        if(!hasContext())
            marryFrame(context);
    }

    inline void ensureHasUpdatedMarriedSpouse(VMContext *context)
    {
        if (!hasContext())
            marryFrame(context);
        updateMarriedSpouseState();
    }


	uint8_t *framePointer;
	uint8_t *stackPointer;
};

/**
 * Stack memory page
 */
class StackPage
{
public:
    StackPage(VMContext *context, uint8_t *memoryLocation);
    ~StackPage();

    VMContext *getContext()
    {
        return context;
    }

    inline bool isInUse()
    {
        return baseFramePointer != nullptr;
    }

    inline void startUsing()
    {
        baseFramePointer = stackPageHighest;
        headFrame = StackFrame(nullptr, stackPageHighest);
    }
    
    inline void freed()
    {
        baseFramePointer = nullptr;
        headFrame = StackFrame(nullptr, nullptr);

        // Remove myself from the linked list.
        if (previousPage)
            previousPage->nextPage = nextPage;
        if (nextPage)
            nextPage->previousPage = previousPage;
        previousPage = nullptr;
        nextPage = nullptr;
    }

    void divorceAllFrames();

    VMContext *context;

    uint8_t *baseFramePointer;
    StackFrame headFrame;

    uint8_t *stackPageLowest;
    uint8_t *stackPageHighest;
    size_t stackPageSize;
    uint8_t *overflowLimit;
    uint8_t *stackLimit;

    StackPage *previousPage;
    StackPage *nextPage;

};
/**
 * Stack memory for a single thread.
 */
class StackMemory
{
public:
    static constexpr size_t StackPageCount = 16;

	StackMemory(VMContext *context);
	~StackMemory();

    VMContext *getContext()
    {
        return context;
    }

    void createTerminalStackFrame();

public:
	inline size_t getStackSize() const
	{
		return currentPage->stackPageHighest - stackFrame.stackPointer;
	}

	inline size_t getAvailableCapacity() const
	{
		return stackFrame.stackPointer - currentPage->stackPageLowest;
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

	inline Oop &stackOopAtOffset(size_t offset)
	{
		return stackFrame.stackOopAtOffset(offset);
	}

	inline Oop &stackTop()
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

	inline void popMultiplesOops(size_t count)
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

    inline intptr_t getReturnPointer()
    {
        return stackFrame.getReturnPointer();
    }

	inline uint8_t *getPrevFramePointer()
	{
		return stackFrame.getPrevFramePointer();
	}

	inline StackFrame getCurrentFrame()
	{
		return stackFrame;
	}

    inline int getArgumentCount()
    {
        return stackFrame.getArgumentCount();
    }

	inline CompiledMethod *getMethod()
	{
		return stackFrame.getMethod();
	}

	inline const Oop &getReceiver()
	{
		return stackFrame.getReceiver();
	}

    inline Oop &receiver()
	{
		return stackFrame.receiver();
	}

	inline const Oop &getThisContext()
	{
		return stackFrame.getThisContext();
	}

    inline Oop &thisContext()
	{
		return stackFrame.thisContext();
	}

	inline uintptr_t getMetadata()
	{
		return stackFrame.getMetadata();
	}

    void ensureFrameIsMarried()
    {
        stackFrame.ensureFrameIsMarried(context);
    }

	template<typename FT>
	inline void stackFramesDo(const FT &function)
	{
        writeBackFrameData();
        auto page = currentPage;
        for (; page && page->isInUse(); page = page->previousPage)
        {
            auto currentFrame = page->headFrame;

            while (currentFrame.framePointer)
            {
                function(currentFrame);
                currentFrame = currentFrame.getPreviousFrame();
            }
        }
	}

    inline bool checkForOveflowOrEvent()
    {
        if (stackFrame.stackPointer >= currentPage->stackLimit)
            return false;

        stackOverflow();
        return true;
    }

    void stackOverflow();
    void makeBaseFrame(Context *context);
    void writeBackFrameData();

    size_t getPageIndexFor(uint8_t *pointer);
    void useNewPageFor(uint8_t *framePointer);

private:
    StackPage *allocatePage();

    VMContext *context;

    std::vector<StackPage*> stackPages;
    std::vector<StackPage*> freePages;

    StackPage *currentPage;
    StackFrame stackFrame;
    size_t stackSize;
    uint8_t *stackMemoryLowest;
    uint8_t *stackMemoryHighest;
};

// Stack memories interface used by the GC
class StackMemories
{
public:
	std::vector<StackMemory*> getAll()
	{
		std::unique_lock<std::mutex> l(mutex);
		return memories;
	}

	void registerMemory(StackMemory* memory)
	{
		std::unique_lock<std::mutex> l(mutex);
		memories.push_back(memory);
	}

	void unregisterMemory(StackMemory* memory)
	{
		std::unique_lock<std::mutex> l(mutex);
		for(size_t i = 0; i < memories.size(); ++i)
		{
			if(memories[i] == memory)
			{
				memories.erase(memories.begin() + i);
				return;
			}
		}
	}

private:
	std::mutex mutex;
	std::vector<StackMemory*> memories;
};

typedef std::function<void (StackMemory*) > StackMemoryEntry;

void withStackMemory(VMContext *context, const StackMemoryEntry &entryPoint);

} // End of namespace Lodtalk

#endif //LODEN_STACK_MEMORY_HPP
