#ifndef LODTALK_MEMORY_MANAGER_HPP
#define LODTALK_MEMORY_MANAGER_HPP

#include <list>
#include <vector>
#include <utility>
#include <mutex>

#include "StackMemory.hpp"
#include "ObjectModel.hpp"
#include "Synchronization.hpp"

namespace Lodtalk
{
#ifdef OBJECT_MODEL_SPUR_64
static constexpr size_t DefaultMaxVMHeapSize = size_t(16)*1024*1024*1024; // 16 GB
#else
static constexpr size_t DefaultMaxVMHeapSize = size_t(512)*1024*1024; // 512 MB
#endif

/**
 * The class table
 */
class ClassTable
{
public:
    ~ClassTable();

    ClassDescription *getClassFromIndex(size_t index);

    void registerClass(Oop clazz);
    void addSpecialClass(ClassDescription *description, size_t index);

    static ClassTable *get();

private:
    ClassTable();

    void allocatePage();

    static ClassTable *uniqueInstance;

    friend class GarbageCollector;

    SharedMutex sharedMutex;
    std::vector<Oop*> pageTable;
    size_t size;
};

/**
 * The VM heap allocator
 */
class VMHeap
{
public:
    ~VMHeap();

    size_t getMaxCapacity();
    size_t getCapacity();
    size_t getSize();
    void setSize(size_t newSize);

    uint8_t *getAddressSpace();
    uint8_t *getAddressSpaceEnd();

    uint8_t *allocate(size_t size);

    static VMHeap *get();

    inline bool containsPointer(uint8_t *pointer)
    {
        return getAddressSpace() <= pointer && pointer < getAddressSpaceEnd();
    }

private:
    VMHeap();
    void initialize();

    std::mutex mutex;
    uint8_t *addressSpace;
    size_t maxCapacity;
    size_t capacity;
    size_t size;
    size_t pageSize;

    static VMHeap *uniqueInstance;
};

VMHeap *getVMHeap();

/**
 * The garbage collector.
 */
class GarbageCollector
{
public:
	enum Color
	{
		White = 0,
		Gray,
		Black,
	};

	GarbageCollector();
	~GarbageCollector();

    void initialize();

	uint8_t *allocateObjectMemory(size_t objectSize);

	void performCollection();

	void registerOopReference(OopRef *ref);
	void unregisterOopReference(OopRef *ref);

	void registerGCRoot(Oop *gcroot, size_t size);
	void unregisterGCRoot(Oop *gcroot);

    void registerNativeObject(Oop object);

    void enable();
    void disable();

private:

	template<typename FT>
	void onRootsDo(const FT &f)
	{
		// Traverse the root pointers
		for(auto &rootSize : rootPointers)
		{
			auto roots = rootSize.first;
			auto size = rootSize.second;
			for(size_t i = 0; i < size; ++i)
				f(roots[i]);
		}

        // Traverse the classTable
        auto classTable = ClassTable::get();
        for(auto &classTablePage : classTable->pageTable)
        {
            for(size_t i = 0; i < OopsPerPage; ++i)
                f(classTablePage[i]);
        }

		// Traverse the stacks
		for(auto stack : currentStacks)
		{
			stack->stackFramesDo([&](StackFrame &stackFrame) {
				stackFrame.oopElementsDo(f);
			});
		}

		// Traverse the oop reference
		OopRef *pos = firstReference;
		for(; pos; pos = pos->nextReference_)
		{
			f(pos->oop);
		}
	}

	void mark();
	void markObject(Oop objectPointer);
    void updatePointer(Oop *pointer);
    void updatePointersOf(Oop object);
	void compact();
    void abortCompaction();


	std::mutex controlMutex;
	std::vector<std::pair<Oop*, size_t>> rootPointers;
    std::vector<Oop> nativeObjects;
	std::vector<StackMemory*> currentStacks;
	OopRef *firstReference;
	OopRef *lastReference;
    int disableCount;
};

GarbageCollector *getGC();
void registerRuntimeGCRoots();

} // End of namespace Lodtalk

#endif //LODTALK_MEMORY_MANAGER_HPP
