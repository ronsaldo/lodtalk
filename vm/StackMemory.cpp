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
    // First pass. Ensure the frames are married and that they have their context updated.
    auto currentFrame = headFrame;
    while (currentFrame.framePointer != 0)
    {
        currentFrame.ensureHasUpdatedMarriedSpouse(context);
        currentFrame = currentFrame.getPreviousFrame();
    }

    // Get the head frame pc from the next page
    assert(nextPage && nextPage->isInUse());
    {
        StackFrame nextBaseFrame(nextPage->baseFramePointer);
        auto headContext = reinterpret_cast<Context*> (headFrame.getThisContext().pointer);
        headContext->pc = Oop::encodeSmallInteger(nextBaseFrame.getReturnPointer());
    }

    // Second pass. Divorce
    auto nextFrame = headFrame;
    currentFrame = headFrame.getPreviousFrame();
    while (currentFrame.framePointer != 0)
    {
        auto currentContext = reinterpret_cast<Context*> (currentFrame.getThisContext().pointer);
        auto nextContext = reinterpret_cast<Context*> (nextFrame.getThisContext().pointer);
        nextContext->sender = Oop::fromPointer(currentContext);
        currentContext->pc = Oop::encodeSmallInteger(nextFrame.getReturnPointer());

        nextFrame = currentFrame;
        currentFrame = currentFrame.getPreviousFrame();
    }

    freed();
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
    context->argumentCount = Oop::encodeSmallInteger(getArgumentCount());

    // Copy the arguments
    auto argumentCount = getArgumentCount();
    auto argumentEnd = &getArgumentAtReverseIndex(0);
    for (size_t i = 0; i < argumentCount; ++i)
        context->data[i] = argumentEnd[argumentCount - i - 1];
    
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

    // Copy the temporaries back to the context.
    auto currentTemporary = reinterpret_cast<Oop*> (framePointer + InterpreterStackFrame::FirstTempOffset);
    auto temporaryEnd = reinterpret_cast<Oop*> (stackPointer);
    auto currentTempIndex = getArgumentCount();
    for (;  currentTemporary >= temporaryEnd; --currentTemporary, currentTempIndex++)
        context->data[currentTempIndex] = *currentTemporary;
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
    currentPage->baseFramePointer = getFramePointer();

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
        // Find the least used page
        auto leastUsed = currentPage;
        for (; leastUsed->previousPage; leastUsed = leastUsed->previousPage)
            ;

        leastUsed->divorceAllFrames();
        freePages.push_back(leastUsed);
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
    currentPage->baseFramePointer = stackFrame.framePointer;

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

void StackMemory::makeBaseFrame(Context *context)
{
    auto basePage = allocatePage();
    assert(!currentPage->previousPage);
    basePage->startUsing();
    basePage->nextPage = currentPage;
    currentPage->previousPage = basePage;


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
