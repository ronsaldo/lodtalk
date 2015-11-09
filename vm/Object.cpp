#include <string.h>
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/Object.hpp"
#include "Lodtalk/Collections.hpp"
#include "Lodtalk/InterpreterProxy.hpp"
#include "Lodtalk/Exception.hpp"
#include "Method.hpp"

namespace Lodtalk
{

// Special object
UndefinedObject NilObject;
True TrueObject;
False FalseObject;

// Proto object methods
SpecialNativeClassFactory ProtoObject::Factory("ProtoObject", SCI_ProtoObject, nullptr, [](ClassBuilder &builder) {
});

// Proto object method dictionary.
/*
static Metaclass ProtoObject_metaclass(SMCI_ProtoObject, Class::ClassObject, &ProtoObject_metaclass_methodDictBuilder, 0, "", ProtoObject::ClassObject);
ClassDescription *ProtoObject::MetaclassObject = &ProtoObject_metaclass;

static Class ProtoObject_class("ProtoObject", SCI_ProtoObject, SMCI_ProtoObject, &ProtoObject_metaclass, (Behavior*)&NilObject, &ProtoObject_class_methodDictBuilder, OF_EMPTY, 0, "");
ClassDescription *ProtoObject::ClassObject = &ProtoObject_class;
*/

// Object methods
int Object::stClass(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();
	return interpreter->returnOop(interpreter->getContext()->getClassFromOop(interpreter->getReceiver()));
}

int Object::stSize(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto self = interpreter->getReceiver();
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

	return interpreter->returnSmallInteger(self.getNumberOfElements());
}

int Object::stAt(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    interpreter->pushOop(interpreter->getReceiver());
    auto error = stSize(interpreter);
    if(error != 0)
        return error;

    Oop self = interpreter->getReceiver();
    Oop indexOop = interpreter->getTemporary(0);
    auto size = interpreter->popOop().decodeSmallInteger();
	auto index = indexOop.decodeSmallInteger() - 1;
	if(index > size || index < 0)
		nativeError("index out of bounds.");

    // Get the element.
    auto context = interpreter->getContext();
    auto firstIndexableField = self.getFirstIndexableFieldPointer(context);
    auto format = self.header->objectFormat;
    if(format < OF_INDEXABLE_64)
    {
        auto oopData = reinterpret_cast<Oop*> (firstIndexableField);
        return interpreter->returnOop(oopData[index]);
    }

    if(format >= OF_INDEXABLE_8)
    {
        auto data = reinterpret_cast<uint8_t*> (firstIndexableField);
        return interpreter->returnOop(Oop::encodeSmallInteger(data[index]));
    }

    if(format >= OF_INDEXABLE_16)
    {
        auto data = reinterpret_cast<uint16_t*> (firstIndexableField);
        return interpreter->returnOop(Oop::encodeSmallInteger(data[index]));
    }

    if(format >= OF_INDEXABLE_32)
    {
#ifdef OBJECT_MODEL_SPUR_64
        auto data = reinterpret_cast<uint32_t*> (firstIndexableField);
        return interpreter->returnOop(Oop::encodeSmallInteger(data[index]));
#else
        return interpreter->returnOop(context->positiveInt32ObjectFor(data[index]));
#endif
    }

    if(format == OF_INDEXABLE_64)
    {
        auto data = reinterpret_cast<uint64_t*> (firstIndexableField);
        return interpreter->returnOop(context->positiveInt64ObjectFor(data[index]));
    }

    // Should not reach here.
	nativeError("unimplemented");
	abort();
}


int Object::stAtPut(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 2)
        return interpreter->primitiveFailed();

    interpreter->pushOop(interpreter->getReceiver());
    auto error = stSize(interpreter);
    if(error != 0)
        return error;

    auto self = interpreter->getReceiver();
    auto indexOop = interpreter->getTemporary(0);
    auto value = interpreter->getTemporary(1);
	auto size = interpreter->popOop().decodeSmallInteger();
	auto index = indexOop.decodeSmallInteger() - 1;
	if(index > size || index < 0)
		nativeError("index out of bounds.");

