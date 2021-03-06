#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef min
#undef max
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "Lodtalk/VMContext.hpp"

#include <algorithm>
#include <string.h>
#include "Method.hpp"
#include "MemoryManager.hpp"

namespace Lodtalk
{
// The class table
ClassTable::ClassTable()
    : size(0)
{
}

ClassTable::~ClassTable()
{
    for(auto &page : pageTable)
        delete [] page;
}

ClassDescription *ClassTable::getClassFromIndex(size_t index)
{
    ReadLock<SharedMutex> l(sharedMutex);
    if(index >= size)
        return reinterpret_cast<ClassDescription*> (&NilObject);

    auto pageIndex = index / OopsPerPage;
    auto elementIndex = index % OopsPerPage;
    return reinterpret_cast<ClassDescription*> (pageTable[pageIndex][elementIndex].pointer);
}

unsigned int ClassTable::registerClass(Oop clazz)
{
    WriteLock<SharedMutex> l(sharedMutex);
    auto pageIndex = size / OopsPerPage;
    auto elementIndex = size % OopsPerPage;
    if(elementIndex == 0 && pageIndex == pageTable.size())
        allocatePage();

    pageTable[pageIndex][elementIndex] = clazz;
    clazz.header->identityHash = (unsigned int)size;
    return (unsigned int) (size++);
}

void ClassTable::addSpecialClass(ClassDescription *description, size_t index)
{
    WriteLock<SharedMutex> l(sharedMutex);
    auto pageIndex = index / OopsPerPage;
    auto elementIndex = index % OopsPerPage;

    for(size_t i = pageTable.size(); i < pageIndex + 1; ++i)
        allocatePage();

    pageTable[pageIndex][elementIndex] = Oop::fromPointer(description);
    size = std::max(size, index + 1);
    description->object_header_.identityHash = (unsigned int)index;
}

void ClassTable::setClassAtIndex(ClassDescription *description, size_t index)
{
    WriteLock<SharedMutex> l(sharedMutex);
    assert(index < size);
    auto pageIndex = index / OopsPerPage;
    auto elementIndex = index % OopsPerPage;
    pageTable[pageIndex][elementIndex] = Oop::fromPointer(description);
}

void ClassTable::allocatePage()
{
    pageTable.push_back(new Oop[OopsPerPage]);
}

// Extra forwarding pointer used for compaction
struct AllocatedObject
{
    uint64_t rawForwardingPointer;

    union
    {
        struct
        {
            uint64_t extraSlotCount;
            ObjectHeader header;
        } bigObject;

        struct
        {
            ObjectHeader header;
        } smallObject;
    };

    size_t computeSize() const
    {
        auto size = 8 + sizeof(ObjectHeader);
        if(isBigObject())
            size += 8 + bigObject.extraSlotCount*sizeof(void*);
        else
            size += smallObject.header.slotCount*sizeof(void*);

        return size;
    }

    bool isBigObject() const
    {
        return (rawForwardingPointer & 1) != 0;
    }

    void *getForwardingPointer() const
    {
        return reinterpret_cast<void*> (uintptr_t(rawForwardingPointer & (-ObjectAlignment)));
    }

    void *getForwardingDestination() const
    {
        auto offset = isBigObject() ? 16 : 8;
        return reinterpret_cast<uint8_t*> (getForwardingPointer()) - offset;
    }

    void setForwardingPointer(void *newPointer)
    {
        auto newPointerValue = reinterpret_cast<uintptr_t> (newPointer) & (-ObjectAlignment);
        newPointerValue |= rawForwardingPointer & (ObjectAlignment - 1);
        rawForwardingPointer = newPointerValue;
    }

    ObjectHeader &header()
    {
        return isBigObject() ? bigObject.header : smallObject.header;
    }

