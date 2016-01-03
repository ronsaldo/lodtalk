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

StackPage::StackPage(VMContext *context, uint8_t *memoryLocation)
    : context(context)
{
    stackPageSize = StackMemoryPageSize;

    stackPageLowest = memoryLocation;
    stackPageHighest = stackPageLowest + stackPageSize;
    baseFramePointer = nullptr;

    overflowLimit = stackPageLowest + Context::LargeContextSlots*sizeof(void*)*3/2;
    stackLimit = overflowLimit;

    headFrame = StackFrame(nullptr, nullptr);
    previousPage = nullptr;
    nextPage = nullptr;
}

StackPage::~StackPage()
{
}

void StackPage::divorceAllFrames()
{
    baseFramePointer = nullptr;
}

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
    if (isBlockActivation())
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
    setMetadata(getMetadata() | (1 << 16));

    // Ensure the have context flag is set.
    assert(hasContext());
}

void StackFrame::updateMarriedSpouseState()
{
    assert(hasContext());
    auto context = reinterpret_cast<Context*> (getThisContext().pointer);
    assert(!isNil(context));
    context->stackp = Oop::fromPointer(stackPointer + 1);
}

// Stack memory for a single thread.
StackMemory::StackMemory(VMContext *context)
    : context(context)
{
    stackSize = StackMemoryPageSize*StackPageCount;
    stackMemoryLowest = new uint8_t[stackSize];
    stackMemoryHighest = stackMemoryLowest + stackSize;

    for (size_t i = 0; i < StackPageCount; ++i)
    {
        auto page = new StackPage(context, stackMemoryLowest + i*StackMemoryPageSize);
        stackPages.push_back(page);
        freePages.push_back(page);
    }

    currentPage = allocatePage();
    currentPage->startUsing();
    stackFrame = currentPage->headFrame;
}

StackMemory::~StackMemory()
{
    delete[] stackMemoryLowest;

    for (auto page : stackPages)
        delete page;
}

void StackMemory::createTerminalStackFrame()
{
    // Null receiver argument
    pushOop(Oop());

    // Null return pc
    pushPointer(0);

    // Null previous stack frame.
    pushPointer(0);

    // Set the stack frame pointer.
    setFramePointer(getStackPointer());

    // Push the nil method object.
    pushOop(Oop());

    // Encode frame metadata
    pushUInt(encodeFrameMetaData(false, false, 0));

    // Push the nil this context.
    pushOop(Oop());

    // Push the nil receiver oop.
    pushOop(Oop());
}

StackPage *StackMemory::allocatePage()
{
    StackPage *page = nullptr;
    if (freePages.empty())
    {
        // TODO: free the least recently used page.
        LODTALK_UNIMPLEMENTED();
    }

    assert(!freePages.empty());
    page = freePages.back();
    freePages.pop_back();

    page->previousPage = nullptr;
    page->nextPage = nullptr;
    return page;
}

void StackMemory::writeBackFrameData()
{
    currentPage->headFrame = stackFrame;
}

void StackMemory::stackOverflow()
{
    // Store the previous frame
    auto previousFrame = stackFrame.getPreviousFrame();
    currentPage->headFrame = previousFrame;

    // Allocate and link the next page.
    auto nextPage = allocatePage();
    assert(nextPage != currentPage);
    nextPage->startUsing();
    currentPage->nextPage = nextPage;
    nextPage->previousPage = currentPage;

    // Get the source pointers and size
    auto srcBeginCopy = stackFrame.getBeginPointer();
    auto srcEndCopy = stackFrame.getEndPointer();
    auto copySize = srcEndCopy - srcBeginCopy;
    auto framePointerOffset = stackFrame.framePointer - srcBeginCopy;

    // Get the destination pointers
    auto dstEndCopy = nextPage->baseFramePointer;
    auto dstBeginCopy = dstEndCopy - copySize;

    // Copy the stack frame
    memcpy(dstBeginCopy, srcBeginCopy, copySize);

    // Use the new stack page
    auto newFramePointer = dstBeginCopy + framePointerOffset;
    stackFrame = StackFrame(newFramePointer, dstBeginCopy);
    stackFrame.setPrevFramePointer(nullptr);
    currentPage = nextPage;
    currentPage->headFrame = stackFrame;

    // Make sure they have contexts.
    previousFrame.ensureHasUpdatedMarriedSpouse(context);
    stackFrame.ensureHasUpdatedMarriedSpouse(context);

    // Link the new context with the previous context
    auto currentContext = reinterpret_cast<Context*> (stackFrame.getThisContext().pointer);
    currentContext->sender = previousFrame.getThisContext();
    assert(!currentContext->sender.isNil());
}

size_t StackMemory::getPageIndexFor(uint8_t *pointer)
{
    return (pointer - stackMemoryLowest) / StackMemoryPageSize;
}

void StackMemory::useNewPageFor(uint8_t *framePointer)
{
    auto pageIndex = getPageIndexFor(framePointer);
    auto newPage = stackPages[pageIndex];
    assert(newPage->isInUse());

    // Free the the old page.
    if (currentPage)
    {
        currentPage->freed();
        freePages.push_back(currentPage);
    }
    currentPage = newPage;
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
    currentStackMemory->createTerminalStackFrame();
    context->getMemoryManager()->getStackMemories()->registerMemory(currentStackMemory);
	return entryPoint(currentStackMemory);
}

} // End of namespace Lodtalk
