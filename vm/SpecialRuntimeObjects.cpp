#include "MemoryManager.hpp"
#include "SpecialRuntimeObjects.hpp"

namespace Lodtalk
{
SpecialRuntimeObjects::SpecialRuntimeObjects(VMContext *context)
    : context(context)
{
}

SpecialRuntimeObjects::~SpecialRuntimeObjects()
{
}

void SpecialRuntimeObjects::initialize()
{
    WithoutGC wgc(context);

    createSpecialObjectTable();
    createSpecialClassTable();
    registerGCRoots();
}

Oop SpecialRuntimeObjects::makeSelector(const char *string)
{
    return context->makeSelector(string);
}

void SpecialRuntimeObjects::createSpecialObjectTable()
{
    WithoutGC wgc(context);

	specialObjectTable.push_back(nilOop());
	specialObjectTable.push_back(trueOop());
	specialObjectTable.push_back(falseOop());

    // Block activation selectors
    blockActivationSelectorFirst = specialObjectTable.size();
    specialObjectTable.push_back(makeSelector("value"));
    specialObjectTable.push_back(makeSelector("value:"));
    specialObjectTable.push_back(makeSelector("value:value:"));
    specialObjectTable.push_back(makeSelector("value:value:value:"));
    specialObjectTable.push_back(makeSelector("value:value:value:value:"));
    specialObjectTable.push_back(makeSelector("value:value:value:value:value:"));
    specialObjectTable.push_back(makeSelector("value:value:value:value:value:value:"));
    blockActivationSelectorCount = specialObjectTable.size() - blockActivationSelectorFirst;

    // Special message selectors
    specialMessageSelectorFirst = specialObjectTable.size();

    // Arithmetic messages
    specialObjectTable.push_back(makeSelector("+"));
    specialObjectTable.push_back(makeSelector("-"));
    specialObjectTable.push_back(makeSelector("<"));
    specialObjectTable.push_back(makeSelector(">"));
    specialObjectTable.push_back(makeSelector("<="));
    specialObjectTable.push_back(makeSelector(">="));
    specialObjectTable.push_back(makeSelector("="));
    specialObjectTable.push_back(makeSelector("~="));
    specialObjectTable.push_back(makeSelector("*"));
    specialObjectTable.push_back(makeSelector("/"));
    specialObjectTable.push_back(makeSelector("\\\\"));
    specialObjectTable.push_back(makeSelector("@"));
    specialObjectTable.push_back(makeSelector("bitShift:"));
    specialObjectTable.push_back(makeSelector("//"));
    specialObjectTable.push_back(makeSelector("bitAnd:"));
    specialObjectTable.push_back(makeSelector("bitOr:"));

    // Object accessing messages.
    specialObjectTable.push_back(makeSelector("at:"));
    specialObjectTable.push_back(makeSelector("at:put:"));
    specialObjectTable.push_back(makeSelector("size"));
    specialObjectTable.push_back(makeSelector("next"));
    specialObjectTable.push_back(makeSelector("nextPut:"));
    specialObjectTable.push_back(makeSelector("atEnd"));
    specialObjectTable.push_back(makeSelector("=="));
    specialObjectTable.push_back(makeSelector("class"));

    specialObjectTable.push_back(Oop()); // Unassigned

    // Block evaluation.
    specialObjectTable.push_back(makeSelector("value"));
    specialObjectTable.push_back(makeSelector("value:"));
    specialObjectTable.push_back(makeSelector("do:"));

    // Object instantiation.
    specialObjectTable.push_back(makeSelector("new"));
    specialObjectTable.push_back(makeSelector("new:"));
    specialObjectTable.push_back(makeSelector("x"));
    specialObjectTable.push_back(makeSelector("y"));
    specialObjectTable.push_back(makeSelector("basicNew"));
    specialObjectTable.push_back(makeSelector("basicNew:"));

    // Does not understand
    specialObjectTable.push_back(makeSelector("doesNotUnderstand:"));

    specialMessageSelectorCount = specialObjectTable.size() - specialMessageSelectorFirst;
    assert(specialMessageSelectorCount == (size_t)SpecialMessageSelector::SpecialMessageCount);

    compilerMessageSelectorFirst = specialObjectTable.size();
    specialObjectTable.push_back(makeSelector("ifTrue:"));
    specialObjectTable.push_back(makeSelector("ifFalse:"));
    specialObjectTable.push_back(makeSelector("ifTrue:ifFalse:"));
    specialObjectTable.push_back(makeSelector("ifFalse:ifTrue:"));
    specialObjectTable.push_back(makeSelector("ifNil:"));
    specialObjectTable.push_back(makeSelector("ifNotNil:"));
    specialObjectTable.push_back(makeSelector("ifNil:ifNotNil:"));
    specialObjectTable.push_back(makeSelector("ifNotNil:ifNil:"));

    // Loops
    specialObjectTable.push_back(makeSelector("whileTrue:"));
    specialObjectTable.push_back(makeSelector("whileFalse:"));
    specialObjectTable.push_back(makeSelector("repeat"));
    specialObjectTable.push_back(makeSelector("doWhileTrue:"));
    specialObjectTable.push_back(makeSelector("doWhileFalse:"));

    // Iteration
    specialObjectTable.push_back(makeSelector("to:by:do:"));
    specialObjectTable.push_back(makeSelector("to:do:"));

    compilerMessageSelectorCount = specialObjectTable.size() - compilerMessageSelectorFirst;
    assert(compilerMessageSelectorCount == (size_t)CompilerOptimizedSelector::CompilerMessageCount);
}

void SpecialRuntimeObjects::createSpecialClassTable()
{
    specialClassTable.resize(SpecialClassTableSize);
    for(size_t i = 0; i < SpecialClassTableSize; i += 2)
    {
        specialClassTable[i] = Class::basicNativeNewClass(context, i + 1);
        specialClassTable[i + 1] = Metaclass::basicNativeNewMetaclass(context);
    }

    // Register the special classes in the class table.
    auto classTable = context->getMemoryManager()->getClassTable();
    for(size_t i = 0; i < specialClassTable.size(); ++i)
        classTable->addSpecialClass(specialClassTable[i], i);
}

void SpecialRuntimeObjects::registerGCRoots()
{
    auto gc = context->getMemoryManager()->getGarbageCollector();

    // Register the runtime GC  objects table.
	gc->registerGCRoot(&specialObjectTable[0], specialObjectTable.size());

	// Register the special classes table.
	gc->registerGCRoot((Oop*)&specialClassTable[0], specialClassTable.size());

    // nil, true, false
    gc->registerNativeObject(nilOop());
    gc->registerNativeObject(trueOop());
    gc->registerNativeObject(falseOop());
}

} // End of namespace Lodtalk