    uint64_t slotCount()
    {
        return isBigObject() ? bigObject.extraSlotCount : smallObject.header.slotCount;
    }
};

#ifdef _WIN32
inline uint8_t *reserveVirtualAddressSpace(size_t size)
{
    return (uint8_t*)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
}

inline bool allocateVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    auto result = VirtualAlloc(addressSpace + offset, size, MEM_COMMIT, PAGE_READWRITE);
    return result == addressSpace + offset;
}

inline bool freeVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    return VirtualFree(addressSpace + offset, size, MEM_DECOMMIT) == TRUE;
}

inline size_t getPageSize()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

#else
inline uint8_t *reserveVirtualAddressSpace(size_t size)
{
    auto result = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return result != MAP_FAILED ? reinterpret_cast<uint8_t*> (result) : nullptr;
}

inline bool allocateVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    return mprotect(addressSpace + offset, size, PROT_READ | PROT_WRITE) == 0;
}

inline bool freeVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    return mprotect(addressSpace + offset, size, PROT_NONE) == 0;
}

inline size_t getPageSize()
{
    auto pageSize = sysconf(_SC_PAGE_SIZE);
    if(pageSize == -1)
    {
        fprintf(stderr, "Failed to the system page size\n");
        abort();
    }

    return pageSize;
}
#endif


VMHeap::VMHeap()
{
}

VMHeap::~VMHeap()
{
}

void VMHeap::initialize()
{
    // Get the page size.
    pageSize = getPageSize();

    // Allocate the virtual address space.
    maxCapacity = DefaultMaxVMHeapSize;
    capacity = 0;
    size = 0;
    addressSpace = reserveVirtualAddressSpace(maxCapacity);
    if(!addressSpace)
    {
        fprintf(stderr, "Failed to reserve the VM heap memory address space.\n");
        abort();
    }
}

size_t VMHeap::getMaxCapacity()
{
    return maxCapacity;
}

size_t VMHeap::getCapacity()
{
    return capacity;
}

size_t VMHeap::getSize()
{
    return size;
}

void VMHeap::setSize(size_t newSize)
{
    size = newSize;
}

uint8_t *VMHeap::getAddressSpace()
{
    return addressSpace;
}

uint8_t *VMHeap::getAddressSpaceEnd()
{
    return addressSpace + maxCapacity;
}

uint8_t *VMHeap::allocate(size_t objectSize)
{
    size_t newHeapSize = size + objectSize;
    auto result = addressSpace + size;

    // Check the current capacity
    if(newHeapSize <= capacity)
    {
        size = newHeapSize;
        return result;
    }

    // Compute the new capacity
    auto newCapacity = (newHeapSize + pageSize - 1) & (~ (pageSize - 1));
    if(!allocateVirtualAddressRegion(addressSpace, capacity, newCapacity - capacity))
        return nullptr;

    // Store the new capacity and sizes
    //printf("vm heap %zu %zu\n", newHeapSize, newCapacity);
    size = newHeapSize;
    capacity = newCapacity;
    return result;
}

GarbageCollector::GarbageCollector(MemoryManager *memoryManager)
	: memoryManager(memoryManager), firstReference(nullptr), lastReference(nullptr), disableCount(0)
{
    garbageCollectionQueued = false;
}

GarbageCollector::~GarbageCollector()
{
}

void GarbageCollector::initialize()
{
}

void GarbageCollector::enable()
{
    std::unique_lock<std::mutex> l(controlMutex);
    --disableCount;
}

void GarbageCollector::disable()
{
    std::unique_lock<std::mutex> l(controlMutex);
    ++disableCount;
}

