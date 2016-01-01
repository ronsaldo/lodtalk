#ifndef LODTALK_OBJECT_HPP
#define LODTALK_OBJECT_HPP

#include <stdio.h>
#include "Lodtalk/ClassFactory.hpp"
#include "Lodtalk/ObjectModel.hpp"

namespace Lodtalk
{

class ClassDescription;
class MethodDictionary;
class Class;
class Metaclass;

typedef MethodDictionary *(*MethodDictionaryBuilder)();

/**
 * ProtoObject
 */
class LODTALK_VM_EXPORT ProtoObject
{
public:
	ObjectHeader object_header_;

	Oop selfOop()
	{
		return Oop::fromPointer(this);
	}

	uint8_t *getFirstFieldPointer()
	{
		uint8_t *result = reinterpret_cast<uint8_t *> (&object_header_);
		result += sizeof(ObjectHeader);
		if(object_header_.slotCount == 255)
			result += 8;
		return result;
	}

	size_t getNumberOfElements()
	{
		return selfOop().getNumberOfElements();
	}

	int identityHashValue() const
	{
		return object_header_.identityHash;
	}

    static SpecialNativeClassFactory Factory;
};

/**
 * Object
 */
class LODTALK_VM_EXPORT Object: public ProtoObject
{
public:
    static int stClass(InterpreterProxy *interpreter);
    static int stSize(InterpreterProxy *interpreter);
	static int stAt(InterpreterProxy *interpreter);
	static int stAtPut(InterpreterProxy *interpreter);
    static int stIdentityEqual(InterpreterProxy *interpreter);
    static int stIdentityHash(InterpreterProxy *interpreter);

    static SpecialNativeClassFactory Factory;
};

/**
 * Behavior
 */
class LODTALK_VM_EXPORT Behavior: public Object
{
public:
    static constexpr int BehaviorVariableCount = 5;

	Behavior* superclass;
	MethodDictionary *methodDict;
	Oop format;
	Oop fixedVariableCount;
	Oop layout;

	static int stBasicNew(InterpreterProxy *interpreter);
	static int stBasicNewSize(InterpreterProxy *interpreter);
    static int stRegisterInClassTable(InterpreterProxy *interpreter);

	Object *basicNativeNew(VMContext *context);
	Object *basicNativeNew(VMContext *context, size_t indexableSize);

	Oop superLookupSelector(Oop selector);
	Oop lookupSelector(Oop selector);

	Oop getBinding(VMContext *context);

    static SpecialNativeClassFactory Factory;
};

/**
 * ClassDescription
 */
class LODTALK_VM_EXPORT ClassDescription: public Behavior
{

public:
	static constexpr int ClassDescriptionVariableCount = BehaviorVariableCount + 2;

	Oop instanceVariables;
	Oop organization;

    static SpecialNativeClassFactory Factory;
};

/**
 * Class
 */
class LODTALK_VM_EXPORT Class: public ClassDescription
{
public:
	static constexpr int ClassVariableCount = ClassDescriptionVariableCount + 8;

    static Class *basicNativeNewClass(VMContext *context, unsigned int classIndex);

	std::string getNameString();

	Oop getBinding(VMContext *context);

	Oop subclasses;
	Oop name;
	Oop classPool;
	Oop sharedPools;
	Oop category;
	Oop environment;
	Oop traitComposition;
	Oop localSelectors;

    static SpecialNativeClassFactory Factory;
};

/**
 * Metaclass
 */
class LODTALK_VM_EXPORT Metaclass: public ClassDescription
{
public:
    static constexpr int MetaclassVariableCount = ClassDescriptionVariableCount + 3;

    static Metaclass *basicNativeNewMetaclass(VMContext *context);

	Oop getBinding(VMContext *context);

	Oop thisClass;
	Oop traitComposition;
	Oop localSelectors;

    static SpecialNativeClassFactory Factory;
};

class MethodDictionary;

/**
 * UndefinedObject
 */
class LODTALK_VM_EXPORT UndefinedObject: public Object
{
public:
	UndefinedObject()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_Nil, SCI_UndefinedObject);
	}

    static SpecialNativeClassFactory Factory;
};

/**
 * Boolean
 */
class LODTALK_VM_EXPORT Boolean: public Object
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * True
 */
class LODTALK_VM_EXPORT True: public Boolean
{
public:
	True()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_True, SCI_True);
	}

    static SpecialNativeClassFactory Factory;
};

/**
 * False
 */
class LODTALK_VM_EXPORT False: public Boolean
{
public:
	False()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_False, SCI_False);
	}

    static SpecialNativeClassFactory Factory;
};

/**
 * Magnitude
 */
class LODTALK_VM_EXPORT Magnitude: public Object
{
public:
	static SpecialNativeClassFactory Factory;
};

/**
 * Number
 */
class LODTALK_VM_EXPORT Number: public Magnitude
{
public:
    static SpecialNativeClassFactory Factory;

    static int stMakePoint(InterpreterProxy *interpreter);
};

/**
 * Integer
 */
class LODTALK_VM_EXPORT Integer: public Number
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * SmallInteger
 */
