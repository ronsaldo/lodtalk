#ifndef LODTALK_OBJECT_HPP
#define LODTALK_OBJECT_HPP

#include <stdio.h>
#include "ObjectModel.hpp"

namespace Lodtalk
{

#define LODTALK_NATIVE_CLASS() \
public: \
	static ClassDescription * ClassObject; \
	static ClassDescription * MetaclassObject; \


class ClassDescription;
class MethodDictionary;
class Class;
class Metaclass;

typedef MethodDictionary *(*MethodDictionaryBuilder)();

/**
 * ProtoObject
 */
class ProtoObject
{
	LODTALK_NATIVE_CLASS();
public:
	ObjectHeader object_header_;

	Oop selfOop()
	{
		return Oop::fromPointer(this);
	}

	Oop nativePerformWithArguments(Oop selector, int argumentCount, Oop *arguments)
	{
		return sendMessage(selfOop(), selector, argumentCount, arguments);
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

};

/**
 * Object
 */
class Object: public ProtoObject
{
	LODTALK_NATIVE_CLASS();
public:
	static Oop stClass(Oop self);
	static Oop stSize(Oop self);
	static Oop stAt(Oop self, Oop indexOop);
	static Oop stAtPut(Oop self, Oop index, Oop value);

};

/**
 * Behavior
 */
class Behavior: public Object
{
	LODTALK_NATIVE_CLASS();
public:
	static constexpr int BehaviorVariableCount = 5;

	Behavior* superclass;
	MethodDictionary *methodDict;
	Oop format;
	Oop fixedVariableCount;
	Oop layout;

	Oop basicNew();
	Oop basicNew(Oop indexableSize);

	Object *basicNativeNew();
	Object *basicNativeNew(size_t indexableSize);

	Oop superLookupSelector(Oop selector);
	Oop lookupSelector(Oop selector);

	Oop getBinding();

    Oop registerInClassTable();

protected:
	Behavior(Behavior* superclass, MethodDictionaryBuilder methodDictBuilder, ObjectFormat format, int fixedVariableCount)
		: superclass(superclass), methodDict(methodDictBuilder()), format(Oop::encodeSmallInteger((int)format)), fixedVariableCount(Oop::encodeSmallInteger(fixedVariableCount))
	{
		object_header_ = ObjectHeader::specialNativeClass(0, 0, BehaviorVariableCount);
	}
};

/**
 * ClassDescription
 */
class ClassDescription: public Behavior
{
	LODTALK_NATIVE_CLASS();
protected:
	ClassDescription(Behavior* superclass, MethodDictionaryBuilder methodDictBuilder, ObjectFormat format, int fixedVariableCount, const char *instanceVariableNames)
		: Behavior(superclass, methodDictBuilder, format, fixedVariableCount)
	{
		object_header_ = ObjectHeader::specialNativeClass(0, 0, ClassDescriptionVariableCount);
		instanceVariables = splitVariableNames(instanceVariableNames);
	}

public:
	static constexpr int ClassDescriptionVariableCount = BehaviorVariableCount + 2;

	Oop instanceVariables;
	Oop organization;

protected:
	static Oop splitVariableNames(const char *instanceVariableNames);

};

/**
 * Class
 */
class Class: public ClassDescription
{
	LODTALK_NATIVE_CLASS();
public:
	static constexpr int ClassVariableCount = ClassDescriptionVariableCount + 8;

	Class(const char *className, unsigned int classId, unsigned int metaclassId, ClassDescription *metaClass, Behavior* superclass, MethodDictionaryBuilder methodDictBuilder, ObjectFormat format, int fixedVariableCount, const char *instanceVariableNames)
		: ClassDescription(superclass, methodDictBuilder, format, fixedVariableCount, instanceVariableNames)
	{
		object_header_ = ObjectHeader::specialNativeClass(classId, metaclassId, ClassVariableCount);
		name = makeByteSymbol(className);
		setGlobalVariable(name, Oop::fromPointer(this));
	}

	std::string getNameString();

	Oop getBinding();

	Oop subclasses;
	Oop name;
	Oop classPool;
	Oop sharedPools;
	Oop category;
	Oop environment;
	Oop traitComposition;
	Oop localSelectors;
};

/**
 * Metaclass
 */
class Metaclass: public ClassDescription
{
	LODTALK_NATIVE_CLASS();
public:
	static constexpr int MetaclassVariableCount = ClassDescriptionVariableCount + 3;

	Metaclass(unsigned int classId, Behavior* superclass, MethodDictionaryBuilder methodDictBuilder, int fixedVariableCount, const char *instanceVariableNames, ClassDescription *thisClassPointer)
		: ClassDescription(superclass, methodDictBuilder, OF_FIXED_SIZE, Class::ClassVariableCount + fixedVariableCount, instanceVariableNames)
	{
		object_header_ = ObjectHeader::specialNativeClass(classId, SCI_Metaclass, MetaclassVariableCount);
		thisClass = Oop::fromPointer(thisClassPointer);
	}

	Oop getBinding();

	Oop thisClass;
	Oop traitComposition;
	Oop localSelectors;
};

class MethodDictionary;

/**
 * UndefinedObject
 */
class UndefinedObject: public Object
{
	LODTALK_NATIVE_CLASS();
public:
	UndefinedObject()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_Nil, SCI_UndefinedObject);
	}
};