uint8_t *GarbageCollector::allocateObjectMemory(size_t objectSize, bool bigObject)
{
    std::unique_lock<std::mutex> l(controlMutex);

    auto heap = memoryManager->getHeap();
	assert(objectSize >= sizeof(ObjectHeader));
    // Should I enqueue a garbage collection?
    if (!heap->hasCapacityThresholdBeenReached())
        queueGarbageCollection();

    // Add a forwarding slot, used by compaction.
    auto extraHeaderSize = 8;
    if(bigObject)
        extraHeaderSize += 8;

	// Allocate from the VM heap.
    auto result = memoryManager->getHeap()->allocate(objectSize + extraHeaderSize);
    auto allocatedObjectHeader = reinterpret_cast<AllocatedObject*> (result);
    allocatedObjectHeader->rawForwardingPointer = bigObject ? 1 : 0;
    result += extraHeaderSize;

	auto header = reinterpret_cast<ObjectHeader*> (result);
	*header = {0};

	return result;
}

void GarbageCollector::registerOopReference(OopRef *ref)
{
	std::unique_lock<std::mutex> l(controlMutex);
	assert(ref);

	// Insert the reference into the beginning doubly linked list.
	ref->prevReference_ = nullptr;
	ref->nextReference_ = firstReference;
	if(firstReference)
		firstReference->prevReference_ = ref;
	firstReference = ref;

	// Make sure the last reference is set.
	if(!lastReference)
		lastReference = firstReference;
}

void GarbageCollector::unregisterOopReference(OopRef *ref)
{
	std::unique_lock<std::mutex> l(controlMutex);
	assert(ref);

	// Remove the reference from the double linked list.
	if(ref->prevReference_)
		ref->prevReference_->nextReference_ = ref->nextReference_;
	if(ref->nextReference_)
		ref->nextReference_->prevReference_ = ref->prevReference_;

	// Check the beginning.
	if(!ref->prevReference_)
		firstReference = ref->nextReference_;

	// Check the tail.
	if(!ref->nextReference_)
		lastReference = ref->prevReference_;
}

void GarbageCollector::registerGCRoot(Oop *gcroot, size_t size)
{
	std::unique_lock<std::mutex> l(controlMutex);
	rootPointers.push_back(std::make_pair(gcroot, size));
}

void GarbageCollector::unregisterGCRoot(Oop *gcroot)
{
	std::unique_lock<std::mutex> l(controlMutex);
	for(size_t i = 0; i < rootPointers.size(); ++i)
	{
		if(rootPointers[i].first == gcroot)
			rootPointers.erase(rootPointers.begin() + i);
	}
}

void GarbageCollector::registerThreadForGC()
{
}

void GarbageCollector::unregisterThreadForGC()
{
}

bool GarbageCollector::collectionSafePoint()
{
    if(!garbageCollectionQueued || disableCount > 0)
        return false;

    //printf("GC time\n");
    internalPerformCollection();
    garbageCollectionQueued = false;
    return true;
}

void GarbageCollector::registerNativeObject(Oop object)
{
    std::unique_lock<std::mutex> l(controlMutex);
    nativeObjects.push_back(object);
}

void GarbageCollector::performCollection()
{
    std::unique_lock<std::mutex> l(controlMutex);
    internalPerformCollection();
}

void GarbageCollector::internalPerformCollection()
{
    if(disableCount > 0)
        return;

	// Get the current stacks
	currentStacks = memoryManager->getStackMemories()->getAll();

	// TODO: Suspend the other GC threads.
	mark();
	compact();
}

void GarbageCollector::queueGarbageCollection()
{
    garbageCollectionQueued = true;
}

void GarbageCollector::mark()
{
	// Mark from the root objects.
	onRootsDo([this](Oop root) {
		markObject(root);
	});
}