    // Set the element.
    auto context = interpreter->getContext();
    auto firstIndexableField = self.getFirstIndexableFieldPointer(context);
    auto format = self.header->objectFormat;
    if(format < OF_INDEXABLE_64)
    {
        auto oopData = reinterpret_cast<Oop*> (firstIndexableField);
        oopData[index] = value;
    }
    else if(format >= OF_INDEXABLE_8)
    {
        auto data = reinterpret_cast<uint8_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
    }
    else if(format >= OF_INDEXABLE_16)
    {
        auto data = reinterpret_cast<uint16_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
    }
    else if(format >= OF_INDEXABLE_32)
    {
#ifdef OBJECT_MODEL_SPUR_64
        auto data = reinterpret_cast<uint32_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            nativeError("expected a small integer.");

        data[index] = value.decodeSmallInteger();
#else
        data[index] = context->positiveInt32ValueOf(value);
#endif
    }
    else if(format == OF_INDEXABLE_64)
    {
        auto data = reinterpret_cast<uint64_t*> (firstIndexableField);
        data[index] = context->positiveInt64ValueOf(value);
    }
    else
    {
    	nativeError("unimplemented");
    }

    return interpreter->returnReceiver();
}

// Object
SpecialNativeClassFactory Object::Factory("Object", SCI_Object, &ProtoObject::Factory, [](ClassBuilder &builder) {
    builder
        .addMethod("class", Object::stClass)
        .addMethod("size", Object::stSize)
        .addMethod("at:", Object::stAt)
        .addMethod("at:put:", Object::stAtPut);
});

// Undefined object
SpecialNativeClassFactory UndefinedObject::Factory("UndefinedObject", SCI_UndefinedObject, &Object::Factory, [](ClassBuilder &builder) {
});

// Behavior
Oop Behavior::basicNew(VMContext *context)
{
	return Oop::fromPointer(basicNativeNew(context));
}

Oop Behavior::basicNew(VMContext *context, Oop indexableSize)
{
	return Oop::fromPointer(basicNativeNew(context, indexableSize.decodeSmallInteger()));
}

Object *Behavior::basicNativeNew(VMContext *context, size_t indexableSize)
{
	auto theFormat = (ObjectFormat)format.decodeSmallInteger();
	auto fixedSlotCount = fixedVariableCount.decodeSmallInteger();
	auto classIndex = object_header_.identityHash;
	return reinterpret_cast<Object*> (context->newObject(fixedSlotCount, indexableSize, theFormat, classIndex));
}

Object *Behavior::basicNativeNew(VMContext *context)
{
	return basicNativeNew(context, 0);
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

Oop Behavior::getBinding(VMContext *context)
{
	// Dispatch manually the message.
	auto classIndex = classIndexOf(selfOop());
	// Am I a metaclass?
	if(classIndex == SCI_Metaclass)
		return static_cast<Metaclass*> (this)->getBinding(context);

	// Am I a class?
	auto myClass = context->getClassFromIndex(classIndex);
	if(classIndexOf(myClass) == SCI_Metaclass)
		return static_cast<Class*> (this)->getBinding(context);

	// I need to send an actual message
	LODTALK_UNIMPLEMENTED();
}

Oop Behavior::registerInClassTable(VMContext *context)
{
    assert(selfOop().isPointer());
    context->registerClassInTable(selfOop());
    return selfOop();
}

SpecialNativeClassFactory Behavior::Factory("Behavior", SCI_Behavior, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("superclass", "methodDict", "format", "fixedVariableCount", "layout");
    /*
    LODTALK_METHOD("basicNew", static_cast<Oop (Behavior::*)()> (&Behavior::basicNew))
	LODTALK_METHOD("basicNew:", static_cast<Oop (Behavior::*)(Oop)> (&Behavior::basicNew))
    LODTALK_METHOD("registerInClassTable", &Behavior::registerInClassTable)
    */
});

// ClassDescription
SpecialNativeClassFactory ClassDescription::Factory("ClassDescription", SCI_ClassDescription, &Behavior::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("instanceVariables", "organization");
});

// Class
Class *Class::basicNativeNewClass(VMContext *context, unsigned int classIndex)
{
    return reinterpret_cast<Class*> (context->newObject(ClassVariableCount, 0, OF_FIXED_SIZE, classIndex));
}

