#include "Lodtalk/VMContext.hpp"
#include "Compiler.hpp"
#include "StackInterpreter.hpp"
#include "SpecialRuntimeObjects.hpp"
#include "MemoryManager.hpp"
#include "StackMemory.hpp"
#include "ClassFactoryRegistry.hpp"

namespace Lodtalk
{
static thread_local VMContext *currentContext = nullptr;

VMContext::VMContext()
{
    initialize();
}

VMContext::~VMContext()
{
    ClassFactoryRegistry::get()->unregisterVMContext(this);
}

void VMContext::initialize()
{
    memoryManager = new MemoryManager(this);
    createGlobalDictionary();
    specialRuntimeObjects = new SpecialRuntimeObjects(this);
    specialRuntimeObjects->initialize();
    instanceClassFactories();
}

void VMContext::executeDoIt(const std::string &code)
{
    withInterpreter([&](InterpreterProxy *interpreter) {
        Lodtalk::executeDoIt(interpreter, code);
    });
}

void VMContext::executeScript(const std::string &code, const std::string &name, const std::string &basePath)
{
    withInterpreter([&](InterpreterProxy *interpreter) {
        Lodtalk::executeScript(interpreter, code, name, basePath);
    });
}

void VMContext::executeScriptFromFile(FILE *file, const std::string &name, const std::string &basePath)
{
    withInterpreter([&](InterpreterProxy *interpreter) {
        Lodtalk::executeScriptFromFile(interpreter, file, name, basePath);
    });
}

void VMContext::executeScriptFromFileNamed(const std::string &fileName)
{
    withInterpreter([&](InterpreterProxy *interpreter) {
        Lodtalk::executeScriptFromFileNamed(interpreter, fileName);
    });
}

// Memory manager
void VMContext::disableGC()
{
    memoryManager->getGarbageCollector()->disable();
}

void VMContext::enableGC()
{
    memoryManager->getGarbageCollector()->enable();
}

void VMContext::registerGCRoot(Oop *gcroot, size_t size)
{
	memoryManager->getGarbageCollector()->registerGCRoot(gcroot, size);
}

void VMContext::unregisterGCRoot(Oop *gcroot)
{
	memoryManager->getGarbageCollector()->unregisterGCRoot(gcroot);
}

void VMContext::registerNativeObject(Oop object)
{
    memoryManager->getGarbageCollector()->registerNativeObject(object);
}

uint8_t *VMContext::allocateObjectMemory(size_t objectSize)
{
	return memoryManager->getGarbageCollector()->allocateObjectMemory(objectSize);
}

MemoryManager *VMContext::getMemoryManager()
{
    return memoryManager;
}

SpecialRuntimeObjects *VMContext::getSpecialRuntimeObjects()
{
    return specialRuntimeObjects;
}

void VMContext::instanceClassFactories()
{
    WithoutGC wgc(this);
    ClassFactoryRegistry::get()->registerVMContext(this);
}

unsigned int VMContext::instanceClassFactory(AbstractClassFactory *factory)
{
    auto oldIt = instancedClassFactories.find(factory);
    if(oldIt != instancedClassFactories.end())
        return oldIt->second;

    // Compute or allocate the class indices.
    auto classTable = memoryManager->getClassTable();
    unsigned int classIndex;
    unsigned int metaclassIndex;
    bool isSpecialClass = factory->getSpecialClassIndex() >= 0;
    if(isSpecialClass)
    {
        classIndex = factory->getSpecialClassIndex();
        metaclassIndex = classIndex + 1;
    }
    else
    {
        metaclassIndex = classTable->registerClass(Oop::fromPointer(Metaclass::basicNativeNewMetaclass(this)));
        classIndex = classTable->registerClass(Oop::fromPointer(Class::basicNativeNewClass(this, metaclassIndex)));
    }
    instancedClassFactories[factory] = classIndex;

    // Instance the super class.
    auto superFactory = factory->getSuperClass();
    Oop superClass;
    Oop superClassMeta;
    if(superFactory)
    {
        auto superIndex = instanceClassFactory(superFactory);
        superClass = Oop::fromPointer(classTable->getClassFromIndex(superIndex));
        superClassMeta = getClassFromOop(superClass);
    }
    else
    {
        // This must be ProtoObject.
        superClass = nilOop();
        superClassMeta = getClassFromIndex(SCI_Class);
    }

    // Get pointers to the class and the meta class.
    Class *clazz = reinterpret_cast<Class*> (getClassFromIndex(classIndex).pointer);
    Metaclass *metaclass = reinterpret_cast<Metaclass*> (getClassFromIndex(metaclassIndex).pointer);
    metaclass->thisClass = Oop::fromPointer(clazz);

    // Build the class.
    //printf("Build class %s %d %d\n", factory->getName(), classIndex, metaclassIndex);
    ClassBuilder builder(this, clazz, metaclass);
    builder.setName(factory->getName());
    builder.setSuperclass(reinterpret_cast<Class*> (superClass.pointer), reinterpret_cast<Metaclass*> (superClassMeta.pointer));
    factory->build(builder);
    builder.finish();

    // Return the class index.
    return classIndex;
}

PrimitiveFunction VMContext::findPrimitive(int primitiveIndex)
{
    auto it = numberedPrimitives.find(primitiveIndex);
    if(it != numberedPrimitives.end())
        return it->second;

    return nullptr;
}

void VMContext::registerPrimitive(int primitiveIndex, PrimitiveFunction primitive)
{
    numberedPrimitives[primitiveIndex] = primitive;
}

void VMContext::registerNamedPrimitive(Oop name, Oop module, PrimitiveFunction primitive)
{
    // TODO: Implement this
}

LODTALK_VM_EXPORT VMContext *createVMContext()
{
    return new VMContext();
}

LODTALK_VM_EXPORT VMContext *getCurrentContext()
{
    return currentContext;
}

LODTALK_VM_EXPORT void setCurrentContext(VMContext *context)
{
    currentContext = context;
}

} // End of namespace Lodtalk