void GarbageCollector::markObject(Oop objectPointer)
{
	// TODO: Use the schorr-waite algorithm.

	// mark pointer objects.
	if(!objectPointer.isPointer())
		return;

	// Get the object header.
	auto header = objectPointer.header;
	if(header->gcColor)
		return;

	// Mark gray
	header->gcColor = Gray;

	// Mark recursively the children
	auto format = header->objectFormat;
	if(format == OF_FIXED_SIZE ||
	   format == OF_VARIABLE_SIZE_NO_IVARS ||
	   format == OF_VARIABLE_SIZE_IVARS)
	{
		auto slotCount = header->slotCount;
		auto headerSize = sizeof(ObjectHeader);
		if(slotCount == 255)
            slotCount = reinterpret_cast<uint64_t*> (header)[-1];

		// Traverse the slots.
		auto slots = reinterpret_cast<Oop*> (objectPointer.pointer + headerSize);
		for(size_t i = 0; i < slotCount; ++i)
			markObject(slots[i]);
	}

	// Special handilng of compiled method literals
	if(format >= OF_COMPILED_METHOD)
	{
		auto compiledMethod = reinterpret_cast<CompiledMethod*> (objectPointer.pointer);
		auto literalCount = compiledMethod->getLiteralCount();
		auto literals = compiledMethod->getFirstLiteralPointer();
		for(size_t i = 0; i < literalCount; ++i)
			markObject(literals[i]);
	}

	// Mark as black before ending.
	header->gcColor = Black;
}

void GarbageCollector::compact()
{
    auto heap = memoryManager->getHeap();
    auto lowestAddress = heap->getAddressSpace();
    auto endAddress = lowestAddress + heap->getSize();

    //--------------------------------------------------------------------------
    // First Pass
    // Compute the forwarding locations.
    auto freeAddress = lowestAddress;
    auto live = lowestAddress;
    uint64_t freeCount = 0;
    while(live < endAddress)
    {
        auto liveHeader = reinterpret_cast<AllocatedObject*> (live);
        auto liveSize = liveHeader->computeSize();

        // Is this object not condemned?
        if(liveHeader->header().gcColor != White)
        {
            liveHeader->setForwardingPointer(freeAddress + sizeof(void*));
            freeAddress += liveSize;
        }
        else
        {
            //printf("Free %s\n", memoryManager->getContext()->getClassNameOfObject(Oop::fromPointer(&liveHeader->header)).c_str());
            liveHeader->setForwardingPointer(nullptr);
            ++freeCount;
        }

        // Process the next object.
        live += liveSize;
    }

    assert(freeAddress <= live);
    assert(live == endAddress);

    // Are we freeing objects.
    if(!freeCount)
    {
        // Abort the compaction.
        abortCompaction();
        return;
    }

    //--------------------------------------------------------------------------
    // Second Pass
    // Update the object pointers.
    live = lowestAddress;
    while(live < endAddress)
    {
        auto liveHeader = reinterpret_cast<AllocatedObject*> (live);
        auto liveSize = liveHeader->computeSize();

        // Is this object not condemned?
        if(liveHeader->header().gcColor != White)
            updatePointersOf(Oop::fromPointer(&liveHeader->header()));

        // Process the next object.
        live += liveSize;
    }
    assert(live == endAddress);

    // Update the root pointers.
    onRootsDo([&](Oop &pointer) {
        updatePointer(&pointer);
    });

    // Some elements were not allocated by myself. Update their pointers.
    for(auto &nativeObject : nativeObjects)
        updatePointersOf(nativeObject);

    //--------------------------------------------------------------------------
    // Third Pass
    // Move the objects
    live = lowestAddress;
    while(live < endAddress)
    {
        auto liveHeader = reinterpret_cast<AllocatedObject*> (live);
        auto liveSize = liveHeader->computeSize();

        // Is this object not condemned?
        if(liveHeader->header().gcColor != White)
        {
            // Clear the color of the object for the next garbage collection.
            liveHeader->header().gcColor = White;

            // Move the object.
            auto destination = liveHeader->getForwardingDestination();
            //printf("Move %p %p\n", destination, liveHeader);
            memmove(destination, liveHeader, liveSize);

        }

        // Process the next object.
        live += liveSize;
    }
    assert(live == endAddress);

    // Set the white color of the native objects.
    for(auto &nativeObject : nativeObjects)
        nativeObject.header->gcColor = White;

    // Set the new heap size.
    heap->setSize(freeAddress - lowestAddress);
    //printf("Compaction ended\n");
}

