#include <string.h>
#include "Object.hpp"
#include "Collections.hpp"
#include "Method.hpp"
#include "Exception.hpp"

namespace Lodtalk
{

// Special object
UndefinedObject NilObject;
True TrueObject;
False FalseObject;

// Proto object methods
LODTALK_BEGIN_CLASS_SIDE_TABLE(ProtoObject)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ProtoObject)
LODTALK_END_CLASS_TABLE()

// Proto object method dictionary.
static Metaclass ProtoObject_metaclass(SMCI_ProtoObject, Class::ClassObject, &ProtoObject_metaclass_methodDictBuilder, 0, "", ProtoObject::ClassObject);
ClassDescription *ProtoObject::MetaclassObject = &ProtoObject_metaclass;

static Class ProtoObject_class("ProtoObject", SCI_ProtoObject, SMCI_ProtoObject, &ProtoObject_metaclass, (Behavior*)&NilObject, &ProtoObject_class_methodDictBuilder, OF_EMPTY, 0, "");
ClassDescription *ProtoObject::ClassObject = &ProtoObject_class;

// Object methods
Oop Object::stClass(Oop self)
{
	return getClassFromOop(self);
}

Oop Object::stSize(Oop self)
{
	if(!self.isPointer())
	{
		if(self.isSmallInteger())
			nativeError("instances of Smallinteger are not indexable.");
		if(self.isCharacter())
			nativeError("instances of Character are not indexable.");
		if(self.isSmallFloat())
			nativeError("instances of Smallfloat are not indexable.");
		nativeError("not indexable instance.");
	}

	if(!self.isIndexable())
		nativeError("not indexable instance.");

	return Oop::encodeSmallInteger(self.getNumberOfElements());
}

Oop Object::stAt(Oop self, Oop indexOop)
{
	auto size = stSize(self).decodeSmallInteger();
	auto index = indexOop.decodeSmallInteger() - 1;
	if(index > size || index < 0)
		nativeError("index out of bounds.");

    // Get the element.
    auto firstIndexableField = self.getFirstIndexableFieldPointer();
    auto format = self.header->objectFormat;
    if(format < OF_INDEXABLE_64)
    {
        auto oopData = reinterpret_cast<Oop*> (firstIndexableField);
        return oopData[index];
    }

    if(format >= OF_INDEXABLE_8)
    {
        auto data = reinterpret_cast<uint8_t*> (firstIndexableField);
        return Oop::encodeSmallInteger(data[index]);
    }

    if(format >= OF_INDEXABLE_16)
    {
        auto data = reinterpret_cast<uint16_t*> (firstIndexableField);
        return Oop::encodeSmallInteger(data[index]);
    }

    if(format >= OF_INDEXABLE_32)
    {
#ifdef OBJECT_MODEL_SPUR_64
        auto data = reinterpret_cast<uint32_t*> (firstIndexableField);
        return Oop::encodeSmallInteger(data[index]);
#else
        return positiveInt32ObjectFor(data[index]);
#endif
    }

    if(format == OF_INDEXABLE_64)
    {
        auto data = reinterpret_cast<uint64_t*> (firstIndexableField);
        return positiveInt64ObjectFor(data[index]);
    }

    // Should not reach here.
	nativeError("unimplemented");
	return Oop();
}

Oop Object::stAtPut(Oop self, Oop indexOop, Oop value)
{
	auto size = stSize(self).decodeSmallInteger();
	auto index = indexOop.decodeSmallInteger() - 1;
	if(index > size || index < 0)
		nativeError("index out of bounds.");

    // Set the element.
    auto firstIndexableField = self.getFirstIndexableFieldPointer();
    auto format = self.header->objectFormat;
    if(format < OF_INDEXABLE_64)
    {
        auto oopData = reinterpret_cast<Oop*> (firstIndexableField);
        oopData[index] = value;
        return self;
    }
    else if(format >= OF_INDEXABLE_8)
    {
        auto data = reinterpret_cast<uint8_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
        return self;
    }
    else if(format >= OF_INDEXABLE_16)
    {
        auto data = reinterpret_cast<uint16_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
        return self;
    }
    else if(format >= OF_INDEXABLE_32)
    {
#ifdef OBJECT_MODEL_SPUR_64
        auto data = reinterpret_cast<uint32_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
#else
        data[index] = positiveInt32ValueOf(value);
        return self;
#endif
    }
    else if(format == OF_INDEXABLE_64)
    {
        auto data = reinterpret_cast<uint64_t*> (firstIndexableField);
        data[index] = positiveInt64ValueOf(value);
        return self;
    }

	nativeError("unimplemented");
	return Oop();
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Object)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Object)
	LODTALK_METHOD("class", Object::stClass)
	LODTALK_METHOD("size", Object::stSize)
	LODTALK_METHOD("at:", Object::stAt)
	LODTALK_METHOD("at:put:", Object::stAtPut)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Object, ProtoObject, OF_EMPTY, 0);

