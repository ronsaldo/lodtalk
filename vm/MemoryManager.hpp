#ifndef LODTALK_MEMORY_MANAGER_HPP
#define LODTALK_MEMORY_MANAGER_HPP

#include <list>
#include <vector>
#include <utility>
#include <mutex>
#include <unordered_map>

#include "Lodtalk/ObjectModel.hpp"
#include "Constants.hpp"
#include "StackMemory.hpp"
#include "Lodtalk/Synchronization.hpp"

namespace Lodtalk
{
#ifdef OBJECT_MODEL_SPUR_64
static constexpr size_t DefaultMaxVMHeapSize = size_t(16)*1024*1024*1024; // 16 GB
#else
static constexpr size_t DefaultMaxVMHeapSize = size_t(512)*1024*1024; // 512 MB
#endif

class VMHeap;
class ClassTable;
class GarbageCollector;
class StackMemories;

class MemoryManager
{
public:
    typedef std::unordered_map<std::string, Oop> SymbolDictionary;

    MemoryManager(VMContext *context);
    ~MemoryManager();

    VMContext *getContext();
    VMHeap *getHeap();
    ClassTable *getClassTable();
    GarbageCollector *getGarbageCollector();
    StackMemories *getStackMemories();
    SymbolDictionary &getSymbolDictionary();

private:
    VMContext *context;
    VMHeap *heap;
    ClassTable *classTable;
    GarbageCollector *garbageCollector;
    StackMemories *stackMemories;
    SymbolDictionary symbolDictionary;
};


/**
 * The class table
 */
class ClassTable
{
public:
    ClassTable();
    ~ClassTable();

    ClassDescription *getClassFromIndex(size_t index);

    unsigned int registerClass(Oop clazz);
    void addSpecialClass(ClassDescription *description, size_t index);
    void setClassAtIndex(ClassDescription *description, size_t index);

private:
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
    VMHeap();
    ~VMHeap();

    void initialize();

    size_t getMaxCapacity();
    size_t getCapacity();
    size_t getSize();
    void setSize(size_t newSize);

    uint8_t *getAddressSpace();
    uint8_t *getAddressSpaceEnd();

    uint8_t *allocate(size_t size);

    inline bool containsPointer(uint8_t *pointer)
    {
        return getAddressSpace() <= pointer && pointer < getAddressSpaceEnd();
    }

    inline bool hasCapacityFor(size_t newSize)
    {
        return size + newSize <= capacity;
    }

private:

    uint8_t *addressSpace;
    size_t maxCapacity;
    size_t capacity;
    size_t size;
    size_t pageSize;
};

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

	GarbageCollector(MemoryManager *memoryManager);
	~GarbageCollector();

    void initialize();

	uint8_t *allocateObjectMemory(size_t objectSize, bool bigObject);

	void performCollection();

	void registerOopReference(OopRef *ref);
	void unregisterOopReference(OopRef *ref);

	void registerGCRoot(Oop *gcroot, size_t size);
	void unregisterGCRoot(Oop *gcroot);

    void registerNativeObject(Oop object);

    void enable();
    void disable();

private:
    void internalPerformCollection();

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
        auto classTable = memoryManager->getClassTable();
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

        // Traverse the symbol dictionary.
        {
            auto &symbolDict = memoryManager->getSymbolDictionary();
            auto it = symbolDict.begin();
            for(; it != symbolDict.end(); ++it)
                f(it->second);
        }
	}

	void mark();
	void markObject(Oop objectPointer);
    void updatePointer(Oop *pointer);
    void updatePointersOf(Oop object);
	void compact();
    void abortCompaction();

    MemoryManager *memoryManager;
	std::mutex controlMutex;
	std::vector<std::pair<Oop*, size_t>> rootPointers;
    std::vector<Oop> nativeObjects;
	std::vector<StackMemory*> currentStacks;
	OopRef *firstReference;
	OopRef *lastReference;
    int disableCount;
};

} // End of namespace Lodtalk

#endif //LODTALK_MEMORY_MANAGER_HPP
