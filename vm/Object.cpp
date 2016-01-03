#include "Lodtalk/Object.hpp"
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/Collections.hpp"
#include "Lodtalk/InterpreterProxy.hpp"
#include "Lodtalk/Exception.hpp"
#include "Lodtalk/Math.hpp"
#include "Method.hpp"
#include <string.h>
#include <math.h>

namespace Lodtalk
{

// Special object
LODTALK_VM_EXPORT UndefinedObject NilObject;
LODTALK_VM_EXPORT True TrueObject;
LODTALK_VM_EXPORT False FalseObject;

// Proto object methods
SpecialNativeClassFactory ProtoObject::Factory("ProtoObject", SCI_ProtoObject, nullptr, [](ClassBuilder &builder) {
});

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
        return interpreter->primitiveFailed();

	if(!self.isIndexable())
		return interpreter->primitiveFailed();

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
		return interpreter->primitiveFailed();

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
    return interpreter->primitiveFailed();
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
		return interpreter->primitiveFailed();

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
            return interpreter->primitiveFailed();

        data[index] = (uint8_t)value.decodeSmallInteger();
    }
    else if(format >= OF_INDEXABLE_16)
    {
        auto data = reinterpret_cast<uint16_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            return interpreter->primitiveFailed();

        data[index] = (uint16_t)value.decodeSmallInteger();
    }
    else if(format >= OF_INDEXABLE_32)
    {
#ifdef OBJECT_MODEL_SPUR_64
        auto data = reinterpret_cast<uint32_t*> (firstIndexableField);
        if(!value.isSmallInteger())
            return interpreter->primitiveFailed();

        data[index] = (uint32_t)value.decodeSmallInteger();
#else
        data[index] = (uint32_t)context->positiveInt32ValueOf(value);
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

    return interpreter->returnOop(value);
}

int Object::stIdentityEqual(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto self = interpreter->getReceiver();
    auto test = interpreter->getTemporary(0);
    if(self == test)
        return interpreter->returnTrue();
    else
        return interpreter->returnFalse();
}

int Object::stIdentityHash(InterpreterProxy *interpreter)
{
    auto receiver = interpreter->getReceiver();
    return interpreter->returnSmallInteger(identityHashOf(receiver));
}

// Object
SpecialNativeClassFactory Object::Factory("Object", SCI_Object, &ProtoObject::Factory, [](ClassBuilder &builder) {
    builder
        .addPrimitiveMethod(60, "basicAt:", Object::stAt)
        .addPrimitiveMethod(61, "basicAt:put:", Object::stAtPut)
        .addPrimitiveMethod(62, "basicSize", Object::stSize)
        .addPrimitiveMethod(75, "identityHash", Object::stIdentityHash)
        .addPrimitiveMethod(110, "==", Object::stIdentityEqual)
        .addPrimitiveMethod(111, "class", Object::stClass)

        .addMethod("at:", Object::stAt)
        .addMethod("at:put:", Object::stAtPut)
        .addMethod("size:", Object::stSize)
        .addMethod("hash", Object::stIdentityHash);
});

// Undefined object
SpecialNativeClassFactory UndefinedObject::Factory("UndefinedObject", SCI_UndefinedObject, &Object::Factory, [](ClassBuilder &builder) {
});

// Behavior
int Behavior::stBasicNew(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto self = reinterpret_cast<Behavior*> (interpreter->getReceiver().pointer);
    return interpreter->returnOop(Oop::fromPointer(self->basicNativeNew(interpreter->getContext())));
}

int Behavior::stBasicNewSize(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    Oop size = interpreter->getTemporary(0);
    auto self = reinterpret_cast<Behavior*> (interpreter->getReceiver().pointer);
    return interpreter->returnOop(Oop::fromPointer(self->basicNativeNew(interpreter->getContext(), size.decodeSmallInteger())));
}

int Behavior::stRegisterInClassTable(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto selfOop = interpreter->getReceiver();
    interpreter->getContext()->registerClassInTable(selfOop);
    return interpreter->returnReceiver();
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

SpecialNativeClassFactory Behavior::Factory("Behavior", SCI_Behavior, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("superclass", "methodDict", "format", "fixedVariableCount", "layout");

    builder
        .addPrimitiveMethod(70, "basicNew", Behavior::stBasicNew)
        .addPrimitiveMethod(71, "basicNew:", Behavior::stBasicNewSize)
        .addMethod("registerInClassTable", Behavior::stRegisterInClassTable);
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
int Number::stMakePoint(InterpreterProxy *interpreter)
{
    return interpreter->primitiveFailed();
}

SpecialNativeClassFactory Number::Factory("Number", SCI_Number, &Magnitude::Factory, [](ClassBuilder &builder) {
    builder
        .addPrimitiveMethod(18, "@", Number::stMakePoint);
});

// Integer
SpecialNativeClassFactory Integer::Factory("Integer", SCI_Integer, &Number::Factory, [](ClassBuilder &builder) {
});


// SmallInteger
int SmallInteger::stPrintString(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto self = interpreter->getReceiver();
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

        buffer[dest++] = char('0' + d);
        temp /= 10;
    } while(temp != 0);

    // Put the minus sign.
    if(negative)
        buffer[dest++] = '-';

    auto result = Oop::fromPointer(ByteString::fromNativeReverseRange(interpreter->getContext(), buffer, dest));
    return interpreter->returnOop(result);
}

int SmallInteger::stAdd(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnInteger(ia + ib);
}

int SmallInteger::stSub(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnInteger(ia - ib);
}

int SmallInteger::stLess(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia < ib);
}

