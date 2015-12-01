#ifndef LODTALK_VMCONTEXT_HPP_
#define LODTALK_VMCONTEXT_HPP_

#include <string>
#include <stdio.h>
#include <functional>
#include <unordered_map>
#include "Lodtalk/Definitions.h"
#include "Lodtalk/ObjectModel.hpp"

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4251 )
#endif

namespace Lodtalk
{

class InterpreterProxy;
class MemoryManager;
class GarbageCollector;
class SpecialRuntimeObjects;
class AbstractClassFactory;
class SystemDictionary;

typedef int (*PrimitiveFunction) (InterpreterProxy *proxy);
typedef std::function<void (InterpreterProxy *)> WithInterpreterBlock;

/**
 * VM Context
 */
class LODTALK_VM_EXPORT VMContext
{
public:
    VMContext();
    ~VMContext();

    MemoryManager *getMemoryManager();
    SpecialRuntimeObjects *getSpecialRuntimeObjects();

    void executeDoIt(const std::string &code);
    void executeScript(const std::string &code, const std::string &name = "unnamed", const std::string &basePath = ".");
    void executeScriptFromFile(FILE *file, const std::string &name = "unnamed", const std::string &basePath = ".");
    void executeScriptFromFileNamed(const std::string &fileName);

    void withInterpreter(const WithInterpreterBlock &block);

    // Garbage collection interface
    void disableGC();
    void enableGC();

    void registerGCRoot(Oop *gcroot, size_t size);
    void unregisterGCRoot(Oop *gcroot);

    void registerNativeObject(Oop object);

    // Object memory
    uint8_t *allocateObjectMemory(size_t objectSize);
    ObjectHeader *newObject(size_t fixedSlotCount, size_t indexableSize, ObjectFormat format, int classIndex, int identityHash = -1);

    ObjectHeader *basicNativeNewFromClassIndex(size_t classIndex);
    ObjectHeader *basicNativeNewFromFactory(AbstractClassFactory *factory);

    // Some object creation / accessing
    int64_t readIntegerObject(const Ref<Oop> &ref);
    double readDoubleObject(const Ref<Oop> &ref);

    Oop positiveInt32ObjectFor(uint32_t value);
    Oop positiveInt64ObjectFor(uint64_t value);

    Oop signedInt32ObjectFor(int32_t value);
    Oop signedInt64ObjectFor(int64_t value);

    uint32_t positiveInt32ValueOf(Oop object);
    uint64_t positiveInt64ValueOf(Oop object);

    Oop floatObjectFor(double value);
    double floatValueOf(Oop object);

    // Class table
    Oop getClassFromIndex(int classIndex);
    Oop getClassFromOop(Oop oop);
    void registerClassInTable(Oop clazz);
    size_t getFixedSlotCountOfClass(Oop clazzOop);

    // Class testing
    bool isClassOrMetaclass(Oop oop);
    bool isMetaclass(Oop oop);
    bool isClass(Oop oop);

    // Global variables
    Oop setGlobalVariable(const char *name, Oop value);
    Oop setGlobalVariable(Oop name, Oop value);

    Oop getGlobalFromName(const char *name);
    Oop getGlobalFromSymbol(Oop symbol);

    Oop getGlobalValueFromName(const char *name);
    Oop getGlobalValueFromSymbol(Oop symbol);

    // Object creation
    Oop makeByteString(const std::string &content);
    Oop makeByteSymbol(const std::string &content);
    Oop makeSelector(const std::string &content);

    // Global context.
    Oop getGlobalContext();

    // Object reading
    std::string getClassNameOfObject(Oop object);
    std::string getByteSymbolData(Oop object);
    std::string getByteStringData(Oop object);

    // Other special objects.
    Oop getBlockActivationSelector(size_t argumentCount);
    Oop getSpecialMessageSelector(SpecialMessageSelector selectorIndex);
    Oop getCompilerOptimizedSelector(CompilerOptimizedSelector selectorIndex);
    CompilerOptimizedSelector getCompilerOptimizedSelectorId(Oop selector);

    unsigned int instanceClassFactory(AbstractClassFactory *factory);

    // Primitives
    PrimitiveFunction findPrimitive(int primitiveIndex);
    void registerPrimitive(int primitiveIndex, PrimitiveFunction primitive);
    void registerNamedPrimitive(Oop name, Oop module, PrimitiveFunction primitive);

private:
    void initialize();
    void createGlobalDictionary();
    void instanceClassFactories();

    MemoryManager *memoryManager;
    SpecialRuntimeObjects *specialRuntimeObjects;
    SystemDictionary *globalDictionary;

    std::unordered_map<AbstractClassFactory*, unsigned int> instancedClassFactories;
    std::unordered_map<int, PrimitiveFunction> numberedPrimitives;
};

LODTALK_VM_EXPORT VMContext *createVMContext();
LODTALK_VM_EXPORT VMContext *getCurrentContext();
LODTALK_VM_EXPORT void setCurrentContext(VMContext *context);

// Garbage collection interface
class LODTALK_VM_EXPORT WithoutGC
{
public:
    WithoutGC(VMContext *context)
        : context(context)
    {
        context->disableGC();
    }

    ~WithoutGC()
    {
        context->enableGC();
    }

private:
    VMContext *context;
};

#ifdef _MSC_VER
#  pragma warning( pop )
#endif

} // End of namespace VMContext
#endif //LODTALK_VMCONTEXT_HPP_