class LODTALK_VM_EXPORT SmallInteger: public Integer
{
public:
    static SpecialNativeClassFactory Factory;

    static int stPrintString(InterpreterProxy *interpreter);

    static int stAdd(InterpreterProxy *interpreter);
    static int stSub(InterpreterProxy *interpreter);
    static int stLess(InterpreterProxy *interpreter);
    static int stGreater(InterpreterProxy *interpreter);
    static int stLessEqual(InterpreterProxy *interpreter);
    static int stGreaterEqual(InterpreterProxy *interpreter);
    static int stEqual(InterpreterProxy *interpreter);
    static int stNotEqual(InterpreterProxy *interpreter);
    static int stMul(InterpreterProxy *interpreter);
    static int stDiv(InterpreterProxy *interpreter);
    static int stMod(InterpreterProxy *interpreter);
    static int stIntegerDivide(InterpreterProxy *interpreter);
    static int stQuotient(InterpreterProxy *interpreter);
    static int stBitAnd(InterpreterProxy *interpreter);
    static int stBitOr(InterpreterProxy *interpreter);
    static int stBitXor(InterpreterProxy *interpreter);
    static int stBitShift(InterpreterProxy *interpreter);

    static int stAsFloat(InterpreterProxy *interpreter);
    
};

/**
 * Float
 */
class LODTALK_VM_EXPORT Float: public Number
{
public:
    static SpecialNativeClassFactory Factory;

    static int stAdd(InterpreterProxy *interpreter);
    static int stSub(InterpreterProxy *interpreter);
    static int stLess(InterpreterProxy *interpreter);
    static int stGreater(InterpreterProxy *interpreter);
    static int stLessEqual(InterpreterProxy *interpreter);
    static int stGreaterEqual(InterpreterProxy *interpreter);
    static int stEqual(InterpreterProxy *interpreter);
    static int stNotEqual(InterpreterProxy *interpreter);
    static int stMul(InterpreterProxy *interpreter);
    static int stDiv(InterpreterProxy *interpreter);
    static int stTruncated(InterpreterProxy *interpreter);
    static int stFractionPart(InterpreterProxy *interpreter);
    static int stExponent(InterpreterProxy *interpreter);
    static int stTimesTwoPower(InterpreterProxy *interpreter);

};

/**
 * SmallFloat
 */
class LODTALK_VM_EXPORT SmallFloat: public Float
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * Character
 */
class LODTALK_VM_EXPORT Character: public Magnitude
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * Lookup key
 */
class LODTALK_VM_EXPORT LookupKey: public Magnitude
{
public:
    static SpecialNativeClassFactory Factory;

	Oop key;
};

inline Oop getLookupKeyKey(const Oop lookupKey)
{
	return reinterpret_cast<LookupKey*> (lookupKey.pointer)->key;
}

/**
 * Association
 */
class LODTALK_VM_EXPORT Association: public LookupKey
{
public:
    static SpecialNativeClassFactory Factory;

    static Association *newNativeKeyValue(VMContext *context, int clazzId, Oop key, Oop value);

	static Association* make(VMContext *context, Oop key, Oop value)
	{
		return newNativeKeyValue(context, SCI_Association, key, value);
	}

	Oop value;
};

/**
 * LiteralVariable
 */
class LODTALK_VM_EXPORT LiteralVariable: public Association
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * GlobalVariable
 */
class LODTALK_VM_EXPORT GlobalVariable: public LiteralVariable
{
public:
    static SpecialNativeClassFactory Factory;

	static Association* make(VMContext *context, Oop key, Oop value)
	{
		return newNativeKeyValue(context, SCI_GlobalVariable, key, value);
	}
};

/**
 * ClassVariable
 */
class LODTALK_VM_EXPORT ClassVariable: public LiteralVariable
{
public:
    static SpecialNativeClassFactory Factory;

	static Association* make(VMContext *context, Oop key, Oop value)
	{
		return newNativeKeyValue(context, SCI_ClassVariable, key, value);
	}
};

/**
 * Global context class
 */
class LODTALK_VM_EXPORT GlobalContext: public Object
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * The smalltalk image
 */
class LODTALK_VM_EXPORT SmalltalkImage: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static SmalltalkImage *create(VMContext *context);

    Oop globals;
};

/**
 * External handle
 */
class LODTALK_VM_EXPORT ExternalHandle: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static ExternalHandle *create(VMContext *context, void *pointer);

    void *pointer;
};

/**
 * External pointer
 */
class LODTALK_VM_EXPORT ExternalPointer: public ExternalHandle
{
public:
    static SpecialNativeClassFactory Factory;

    static ExternalPointer *create(VMContext *context, void *pointer);
};

/**
 *  Pragma
 */
class LODTALK_VM_EXPORT Pragma: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static Pragma *create(VMContext *context);

    Oop method;
    Oop keyword;
    Oop arguments;
};

// The nil oop
inline bool isNil(ProtoObject *obj)
{
	return (UndefinedObject*)obj == &NilObject;
}

inline bool isNil(Oop obj)
{
	return obj.isNil();
}

} // End of namespace Lodtalk

#endif //LODTALK_OBJECT_HPP