int SmallInteger::stGreater(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia > ib);
}

int SmallInteger::stLessEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia <= ib);
}

int SmallInteger::stGreaterEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia >= ib);
}

int SmallInteger::stEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia == ib);
}

int SmallInteger::stNotEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnBoolean(ia != ib);
}

int SmallInteger::stMul(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    auto result = ia * ib;
    if(!signedFitsInSmallInteger(result) || result / ib != ia)
        return interpreter->primitiveFailed();

    return interpreter->returnInteger(result);
}

int SmallInteger::stDiv(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    if (ib == 0 || (ia % ib != 0))
        return interpreter->primitiveFailed();

    return interpreter->returnInteger(divideRoundNeg(ia, ib));
}

int SmallInteger::stMod(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    if (ib == 0)
        return interpreter->primitiveFailed();

    return interpreter->returnInteger(moduleRoundNeg(ia, ib));
}

int SmallInteger::stIntegerDivide(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    if (ib == 0)
        return interpreter->primitiveFailed();
    return interpreter->returnInteger(divideRoundNeg(ia, ib));
}

int SmallInteger::stQuotient(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    if(ib == 0)
        return interpreter->primitiveFailed();
    return interpreter->returnInteger(ia / ib);
}

int SmallInteger::stBitAnd(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnInteger(ia & ib);
}

int SmallInteger::stBitOr(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnInteger(ia | ib);
}

int SmallInteger::stBitXor(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    return interpreter->returnInteger(ia ^ ib);
}

int SmallInteger::stBitShift(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isSmallInteger() || !b.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = a.decodeSmallInteger();
    auto ib = b.decodeSmallInteger();
    if(ib < 0)
        return interpreter->returnInteger(ia >> SmallIntegerValue(-ib));
    return interpreter->returnInteger(ia << ib);
}

int SmallInteger::stAsFloat(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto v = interpreter->getReceiver();
    if (!v.isSmallInteger())
        return interpreter->primitiveFailed();

    auto ia = v.decodeSmallInteger();
    return interpreter->returnFloat((double)ia);
}

SpecialNativeClassFactory SmallInteger::Factory("SmallInteger", SCI_SmallInteger, &Integer::Factory, [](ClassBuilder &builder) {
    builder
        .addPrimitiveMethod(1, "+", SmallInteger::stAdd)
        .addPrimitiveMethod(2, "-", SmallInteger::stSub)
        .addPrimitiveMethod(3, "<", SmallInteger::stLess)
        .addPrimitiveMethod(4, ">", SmallInteger::stGreater)
        .addPrimitiveMethod(5, "<=", SmallInteger::stLessEqual)
        .addPrimitiveMethod(6, ">=", SmallInteger::stGreaterEqual)
        .addPrimitiveMethod(7, "=", SmallInteger::stEqual)
        .addPrimitiveMethod(8, "~=", SmallInteger::stNotEqual)
        .addPrimitiveMethod(9, "*", SmallInteger::stMul)
        .addPrimitiveMethod(10, "/", SmallInteger::stDiv)
        .addPrimitiveMethod(11, "\\\\", SmallInteger::stMod)
        .addPrimitiveMethod(12, "//", SmallInteger::stIntegerDivide)
        .addPrimitiveMethod(13, "quo:", SmallInteger::stQuotient)
        .addPrimitiveMethod(14, "bitAnd:", SmallInteger::stBitAnd)
        .addPrimitiveMethod(15, "bitOr:", SmallInteger::stBitOr)
        .addPrimitiveMethod(16, "bitXor:", SmallInteger::stBitXor)
        .addPrimitiveMethod(17, "bitShift:", SmallInteger::stBitShift)

        .addPrimitiveMethod(40, "asFloat", SmallInteger::stAsFloat)

        .addMethod("printString", SmallInteger::stPrintString);
});

// Float
int Float::stAdd(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa + fb);
}

int Float::stSub(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa - fb);
}

int Float::stLess(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa < fb);
}

int Float::stGreater(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa > fb);
}

int Float::stLessEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa <= fb);
}

int Float::stGreaterEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa >= fb);
}

int Float::stEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa == fb);
}

int Float::stNotEqual(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa != fb);
}

int Float::stMul(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa * fb);
}