Oop Class::getBinding(VMContext *context)
{
	// Find myself in the global dictionary
	auto result = context->getGlobalFromSymbol(name);
	if(!isNil(result))
		return result;

	// TODO: Find an existing
	return Oop::fromPointer(Association::make(context, nilOop(), selfOop()));
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

SpecialNativeClassFactory Class::Factory("Class", SCI_Class, &ClassDescription::Factory, [](ClassBuilder &builder) {
    //LODTALK_METHOD("binding", &Class::getBinding)

    builder
        .addInstanceVariables("subclasses", "name", "classPool", "sharedPools", "category", "environment", "traitComposition", "localSelectors");
});

// Metaclass
Metaclass *Metaclass::basicNativeNewMetaclass(VMContext *context)
{
    return reinterpret_cast<Metaclass*> (context->newObject(MetaclassVariableCount, 0, OF_FIXED_SIZE, SCI_Metaclass));
}

Oop Metaclass::getBinding(VMContext *context)
{
	// TODO: Find an existing
	return Oop::fromPointer(Association::make(context, nilOop(), selfOop()));
}

SpecialNativeClassFactory Metaclass::Factory("Metaclass", SCI_Metaclass, &ClassDescription::Factory, [](ClassBuilder &builder) {
    //LODTALK_METHOD("binding", &Metaclass::getBinding)

    builder
        .addInstanceVariables("thisClass", "traitComposition", "localSelectors");
});

// Boolean
SpecialNativeClassFactory Boolean::Factory("Boolean", SCI_Boolean, &Object::Factory, [](ClassBuilder &builder) {
});

// True
SpecialNativeClassFactory True::Factory("True", SCI_True, &Boolean::Factory, [](ClassBuilder &builder) {
});

// False
SpecialNativeClassFactory False::Factory("False", SCI_False, &Boolean::Factory, [](ClassBuilder &builder) {
});

// Magnitude
SpecialNativeClassFactory Magnitude::Factory("Magnitude", SCI_Magnitude, &Object::Factory, [](ClassBuilder &builder) {
});

// Number
SpecialNativeClassFactory Number::Factory("Number", SCI_Number, &Magnitude::Factory, [](ClassBuilder &builder) {
});

// Integer
SpecialNativeClassFactory Integer::Factory("Integer", SCI_Integer, &Number::Factory, [](ClassBuilder &builder) {
});

// SmallInteger
/*static Oop SmallInteger_printString(Oop self)
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
*/

SpecialNativeClassFactory SmallInteger::Factory("SmallInteger", SCI_SmallInteger, &Integer::Factory, [](ClassBuilder &builder) {
    //LODTALK_METHOD("printString", SmallInteger_printString)
});

// Float
SpecialNativeClassFactory Float::Factory("Float", SCI_Float, &Number::Factory, [](ClassBuilder &builder) {
});

// SmallFloat
SpecialNativeClassFactory SmallFloat::Factory("SmallFloat", SCI_SmallFloat, &Float::Factory, [](ClassBuilder &builder) {
});

// Character
SpecialNativeClassFactory Character::Factory("Character", SCI_Character, &Magnitude::Factory, [](ClassBuilder &builder) {
});

// LookupKey
SpecialNativeClassFactory LookupKey::Factory("LookupKey", SCI_LookupKey, &Magnitude::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("key");
});

// Association
Association* Association::newNativeKeyValue(VMContext *context, int classIndex, Oop key, Oop value)
{
	auto assoc = reinterpret_cast<Association*> (context->newObject(2, 0, OF_FIXED_SIZE, classIndex));
	assoc->key = key;
	assoc->value = value;
	return assoc;
}

SpecialNativeClassFactory Association::Factory("Association", SCI_Association, &LookupKey::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("value");
});

// LiteralVariable
SpecialNativeClassFactory LiteralVariable::Factory("LiteralVariable", SCI_LiteralVariable, &Association::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("value");
});

// GlobalVariable
SpecialNativeClassFactory GlobalVariable::Factory("GlobalVariable", SCI_GlobalVariable, &LiteralVariable::Factory, [](ClassBuilder &builder) {
});

// ClassVariable
SpecialNativeClassFactory ClassVariable::Factory("ClassVariable", SCI_ClassVariable, &LiteralVariable::Factory, [](ClassBuilder &builder) {
});

// GlobalContext
SpecialNativeClassFactory GlobalContext::Factory("GlobalContext", SCI_GlobalContext, &Object::Factory, [](ClassBuilder &builder) {
});

// Smalltalk image
SmalltalkImage *SmalltalkImage::create(VMContext *context)
{
    return reinterpret_cast<SmalltalkImage*> (context->newObject(1, 0, OF_FIXED_SIZE, SCI_SmalltalkImage));
}

SpecialNativeClassFactory SmalltalkImage::Factory("SmalltalkImage", SCI_SmalltalkImage, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("globals");
});

} // End of namespace Lodtalk