void GarbageCollector::abortCompaction()
{
    auto heap = memoryManager->getHeap();
    auto lowestAddress = heap->getAddressSpace();
    auto endAddress = lowestAddress + heap->getSize();

    auto live = lowestAddress;
    while(live < endAddress)
    {
        auto liveHeader = reinterpret_cast<AllocatedObject*> (live);
        auto liveSize = liveHeader->computeSize();

        // Set the color to white.
        liveHeader->header().gcColor = White;

        // Process the next object.
        live += liveSize;
    }
    assert(live == endAddress);

    // Set the white color of the native objects.
    for(auto &nativeObject : nativeObjects)
        nativeObject.header->gcColor = White;
}

void GarbageCollector::updatePointer(Oop *pointer)
{
    // I only can update reference objects.
    if(!pointer->isPointer())
        return;

    // Is this a weak reference that was collected?
    if(pointer->header->gcColor == White)
    {
        *pointer = Oop();
        return;
    }

    // Is this object in the heap?
    auto heap = memoryManager->getHeap();
    if(!heap->containsPointer(pointer->pointer))
        return;

    // Use the forwarding pointer.
    uint8_t **forwardingPointer = reinterpret_cast<uint8_t**> (pointer->pointer - sizeof(uint8_t*));
    pointer->pointer = *forwardingPointer;
}

void GarbageCollector::updatePointersOf(Oop object)
{
    // Get the object header.
	auto header = object.header;

	// Mark recursively the children
	auto format = header->objectFormat;
	if(format == OF_FIXED_SIZE ||
	   format == OF_VARIABLE_SIZE_NO_IVARS ||
	   format == OF_VARIABLE_SIZE_IVARS ||
       format == OF_WEAK_VARIABLE_SIZE ||
       format == OF_WEAK_FIXED_SIZE )
	{
		auto slotCount = header->slotCount;
		auto headerSize = sizeof(ObjectHeader);
		if(slotCount == 255)
            slotCount = reinterpret_cast<uint64_t*> (header)[-1];

		// Traverse the slots.
		auto slots = reinterpret_cast<Oop*> (object.pointer + headerSize);
		for(size_t i = 0; i < slotCount; ++i)
			updatePointer(&slots[i]);

	}

	// Special handling of compiled method literals
	if(format >= OF_COMPILED_METHOD)
	{
		auto compiledMethod = reinterpret_cast<CompiledMethod*> (object.pointer);
		auto literalCount = compiledMethod->getLiteralCount();
		auto literals = compiledMethod->getFirstLiteralPointer();
		for(size_t i = 0; i < literalCount; ++i)
			updatePointer(&literals[i]);
	}
}

// OopRef
void OopRef::registerSelf()
{
	context_->getMemoryManager()->getGarbageCollector()->registerOopReference(this);
}

void OopRef::unregisterSelf()
{
	context_->getMemoryManager()->getGarbageCollector()->unregisterOopReference(this);
}

/**
 * Memory manager
 */
MemoryManager::MemoryManager(VMContext *context)
    : context(context)
{
    heap = new VMHeap();
    heap->initialize();

    classTable = new ClassTable();
    stackMemories = new StackMemories();
    garbageCollector = new GarbageCollector(this);
}

MemoryManager::~MemoryManager()
{
}

VMContext *MemoryManager::getContext()
{
    return context;
}

VMHeap *MemoryManager::getHeap()
{
    return heap;
}

ClassTable *MemoryManager::getClassTable()
{
    return classTable;
}

GarbageCollector *MemoryManager::getGarbageCollector()
{
    return garbageCollector;
}

StackMemories *MemoryManager::getStackMemories()
{
    return stackMemories;
}

MemoryManager::SymbolDictionary &MemoryManager::getSymbolDictionary()
{
    return symbolDictionary;
}

}