int Float::stDiv(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto a = interpreter->getReceiver();
    auto b = interpreter->getTemporary(0);
    if (!a.isFloatOrInt() || !b.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fa = a.decodeFloatOrInt();
    auto fb = b.decodeFloatOrInt();
    return interpreter->returnFloat(fa / fb);
}

int Float::stTruncated(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto v = interpreter->getReceiver();
    if (!v.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fv = v.decodeFloatOrInt();
    double fractPart, integerPart;
    fractPart = modf(fv, &integerPart);
    return interpreter->returnInteger((SmallIntegerValue)integerPart);
}

int Float::stFractionPart(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto v = interpreter->getReceiver();
    if (!v.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fv = v.decodeFloatOrInt();
    double fractPart, integerPart;
    fractPart = modf(fv, &integerPart);
    return interpreter->returnFloat(fractPart);
}

int Float::stExponent(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto v = interpreter->getReceiver();
    if (!v.isFloatOrInt())
        return interpreter->primitiveFailed();

    auto fv = v.decodeFloatOrInt();
    int exp;
     frexp(fv, &exp);

    return interpreter->returnInteger(exp);
}

int Float::stTimesTwoPower(InterpreterProxy *interpreter)
{
    if (interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    auto v = interpreter->getReceiver();
    auto exp = interpreter->getTemporary(0);
    if (!v.isFloatOrInt() || !exp.isSmallInteger())
        return interpreter->primitiveFailed();

    auto fv = v.decodeFloatOrInt();
    auto fexp = exp.decodeSmallInteger();
    return interpreter->returnFloat(ldexp(fv, (int)fexp));
}

SpecialNativeClassFactory Float::Factory("Float", SCI_Float, &Number::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8()

        .addPrimitiveMethod(41, "+", Float::stAdd)
        .addPrimitiveMethod(42, "-", Float::stSub)
        .addPrimitiveMethod(43, "<", Float::stLess)
        .addPrimitiveMethod(44, ">", Float::stGreater)
        .addPrimitiveMethod(45, "<=", Float::stLessEqual)
        .addPrimitiveMethod(46, ">=", Float::stGreaterEqual)
        .addPrimitiveMethod(47, "=", Float::stEqual)
        .addPrimitiveMethod(48, "~=", Float::stNotEqual)
        .addPrimitiveMethod(49, "*", Float::stMul)
        .addPrimitiveMethod(50, "/", Float::stDiv)
        .addPrimitiveMethod(51, "truncated", Float::stTruncated)
        .addPrimitiveMethod(52, "fractionPart", Float::stFractionPart)
        .addPrimitiveMethod(53, "exponent", Float::stExponent)
        .addPrimitiveMethod(54, "timesTwoPower:", Float::stTimesTwoPower);

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

int SmalltalkImage::stQuitPrimitive(InterpreterProxy *interpreter)
{
    int exitCode = 0;
    if (interpreter->getArgumentCount() == 1)
    {
        auto exitCodeObject = interpreter->getTemporary(0);
        if (exitCodeObject.isSmallInteger())
            exitCode = (int)exitCodeObject.decodeSmallInteger();
    }

    exit(exitCode);
    // Should never reach here.
}

int SmalltalkImage::stExitToDebugger(InterpreterProxy *interpreter)
{
    abort();
}

int SmalltalkImage::stNativeBreakpoint(InterpreterProxy *interpreter)
{
    return interpreter->returnReceiver();
}

int SmalltalkImage::stWordSize(InterpreterProxy *interpreter)
{
    return interpreter->returnSmallInteger(sizeof(void*));
}

SpecialNativeClassFactory SmalltalkImage::Factory("SmalltalkImage", SCI_SmalltalkImage, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("globals")

        .addPrimitiveMethod(113, "quitPrimitive", &stQuitPrimitive)
        .addPrimitiveMethod(114, "exitToDebugger", &stExitToDebugger)

        .addMethod("nativeBreakpoint", &stNativeBreakpoint)
        .addMethod("wordSize", &stWordSize);
});

// External handle
ExternalHandle *ExternalHandle::create(VMContext *context, void *pointer)
{
    auto handle = reinterpret_cast<ExternalHandle*> (context->newObject(0, sizeof(pointer), OF_INDEXABLE_8, SCI_ExternalHandle));
    handle->pointer = pointer;
    return handle;
}

SpecialNativeClassFactory ExternalHandle::Factory("ExternalHandle", SCI_ExternalHandle, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// External pointer
ExternalPointer *ExternalPointer::create(VMContext *context, void *pointer)
{
    auto externalPointer = reinterpret_cast<ExternalPointer*> (context->newObject(0, sizeof(pointer), OF_INDEXABLE_8, SCI_ExternalPointer));
    externalPointer->pointer = pointer;
    return externalPointer;
}

SpecialNativeClassFactory ExternalPointer::Factory("ExternalPointer", SCI_ExternalPointer, &ExternalHandle::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// Pragma
Pragma *Pragma::create(VMContext *context)
{
    return reinterpret_cast<Pragma *> (context->newObject(3, 0, OF_FIXED_SIZE, SCI_Pragma));
}

SpecialNativeClassFactory Pragma::Factory("Pragma", SCI_Pragma, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("method", "keyword", "arguments");
});

} // End of namespace Lodtalk
