#include <stdlib.h>
#include <vector>
#include <mutex>
#include "Collections.hpp"
#include "Method.hpp"
#include "StackMemory.hpp"

namespace Lodtalk
{
LODTALK_BEGIN_CLASS_SIDE_TABLE(StackMemoryCommitedPage)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(StackMemoryCommitedPage)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(StackMemoryCommitedPage, Object, OF_INDEXABLE_8, 0);

// Stack frame
void StackFrame::marryFrame()
{
    assert(!hasContext());

    auto method = getMethod();

    // Instantiate the context.
    auto slotCount = method->getHeader()->needsLargeFrame() ? Context::LargeContextSlots : Context::SmallContextSlots;
    auto context = Context::create(slotCount);
    method = getMethod();

    // Get the closure
    Oop closure;
    if(isBlockActivation())
    {
        auto blockArgCount = getArgumentCount();
        closure = getArgumentAtReverseIndex(blockArgCount);
    }

    // Set the context parameters.
    context->sender = Oop::fromPointer(framePointer + 1);
    context->pc = Oop::fromPointer(getPrevFramePointer() + 1);
    context->stackp = Oop::fromPointer(stackPointer + 1);
    context->method = method;
    context->closureOrNil = closure;
    context->receiver = getReceiver();

    // Store the context in this frame.
    setThisContext(Oop::fromPointer(context));
    setMetadata(getMetadata() | (1<<16));

    // Ensure the have context flag is set.
    assert(hasContext());

}

// Stack memory for a single thread.
StackMemory::StackMemory()
{
}

StackMemory::~StackMemory()
{
}

void StackMemory::setStorage(uint8_t *storage, size_t storageSize)
{
	stackPageSize = storageSize;
	stackPageHighest = storage + storageSize;
	stackPageLowest = storage;
	stackFrame = StackFrame(nullptr, stackPageHighest);
}

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

static StackMemories *allStackMemories = nullptr;
static StackMemories *getStackMemoriesData()
{
	if(!allStackMemories)
		allStackMemories = new StackMemories();
	return allStackMemories;
}

// Interface for accessing the stack memory for the current native thread.
static thread_local StackMemory *currentStackMemory = nullptr;

void withStackMemory(const StackMemoryEntry &entryPoint)
{
	if(currentStackMemory)
		return entryPoint(currentStackMemory);

	// Ensure the current stack memory pointer is clear when I finish.
	struct EnsureBlock
	{
		~EnsureBlock()
		{
			getStackMemoriesData()->unregisterMemory(currentStackMemory);
			delete currentStackMemory;
			currentStackMemory = nullptr;
		}
	} ensure;

	currentStackMemory = new StackMemory();
	currentStackMemory->setStorage(reinterpret_cast<uint8_t*> (alloca(StackMemoryPageSize)), StackMemoryPageSize);
	getStackMemoriesData()->registerMemory(currentStackMemory);
	return entryPoint(currentStackMemory);
}

std::vector<StackMemory*> getAllStackMemories()
{
	return getStackMemoriesData()->getAll();
}

StackMemory *getCurrentStackMemory()
{
	return currentStackMemory;
}

} // End of namespace Lodtalk