// Undefined object
LODTALK_BEGIN_CLASS_SIDE_TABLE(UndefinedObject)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(UndefinedObject)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(UndefinedObject, Object, OF_EMPTY, 0);

// Behavior
Oop Behavior::basicNew()
{
	return Oop::fromPointer(basicNativeNew());
}

Oop Behavior::basicNew(Oop indexableSize)
{
	return Oop::fromPointer(basicNativeNew(indexableSize.decodeSmallInteger()));
}

Object *Behavior::basicNativeNew(size_t indexableSize)
{
	auto theFormat = (ObjectFormat)format.decodeSmallInteger();
	auto fixedSlotCount = fixedVariableCount.decodeSmallInteger();
	auto classIndex = object_header_.identityHash;
	return reinterpret_cast<Object*> (newObject(fixedSlotCount, indexableSize, theFormat, classIndex));
}

Object *Behavior::basicNativeNew()
{
	return basicNativeNew(0);
}

Oop Behavior::superLookupSelector(Oop selector)
{
	// Find in the super class.
	if(isNil(superclass))
		return nilOop();
	return superclass->lookupSelector(selector);
}

Oop Behavior::lookupSelector(Oop selector)
{
	// Sanity check.
	if(isNil(methodDict))
		return nilOop();

	// Look the method in the dictionary.
	auto method = methodDict->atOrNil(selector);
	if(!isNil(method))
		return method;

	// Find in the super class.
	if(isNil(superclass))
		return nilOop();
	return superclass->lookupSelector(selector);
}

Oop Behavior::getBinding()
{
	// Dispatch manually the message.
	auto classIndex = classIndexOf(selfOop());
	// Am I a metaclass?
	if(classIndex == SCI_Metaclass)
		return static_cast<Metaclass*> (this)->getBinding();

	// Am I a class?
	auto myClass = getClassFromIndex(classIndex);
	if(classIndexOf(myClass) == SCI_Metaclass)
		return static_cast<Class*> (this)->getBinding();

	// I need to send an actual message
	LODTALK_UNIMPLEMENTED();
}

Oop Behavior::registerInClassTable()
{
    assert(selfOop().isPointer());
    registerClassInTable(selfOop());
    return selfOop();
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Behavior)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Behavior)
	LODTALK_METHOD("basicNew", static_cast<Oop (Behavior::*)()> (&Behavior::basicNew))
	LODTALK_METHOD("basicNew:", static_cast<Oop (Behavior::*)(Oop)> (&Behavior::basicNew))
    LODTALK_METHOD("registerInClassTable", &Behavior::registerInClassTable)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(Behavior, Object, OF_FIXED_SIZE, Behavior::BehaviorVariableCount, "superclass methodDict format fixedVariableCount layout");

// ClassDescription
Oop ClassDescription::splitVariableNames(const char *instanceVariableNames)
{
	return ByteString::splitVariableNames(instanceVariableNames);
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(ClassDescription)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ClassDescription)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(ClassDescription, Behavior, OF_FIXED_SIZE, ClassDescription::ClassDescriptionVariableCount, "instanceVariables organization");

// Class
Oop Class::getBinding()
{
	// Find myself in the global dictionary
	auto result = getGlobalFromSymbol(name);
	if(!isNil(result))
		return result;

	// TODO: Find an existing
	return Oop::fromPointer(Association::make(nilOop(), selfOop()));
}

std::string Class::getNameString()
{
	if(!name.pointer)
		return "uninitialized";
	if(name.isNil())
		return "UnknownClass";

	ByteSymbol *nameSymbol = reinterpret_cast<ByteSymbol *> (name.pointer);
	return nameSymbol->getString();
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Class)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Class)
	LODTALK_METHOD("binding", &Class::getBinding)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(Class, ClassDescription, OF_FIXED_SIZE, Class::ClassVariableCount,
"subclasses name classPool sharedPools category environment traitComposition localSelectors");