/**
 * Boolean
 */
class Boolean: public Object
{
	LODTALK_NATIVE_CLASS();
};

/**
 * True
 */
class True: public Boolean
{
	LODTALK_NATIVE_CLASS();
public:
	True()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_True, SCI_True);
	}
};

/**
 * False
 */
class False: public Boolean
{
	LODTALK_NATIVE_CLASS();
public:
	False()
	{
		object_header_ = ObjectHeader::emptySpecialNativeClass(SOI_False, SCI_False);
	}
};

/**
 * Magnitude
 */
class Magnitude: public Object
{
	LODTALK_NATIVE_CLASS();
};

/**
 * Number
 */
class Number: public Magnitude
{
	LODTALK_NATIVE_CLASS();
};

/**
 * Integer
 */
class Integer: public Number
{
	LODTALK_NATIVE_CLASS();
};

/**
 * SmallInteger
 */
class SmallInteger: public Integer
{
	LODTALK_NATIVE_CLASS();
};

/**
 * Float
 */
class Float: public Number
{
	LODTALK_NATIVE_CLASS();
};

/**
 * SmallFloat
 */
class SmallFloat: public Float
{
	LODTALK_NATIVE_CLASS();
};

/**
 * Character
 */
class Character: public Magnitude
{
	LODTALK_NATIVE_CLASS();
};

/**
 * Lookup key
 */
class LookupKey: public Magnitude
{
	LODTALK_NATIVE_CLASS();
public:
	Oop key;
};

inline Oop getLookupKeyKey(const Oop lookupKey)
{
	return reinterpret_cast<LookupKey*> (lookupKey.pointer)->key;
}

/**
 * Association
 */
class Association: public LookupKey
{
	LODTALK_NATIVE_CLASS();
	Association() {}
public:
	static Association *newNativeKeyValue(int clazzId, Oop key, Oop value);

	static Association* make(Oop key, Oop value)
	{
		return newNativeKeyValue(SCI_Association, key, value);
	}

	Oop value;
};

/**
 * LiteralVariable
 */
class LiteralVariable: public Association
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * GlobalVariable
 */
class GlobalVariable: public LiteralVariable
{
	LODTALK_NATIVE_CLASS();
public:
	static Association* make(Oop key, Oop value)
	{
		return newNativeKeyValue(SCI_GlobalVariable, key, value);
	}
};

/**
 * ClassVariable
 */
class ClassVariable: public LiteralVariable
{
	LODTALK_NATIVE_CLASS();
public:
	static Association* make(Oop key, Oop value)
	{
		return newNativeKeyValue(SCI_ClassVariable, key, value);
	}

};

/**
 * Global context class
 */
class GlobalContext: public Object
{
	LODTALK_NATIVE_CLASS();
};

/**
 * The smalltalk image
 */
class SmalltalkImage: public Object
{
	LODTALK_NATIVE_CLASS();
public:
    static SmalltalkImage *create();

    Oop globals;
};

// Class method dictionary.
#define LODTALK_BEGIN_CLASS_TABLE(className) \
static MethodDictionary *className ## _class_methodDictBuilder() { \
	Ref<MethodDictionary> methodDict(MethodDictionary::basicNativeNew()); \

#define LODTALK_METHOD(selector, methodImplementation) \
	methodDict->addMethod(makeNativeMethodDescriptor(selector, methodImplementation));

#define LODTALK_END_CLASS_TABLE() \
	return methodDict.get(); \
}

// Metaclass method dictionary.
#define LODTALK_BEGIN_CLASS_SIDE_TABLE(className) \
static MethodDictionary *className ## _metaclass_methodDictBuilder() { \
	Ref<MethodDictionary> methodDict(MethodDictionary::basicNativeNew()); \

#define LODTALK_END_CLASS_SIDE_TABLE() \
	return methodDict.get(); \
}

// Special class and meta class definition
#define LODTALK_SPECIAL_METACLASS_DEFINITION(className, superName, fixedVariableCount, variableNames) \
static Metaclass className ## _metaclass (SMCI_ ##className, superName::MetaclassObject, &className ## _metaclass_methodDictBuilder, fixedVariableCount, variableNames, className::ClassObject); \
ClassDescription *className::MetaclassObject = &className ## _metaclass;

#define LODTALK_SPECIAL_CLASS_DEFINITION(className, superName, format, fixedVariableCount, variableNames) \
static Class className ## _class (#className, SCI_ ##className, SMCI_ ##className, className::MetaclassObject, superName::ClassObject, &className ## _class_methodDictBuilder, format, fixedVariableCount, variableNames); \
ClassDescription *className::ClassObject = &className ## _class;

#define LODTALK_SPECIAL_SUBCLASS_DEFINITION(className, superName, format, fixedVariableCount) \
LODTALK_SPECIAL_METACLASS_DEFINITION(className, superName, 0, "") \
LODTALK_SPECIAL_CLASS_DEFINITION(className, superName, format, fixedVariableCount, "")

#define LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(className, superName, format, fixedVariableCount, instanceVariableNames) \
LODTALK_SPECIAL_METACLASS_DEFINITION(className, superName, 0, "") \
LODTALK_SPECIAL_CLASS_DEFINITION(className, superName, format, fixedVariableCount, instanceVariableNames)

// Native method.
#define LODTALK_NATIVE_METHOD(selector, cppImplementation)

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
