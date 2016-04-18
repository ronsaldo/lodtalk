#include <list>
#include <string.h>
#include "Lodtalk/ObjectModel.hpp"
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/Object.hpp"
#include "Lodtalk/Collections.hpp"
#include "Method.hpp"

#include "Compiler.hpp"
#include "InputOutput.hpp"
#include "MemoryManager.hpp"
#include "StackMemory.hpp"
#include "SpecialRuntimeObjects.hpp"

namespace Lodtalk
{

ObjectHeader *VMContext::newObject(size_t fixedSlotCount, size_t indexableSize, ObjectFormat format, int classIndex, int identityHash)
{
	// Compute the header size.
	size_t indexableSlotCount = 0;
	auto indexableSlotSize = variableSlotSizeFor(format);
	auto indexableFormatExtraBits = 0;
	bool hasPrimitiveData = false;
	if(indexableSlotSize && indexableSize)
	{
		if(format < OF_INDEXABLE_NATIVE_FIRST)
		{
			indexableSlotCount = indexableSize;
		}
#ifndef OBJECT_MODEL_SPUR_64
		else if(format == OF_INDEXABLE_64)
		{
			indexableSlotCount = indexableSize * 2;
			hasPrimitiveData = true;
		}
#endif
		else
		{
			hasPrimitiveData = true;
			size_t divisor = variableSlotDivisor(format);
			size_t mask = (divisor - 1);

		 	indexableSlotCount = ((indexableSize + divisor - 1) & (~mask)) / divisor;
			indexableFormatExtraBits = int(indexableSize & mask);
			assert(indexableSize <= indexableSlotCount*divisor);
		}
	}

	// Compute more sizes
	auto totalSlotCount = fixedSlotCount + indexableSlotCount;
	auto headerSize = sizeof(ObjectHeader);
	if(totalSlotCount >= 255)
		headerSize += 8;
	auto bodySize = totalSlotCount * sizeof(void*);
	auto objectSize = headerSize + bodySize;

	// Allocate the object memory
	auto data = allocateObjectMemory(objectSize);
	auto header = reinterpret_cast<ObjectHeader*> (data);

	// Generate a hash if requested.
	if(identityHash < 0)
		identityHash = generateIdentityHash(data);

	// Set the object header.
	*header = {0};
	header->slotCount = totalSlotCount < 255 ? totalSlotCount : 255;
	header->identityHash = identityHash;
	header->objectFormat = format + indexableFormatExtraBits;
	header->classIndex = classIndex;
	if(totalSlotCount >= 255)
	{
		auto bigHeader = reinterpret_cast<BigObjectHeader*> (header);
		bigHeader->slotCount = totalSlotCount;
	}

	// Initialize the slots.
	auto slotStarts = data + headerSize;
	if(!hasPrimitiveData)
	{
		auto slots = reinterpret_cast<Object**> (slotStarts);
		for(size_t i = 0; i < totalSlotCount; ++i)
			slots[i] = &NilObject;
	}
	else
	{
		memset(slotStarts, 0, bodySize);
	}

	// Return the object.
	return header;
}

ObjectHeader *VMContext::basicNativeNewFromClassIndex(size_t classIndex)
{
    auto clazz = getClassFromIndex((int)classIndex);
    assert(isClassOrMetaclass(clazz));
    auto desc = reinterpret_cast<ClassDescription*> (clazz.pointer);
    return reinterpret_cast<ObjectHeader*> (desc->basicNativeNew(this));
}

ObjectHeader *VMContext::basicNativeNewFromFactory(AbstractClassFactory *factory)
{
    LODTALK_UNIMPLEMENTED();
}

// Class testing
bool VMContext::isClassOrMetaclass(Oop oop)
{
	auto classIndex = classIndexOf(oop);
	if(classIndex == SCI_Metaclass)
		return true;

	auto clazz = getClassFromIndex(classIndexOf(oop));
	return classIndexOf(clazz) == SCI_Metaclass;
}

bool VMContext::isMetaclass(Oop oop)
{
	return classIndexOf(oop) == SCI_Metaclass;
}

bool VMContext::isClass(Oop oop)
{
	return isClassOrMetaclass(oop) && !isMetaclass(oop);
}

// Object creation / accessing
Oop VMContext::positiveInt32ObjectFor(uint32_t value)
{
    if(unsignedFitsInSmallInteger(value))
        return Oop::encodeSmallInteger(value);
    LODTALK_UNIMPLEMENTED();
}

Oop VMContext::positiveInt64ObjectFor(uint64_t value)
{
    if(unsignedFitsInSmallInteger(value))
        return Oop::encodeSmallInteger(value);
    LODTALK_UNIMPLEMENTED();
}

Oop VMContext::signedInt32ObjectFor(int32_t value)
{
    if(signedFitsInSmallInteger(value))
        return Oop::encodeSmallInteger(value);
    LODTALK_UNIMPLEMENTED();
}

Oop VMContext::signedInt64ObjectFor(int64_t value)
{
    if(signedFitsInSmallInteger(value))
        return Oop::encodeSmallInteger(value);
    LODTALK_UNIMPLEMENTED();
}

uint32_t VMContext::positiveInt32ValueOf(Oop value)
{
    if(value.isSmallInteger())
        return (uint32_t)value.decodeSmallInteger();
    LODTALK_UNIMPLEMENTED();
}

uint64_t VMContext::positiveInt64ValueOf(Oop value)
{
    if(value.isSmallInteger())
        return value.decodeSmallInteger();
    LODTALK_UNIMPLEMENTED();
}

Oop VMContext::floatObjectFor(double value)
{
    LODTALK_UNIMPLEMENTED();
}

double VMContext::floatValueOf(Oop object)
{
    LODTALK_UNIMPLEMENTED();
}

Oop VMContext::makeByteString(const std::string &content)
{
	return ByteString::fromNative(this, content).getOop();
}

Oop VMContext::makeByteSymbol(const std::string &content)
{
	return ByteSymbol::fromNative(this, content);
}

Oop VMContext::makeSelector(const std::string &content)
{
	return makeByteSymbol(content);
}

// Get a class from its index
Oop VMContext::getClassFromIndex(int classIndex)
{
    auto clazz = memoryManager->getClassTable()->getClassFromIndex(classIndex);
	return Oop::fromPointer(clazz);
}

Oop VMContext::getClassFromOop(Oop oop)
{
	return getClassFromIndex(classIndexOf(oop));
}

void VMContext::registerClassInTable(Oop clazz)
{
    memoryManager->getClassTable()->registerClass(clazz);
}

size_t VMContext::getFixedSlotCountOfClass(Oop clazzOop)
{
    auto clazz = reinterpret_cast<ClassDescription*> (clazzOop.pointer);
    return clazz->fixedVariableCount.decodeSmallInteger();
}

size_t Oop::getFixedSlotCount(VMContext *context) const
{
    return context->getFixedSlotCountOfClass(context->getClassFromOop(*this));
}

// Global dictionary
void VMContext::createGlobalDictionary()
{
    WithoutGC wgc(this);

    // Create the global dictionary
    globalDictionary = SystemDictionary::create(this);
    registerGCRoot((Oop*)&globalDictionary, 1);

    // Create the smalltalk image object
    auto smalltalk = SmalltalkImage::create(this);
    setGlobalVariable(makeByteSymbol("Smalltalk"), Oop::fromPointer(smalltalk));
    smalltalk->globals = Oop::fromPointer(globalDictionary);
}

Oop VMContext::setGlobalVariable(const char *name, Oop value)
{
	return setGlobalVariable(ByteSymbol::fromNative(this, name), value);
}

Oop VMContext::setGlobalVariable(Oop symbol, Oop value)
{
	// Set the existing value.
	auto globalVar = globalDictionary->getNativeAssociationOrNil(symbol);
	if(classIndexOf(Oop::fromPointer(globalVar)) == SCI_GlobalVariable)
	{
		globalVar->value = value;
		return Oop::fromPointer(globalVar);
	}

	// Create the global variable
	Ref<Association> newGlobalVar(this, GlobalVariable::make(this, symbol, value));
	globalDictionary->putNativeAssociation(this, newGlobalVar.get());
	return newGlobalVar.getOop();
}

Oop VMContext::getGlobalFromName(const char *name)
{
	return getGlobalFromSymbol(makeByteSymbol(name));
}

Oop VMContext::getGlobalFromSymbol(Oop symbol)
{
	return Oop::fromPointer(globalDictionary->getNativeAssociationOrNil(symbol));
}

Oop VMContext::getGlobalValueFromName(const char *name)
{
	return getGlobalValueFromSymbol(ByteSymbol::fromNative(this, name));
}

Oop VMContext::getGlobalValueFromSymbol(Oop symbol)
{
	auto globalVar = globalDictionary->getNativeAssociationOrNil(symbol);
	if(classIndexOf(Oop::fromPointer(globalVar)) != SCI_GlobalVariable)
		return nilOop();
	return globalVar->value;
}

Oop VMContext::getGlobalContext()
{
	return getClassFromIndex(SCI_GlobalContext);
}

// Object reading
std::string VMContext::getClassNameOfObject(Oop object)
{
	auto classIndex = classIndexOf(object);
	auto classOop = getClassFromIndex(classIndex);
	if(classOop.header->classIndex == SCI_Metaclass)
    {
        auto meta = reinterpret_cast<Metaclass*> (classOop.pointer);
        if(!meta->thisClass.isNil())
        {
            auto clazz = reinterpret_cast<Class*> (meta->thisClass.pointer);
    		return clazz->getNameString() + " class";
        }

        return "a Metaclass";
    }

	auto clazz = reinterpret_cast<Class*> (classOop.pointer);
	return clazz->getNameString();
}

std::string VMContext::getByteSymbolData(Oop object)
{
	auto symbol = reinterpret_cast<ByteSymbol*> (object.pointer);
	return symbol->getString();
}

std::string VMContext::getByteStringData(Oop object)
{
	auto string = reinterpret_cast<ByteString*> (object.pointer);
	return string->getString();
}

// Special selectors.
Oop VMContext::getBlockActivationSelector(size_t argumentCount)
{
    auto specialObjects = getSpecialRuntimeObjects();
    if(argumentCount >= specialObjects->blockActivationSelectorCount)
        return Oop();
    return specialObjects->specialObjectTable[specialObjects->blockActivationSelectorFirst + argumentCount];
}

Oop VMContext::getSpecialMessageSelector(SpecialMessageSelector selectorIndex)
{
    auto specialObjects = getSpecialRuntimeObjects();
    return specialObjects->specialObjectTable[specialObjects->specialMessageSelectorFirst + (size_t)selectorIndex];
}

Oop VMContext::getCompilerOptimizedSelector(CompilerOptimizedSelector selectorIndex)
{
    auto specialObjects = getSpecialRuntimeObjects();
    return specialObjects->specialObjectTable[specialObjects->compilerMessageSelectorFirst + (size_t)selectorIndex];
}

CompilerOptimizedSelector VMContext::getCompilerOptimizedSelectorId(Oop selector)
{
    auto specialObjects = getSpecialRuntimeObjects();
    for(int i = 0; i < (int)CompilerOptimizedSelector::CompilerMessageCount; ++i)
    {
        if(specialObjects->specialObjectTable[specialObjects->compilerMessageSelectorFirst + i] == selector)
            return CompilerOptimizedSelector(i);
    }

    return CompilerOptimizedSelector::Invalid;

}


} // End of namespace Lodtalk