// Metaclass
Oop Metaclass::getBinding()
{
	// TODO: Find an existing
	return Oop::fromPointer(Association::make(nilOop(), selfOop()));
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Metaclass)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Metaclass)
	LODTALK_METHOD("binding", &Metaclass::getBinding)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(Metaclass, ClassDescription, OF_FIXED_SIZE, Metaclass::MetaclassVariableCount,
"thisClass traitComposition localSelectors");

// Boolean
LODTALK_BEGIN_CLASS_SIDE_TABLE(Boolean)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Boolean)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Boolean, Object, OF_EMPTY, 0);

// True
LODTALK_BEGIN_CLASS_SIDE_TABLE(True)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(True)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(True, Boolean, OF_EMPTY, 0);

// False
LODTALK_BEGIN_CLASS_SIDE_TABLE(False)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(False)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(False, Boolean, OF_EMPTY, 0);

// Magnitude
LODTALK_BEGIN_CLASS_SIDE_TABLE(Magnitude)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Magnitude)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Magnitude, Object, OF_EMPTY, 0);

// Number
LODTALK_BEGIN_CLASS_SIDE_TABLE(Number)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Number)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Number, Magnitude, OF_EMPTY, 0);

// Integer
LODTALK_BEGIN_CLASS_SIDE_TABLE(Integer)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Integer)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Integer, Number, OF_EMPTY, 0);

// SmallInteger
static Oop SmallInteger_printString(Oop self)
{
    if(!self.isSmallInteger())
        nativeError("expected a small integer.");

    char buffer[128];
    size_t dest = 0;
    auto temp = self.decodeSmallInteger();
    bool negative = temp < 0;

    // Extract the integer characters.
    do
    {
        auto d = (temp % 10);
        if(d <0)
            d = -d;

        buffer[dest++] = '0' + d;
        temp /= 10;
    } while(temp != 0);

    // Put the minus sign.
    if(negative)
        buffer[dest++] = '-';

    return Oop::fromPointer(ByteString::fromNativeReverseRange(buffer, dest));
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(SmallInteger)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(SmallInteger)
    LODTALK_METHOD("printString", SmallInteger_printString)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(SmallInteger, Integer, OF_EMPTY, 0);

// Float
LODTALK_BEGIN_CLASS_SIDE_TABLE(Float)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Float)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Float, Number, OF_EMPTY, 0);

// SmallFloat
LODTALK_BEGIN_CLASS_SIDE_TABLE(SmallFloat)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(SmallFloat)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(SmallFloat, Float, OF_EMPTY, 0);

// Character
LODTALK_BEGIN_CLASS_SIDE_TABLE(Character)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Character)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Character, Magnitude, OF_EMPTY, 0);

// LookupKey
LODTALK_BEGIN_CLASS_SIDE_TABLE(LookupKey)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(LookupKey)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(LookupKey, Magnitude, OF_FIXED_SIZE, 1, "key");

// Association
Association* Association::newNativeKeyValue(int classIndex, Oop key, Oop value)
{
	auto assoc = reinterpret_cast<Association*> (newObject(2, 0, OF_FIXED_SIZE, classIndex));
	assoc->key = key;
	assoc->value = value;
	return assoc;
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Association)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Association)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(Association, LookupKey, OF_FIXED_SIZE, 2, "value");

// LiteralVariable
LODTALK_BEGIN_CLASS_SIDE_TABLE(LiteralVariable)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(LiteralVariable)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(LiteralVariable, Association, OF_FIXED_SIZE, 2);

// GlobalVariable
LODTALK_BEGIN_CLASS_SIDE_TABLE(GlobalVariable)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(GlobalVariable)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(GlobalVariable, LiteralVariable, OF_FIXED_SIZE, 2);

// ClassVariable
LODTALK_BEGIN_CLASS_SIDE_TABLE(ClassVariable)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ClassVariable)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(ClassVariable, LiteralVariable, OF_FIXED_SIZE, 2);

// GlobalContext
LODTALK_BEGIN_CLASS_SIDE_TABLE(GlobalContext)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(GlobalContext)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(GlobalContext, Object, OF_EMPTY, 0);

// Smalltalk image
SmalltalkImage *SmalltalkImage::create()
{
    return reinterpret_cast<SmalltalkImage*> (newObject(1, 0, OF_FIXED_SIZE, SCI_SmalltalkImage));
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(SmalltalkImage)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(SmalltalkImage)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(SmalltalkImage, Object, OF_FIXED_SIZE, 1,
"globals");


} // End of namespace Lodtalk
