#include <stdlib.h>
#include <vector>
#include <mutex>
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/Collections.hpp"
#include "Method.hpp"
#include "StackMemory.hpp"
#include "MemoryManager.hpp"

namespace Lodtalk
{
SpecialNativeClassFactory StackMemoryCommitedPage::Factory("StackMemoryCommitedPage", SCI_StackMemoryCommitedPage, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// Stack frame
void StackFrame::marryFrame(VMContext *vmContext)
{
    assert(!hasContext());

    auto method = getMethod();

    // Instantiate the context.
    auto slotCount = method->getHeader()->needsLargeFrame() ? Context::LargeContextSlots : Context::SmallContextSlots;
    auto context = Context::create(vmContext, slotCount);
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
StackMemory::StackMemory(VMContext *context)
    : context(context)
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

// Interface for accessing the stack memory for the current native thread.
static thread_local StackMemory *currentStackMemory = nullptr;

void withStackMemory(VMContext *context, const StackMemoryEntry &entryPoint)
{
	if(currentStackMemory && currentStackMemory->getContext() == context)
		return entryPoint(currentStackMemory);

	// Ensure the current stack memory pointer is clear when I finish.
	struct EnsureBlock
	{
        EnsureBlock(VMContext *context)
            : context(context)
        {
            oldStackMemory = currentStackMemory;
            oldContext = getCurrentContext();
            setCurrentContext(context);
        }

		~EnsureBlock()
		{
			context->getMemoryManager()->getStackMemories()->unregisterMemory(currentStackMemory);
			delete currentStackMemory;
			currentStackMemory = oldStackMemory;
            setCurrentContext(oldContext);
		}

        VMContext *context;
        VMContext *oldContext;
        StackMemory *oldStackMemory;

	} ensure(context);

	currentStackMemory = new StackMemory(context);
	currentStackMemory->setStorage(reinterpret_cast<uint8_t*> (alloca(StackMemoryPageSize)), StackMemoryPageSize);
    context->getMemoryManager()->getStackMemories()->registerMemory(currentStackMemory);
	return entryPoint(currentStackMemory);
}

} // End of namespace Lodtalk
