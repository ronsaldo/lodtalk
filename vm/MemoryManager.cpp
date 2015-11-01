#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

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

ClassTable *ClassTable::uniqueInstance = nullptr;
ClassTable *ClassTable::get()
{
    if(!uniqueInstance)
        uniqueInstance = new ClassTable();
    return uniqueInstance;
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

void ClassTable::registerClass(Oop clazz)
{
    WriteLock<SharedMutex> l(sharedMutex);
    auto pageIndex = size / OopsPerPage;
    auto elementIndex = size % OopsPerPage;
    if(elementIndex == 0 && pageIndex == pageTable.size())
        allocatePage();

    pageTable[pageIndex][elementIndex] = clazz;
    clazz.header->identityHash = (unsigned int)size;
    ++size;
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
}

void ClassTable::allocatePage()
{
    pageTable.push_back(new Oop[OopsPerPage]);
}

// Extra forwarding pointer used for compaction
struct AllocatedObject
{
    uint8_t *forwardingPointer;
    ObjectHeader header;
    uint64_t extraSlotCount;

    size_t computeSize()
    {
        auto size = sizeof(void*) + sizeof(ObjectHeader);
        if(header.slotCount == 255)
            size += 8 + extraSlotCount*sizeof(void*);
        else
            size += header.slotCount*sizeof(void*);

        return size;
    }
};

#ifdef _WIN32
inline uint8_t *reserveVirtualAddressSpace(size_t size)
{
    return VirtualAlloc(nullptr, size, MEM_RESERVE, 0);
}

inline bool allocateVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    LODTALK_UNIMPLEMENTED();
}

inline bool freeVirtualAddressRegion(uint8_t *addressSpace, size_t offset, size_t size)
{
    LODTALK_UNIMPLEMENTED();
}

inline size_t getPageSize()
{
    LODTALK_UNIMPLEMENTED();
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
    std::unique_lock<std::mutex> (mutex);
    size_t newHeapSize = size + objectSize;
    auto result = addressSpace + size;

    // Check the current capacity
    if(newHeapSize < capacity)
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

VMHeap *VMHeap::uniqueInstance = nullptr;

VMHeap *VMHeap::get()
{
    if(!uniqueInstance)
    {
        uniqueInstance = new VMHeap();
        uniqueInstance->initialize();
    }

    return uniqueInstance;
}

// The garbage collector
static GarbageCollector *theGarbageCollector = nullptr;

GarbageCollector *getGC()
{
	if(!theGarbageCollector)
    {
		theGarbageCollector = new GarbageCollector();
        theGarbageCollector->initialize();
    }
	return theGarbageCollector;
}

GarbageCollector::GarbageCollector()
	: firstReference(nullptr), lastReference(nullptr), disableCount(0)
{
}

GarbageCollector::~GarbageCollector()
{
}

void GarbageCollector::initialize()
{
    registerRuntimeGCRoots();
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

uint8_t *GarbageCollector::allocateObjectMemory(size_t objectSize)
{
	performCollection();
	assert(objectSize >= sizeof(ObjectHeader));

    // Add a forwarding slot, used by compaction.
    objectSize += sizeof(void*);

	// Allocate from the VM heap.
    auto result = VMHeap::get()->allocate(objectSize);
    result += sizeof(void*);

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

void GarbageCollector::registerNativeObject(Oop object)
{
    std::unique_lock<std::mutex> l(controlMutex);
    nativeObjects.push_back(object);
}

void GarbageCollector::performCollection()
{
	std::unique_lock<std::mutex> l(controlMutex);
    if(disableCount > 0)
        return;

	// Get the current stacks
	currentStacks = getAllStackMemories();

	// TODO: Suspend the other GC threads.
	mark();
	compact();
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
		{
			auto bigHeader = reinterpret_cast<BigObjectHeader*> (header);
			slotCount = bigHeader->slotCount;
			headerSize += 8;
		}

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
    auto heap = VMHeap::get();
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
        if(liveHeader->header.gcColor != White)
        {
            liveHeader->forwardingPointer = freeAddress + sizeof(void*);
            freeAddress += liveSize;
        }
        else
        {
            //printf("Free %s\n", getClassNameOfObject(Oop::fromPointer(&liveHeader->header)).c_str());
            liveHeader->forwardingPointer = nullptr;
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
        if(liveHeader->header.gcColor != White)
            updatePointersOf(Oop::fromPointer(&liveHeader->header));

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
        if(liveHeader->header.gcColor != White)
        {
            // Clear the color of the object for the next garbage collection.
            liveHeader->header.gcColor = White;

            // Move the object.
            auto destination = liveHeader->forwardingPointer - sizeof(void*);
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
    auto heap = VMHeap::get();
    auto lowestAddress = heap->getAddressSpace();
    auto endAddress = lowestAddress + heap->getSize();

    auto live = lowestAddress;
    while(live < endAddress)
    {
        auto liveHeader = reinterpret_cast<AllocatedObject*> (live);
        auto liveSize = liveHeader->computeSize();

        // Set the color to white.
        liveHeader->header.gcColor = White;

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
    auto heap = VMHeap::get();
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
		{
			auto bigHeader = reinterpret_cast<BigObjectHeader*> (header);
			slotCount = bigHeader->slotCount;
			headerSize += 8;
		}

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
	getGC()->registerOopReference(this);
}

void OopRef::unregisterSelf()
{
	getGC()->unregisterOopReference(this);
}

// GC collector public interface
void disableGC()
{
    getGC()->disable();
}

void enableGC()
{
    getGC()->enable();
}

void registerGCRoot(Oop *gcroot, size_t size)
{
	getGC()->registerGCRoot(gcroot, size);
}

void unregisterGCRoot(Oop *gcroot)
{
	getGC()->unregisterGCRoot(gcroot);
}

void registerNativeObject(Oop object)
{
    getGC()->registerNativeObject(object);
}

uint8_t *allocateObjectMemory(size_t objectSize)
{
	return getGC()->allocateObjectMemory(objectSize);
}
}
