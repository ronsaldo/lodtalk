#ifndef LODTALK_OBJECT_MODEL_HPP_
#define LODTALK_OBJECT_MODEL_HPP_

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include "Common.hpp"

#if UINTPTR_MAX > UINT32_MAX
#define OBJECT_MODEL_SPUR_64 1
#define LODTALK_HAS_SMALLFLOAT 1
#else
#define OBJECT_MODEL_SPUR_32 1
#endif

namespace Lodtalk
{

// Object constants
struct ObjectTag
{
#ifdef OBJECT_MODEL_SPUR_64
    static const uintptr_t TagBits = 3;

	static const uintptr_t PointerMask = 7;
	static const uintptr_t Pointer = 0;

	static const uintptr_t SmallInteger = 1;
	static const uintptr_t SmallIntegerMask = 1;
	static const uintptr_t SmallIntegerShift = 1;

	static const uintptr_t Character = 2;
	static const uintptr_t CharacterMask = 7;
	static const uintptr_t CharacterShift = 3;

	static const uintptr_t SmallFloat = 4;
	static const uintptr_t SmallFloatMask = 7;
	static const uintptr_t SmallFloatShift = 3;
#else
    static const uintptr_t TagBits = 2;

	static const uintptr_t PointerMask = 3;
	static const uintptr_t Pointer = 0;

	static const uintptr_t SmallInteger = 1;
	static const uintptr_t SmallIntegerMask = 1;
	static const uintptr_t SmallIntegerShift = 1;

	static const uintptr_t Character = 2;
	static const uintptr_t CharacterMask = 3;
	static const uintptr_t CharacterShift = 2;
#endif
};

static const uintptr_t IdentityHashMask = (1<<22) - 1;

enum ObjectFormat
{
	OF_EMPTY = 0,
	OF_FIXED_SIZE = 1,
	OF_VARIABLE_SIZE_NO_IVARS = 2,
	OF_VARIABLE_SIZE_IVARS = 3,
	OF_WEAK_VARIABLE_SIZE = 4,
	OF_WEAK_FIXED_SIZE = 5,
	OF_INDEXABLE_64 = 9,
	OF_INDEXABLE_32 = 10,
	OF_INDEXABLE_32_1,
	OF_INDEXABLE_16 = 12,
	OF_INDEXABLE_16_1,
	OF_INDEXABLE_16_2,
	OF_INDEXABLE_16_3,
	OF_INDEXABLE_8 = 16,
	OF_INDEXABLE_8_1,
	OF_INDEXABLE_8_2,
	OF_INDEXABLE_8_3,
	OF_INDEXABLE_8_4,
	OF_INDEXABLE_8_5,
	OF_INDEXABLE_8_6,
	OF_INDEXABLE_8_7,
	OF_COMPILED_METHOD = 24,
	OF_COMPILED_METHOD_1,
	OF_COMPILED_METHOD_2,
	OF_COMPILED_METHOD_3,
	OF_COMPILED_METHOD_4,
	OF_COMPILED_METHOD_5,
	OF_COMPILED_METHOD_6,
	OF_COMPILED_METHOD_7,

	OF_INDEXABLE_NATIVE_FIRST = OF_INDEXABLE_64,
};

enum class SpecialMessageSelector
{
    // Arithmetic message
    Add = 0,
    Minus,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,
    Equal,
    NotEqual,
    Multiply,
    Divide,
    Remainder,
    MakePoint,
    BitShift,
    IntegerDivision,
    BitAnd,
    BitOr,

    // Object accessing
    At,
    AtPut,
    Size,
    Next,
    NextPut,
    AtEnd,
    IdentityEqual,
    Class,

    // Unassigned
    Unassigned,

    // Block evaluation
    Value,
    ValueArg,
    Do,

    // Object instantiation
    New,
    NewArray,
    X,
    Y,

    SpecialMessageCount,
    FirstArithmeticMessage = Add,
    ArithmeticMessageCount = BitOr - FirstArithmeticMessage,
};

/**
 * Special selectors that are optimized by the compilers.
 */
enum class CompilerOptimizedSelector
{
    Invalid = -1,

    // Branches
    IfTrue = 0,
    IfFalse,
    IfTrueIfFalse,
    IfFalseIfTrue,
    IfNil,
    IfNotNil,
    IfNilIfNotNil,
    IfNotNilIfNil,

    // Loops
    WhileTrue,
    WhileFalse,
    Repeat,
    DoWhileTrue,
    DoWhileFalse,

    // Iteration
    ToByDo,
    ToDo,

    CompilerMessageCount,
};

inline size_t variableSlotSizeFor(ObjectFormat format)
{
	switch(format)
	{
	case OF_EMPTY:
	case OF_FIXED_SIZE:
		return 0;
	case OF_VARIABLE_SIZE_NO_IVARS:
	case OF_VARIABLE_SIZE_IVARS:
	case OF_WEAK_VARIABLE_SIZE:
		return sizeof(void*);
	case OF_WEAK_FIXED_SIZE:
		return 0;
	case OF_INDEXABLE_64:
		return 8;
	case OF_INDEXABLE_32:
	case OF_INDEXABLE_32_1:
		return 4;
	case OF_INDEXABLE_16:
	case OF_INDEXABLE_16_1:
	case OF_INDEXABLE_16_2:
	case OF_INDEXABLE_16_3:
		return 2;
	case OF_INDEXABLE_8:
	case OF_INDEXABLE_8_1:
	case OF_INDEXABLE_8_2:
	case OF_INDEXABLE_8_3:
	case OF_INDEXABLE_8_4:
	case OF_INDEXABLE_8_5:
	case OF_INDEXABLE_8_6:
	case OF_INDEXABLE_8_7:
	case OF_COMPILED_METHOD:
	case OF_COMPILED_METHOD_1:
	case OF_COMPILED_METHOD_2:
	case OF_COMPILED_METHOD_3:
	case OF_COMPILED_METHOD_4:
	case OF_COMPILED_METHOD_5:
	case OF_COMPILED_METHOD_6:
	case OF_COMPILED_METHOD_7:
		return 1;
	default: abort();
	}
};

inline size_t variableSlotDivisor(ObjectFormat format)
{
#ifdef OBJECT_MODEL_SPUR_64
	switch(format)
	{
	case OF_INDEXABLE_64:
		return 1;
	case OF_INDEXABLE_32:
	case OF_INDEXABLE_32_1:
		return 2;
	case OF_INDEXABLE_16:
	case OF_INDEXABLE_16_1:
	case OF_INDEXABLE_16_2:
	case OF_INDEXABLE_16_3:
		return 4;
	case OF_INDEXABLE_8:
	case OF_INDEXABLE_8_1:
	case OF_INDEXABLE_8_2:
	case OF_INDEXABLE_8_3:
	case OF_INDEXABLE_8_4:
	case OF_INDEXABLE_8_5:
	case OF_INDEXABLE_8_6:
	case OF_INDEXABLE_8_7:
	case OF_COMPILED_METHOD:
	case OF_COMPILED_METHOD_1:
	case OF_COMPILED_METHOD_2:
	case OF_COMPILED_METHOD_3:
	case OF_COMPILED_METHOD_4:
	case OF_COMPILED_METHOD_5:
	case OF_COMPILED_METHOD_6:
	case OF_COMPILED_METHOD_7:
		return 8;
	default: abort();
	}
#else
	switch(format)
	{
	case OF_INDEXABLE_32:
	case OF_INDEXABLE_32_1:
		return 1;
	case OF_INDEXABLE_16:
	case OF_INDEXABLE_16_1:
	case OF_INDEXABLE_16_2:
	case OF_INDEXABLE_16_3:
		return 2;
	case OF_INDEXABLE_8:
	case OF_INDEXABLE_8_1:
	case OF_INDEXABLE_8_2:
	case OF_INDEXABLE_8_3:
	case OF_INDEXABLE_8_4:
	case OF_INDEXABLE_8_5:
	case OF_INDEXABLE_8_6:
	case OF_INDEXABLE_8_7:
	case OF_COMPILED_METHOD:
	case OF_COMPILED_METHOD_1:
	case OF_COMPILED_METHOD_2:
	case OF_COMPILED_METHOD_3:
	case OF_COMPILED_METHOD_4:
	case OF_COMPILED_METHOD_5:
	case OF_COMPILED_METHOD_6:
	case OF_COMPILED_METHOD_7:
		return 1;
	default: abort();
	}
#endif
};

inline constexpr unsigned int generateIdentityHash(void *ptr)
{
	return (unsigned int)((reinterpret_cast<uintptr_t> (ptr) >> ObjectTag::TagBits) & IdentityHashMask);
}

struct ObjectHeader
{
	uint8_t slotCount;
	unsigned int isImmutable : 1;
	unsigned int isPinned : 1;
	unsigned int identityHash : 22;
	unsigned int gcColor : 3;
	unsigned int objectFormat : 5;
	unsigned int reserved : 2;
	unsigned int classIndex : 22;

	static constexpr ObjectHeader specialNativeClass(unsigned int identityHash, unsigned int classIndex, uint8_t slotCount, ObjectFormat format = OF_FIXED_SIZE)
	{
		return {slotCount, false, true, identityHash, 0, (unsigned int)format, 0, classIndex};
	}

	static constexpr ObjectHeader emptySpecialNativeClass(unsigned int identityHash, unsigned int classIndex)
	{
		return {0, true, true, identityHash, 0, OF_EMPTY, 0, classIndex};
	}

	static constexpr ObjectHeader emptyNativeClass(void *self, unsigned int classIndex)
	{
		return {0, true, true, generateIdentityHash(self), 0, OF_EMPTY, 0, classIndex};
	}

};
static_assert(sizeof(ObjectHeader) == 8, "Object header size must be 8");

typedef intptr_t SmallIntegerValue;
static constexpr SmallIntegerValue SmallIntegerMin = SmallIntegerValue(1) << (sizeof(SmallIntegerValue)*8 - 1);
static constexpr SmallIntegerValue SmallIntegerMax = ~SmallIntegerMin;

inline bool unsignedFitsInSmallInteger(uintptr_t value)
{
    return value <= (uintptr_t)SmallIntegerMax;
}

inline bool signedFitsInSmallInteger(intptr_t value)
{
    return SmallIntegerMin <= value && value <= SmallIntegerMax;
}

// Some special objects
class UndefinedObject;
class True;
class False;

extern UndefinedObject NilObject;
extern True TrueObject;
extern False FalseObject;

struct Oop
{
private:
	constexpr Oop(uint8_t *pointer) : pointer(pointer) {}
	constexpr Oop(intptr_t intValue) : intValue(intValue) {}
	constexpr Oop(int, uintptr_t uintValue) : uintValue(uintValue) {}

public:
	constexpr Oop() : pointer(reinterpret_cast<uint8_t*> (&NilObject)) {}

	static constexpr Oop fromPointer(void *pointer)
	{
		return Oop(reinterpret_cast<uint8_t*> (pointer));
	}

	static constexpr Oop fromRawUIntPtr(uintptr_t value)
	{
		return Oop(0, value);
	}

	inline bool isBoolean() const
	{
		return isTrue() || isFalse();
	}

	inline bool isTrue() const
	{
		return pointer == reinterpret_cast<uint8_t*> (&TrueObject);
	}

	inline bool isFalse() const
	{
		return pointer == reinterpret_cast<uint8_t*> (&FalseObject);
	}

	inline bool isNil() const
	{
		return pointer == reinterpret_cast<uint8_t*> (&NilObject);
	}

	inline bool isSmallInteger() const
	{
		return (uintValue & ObjectTag::SmallIntegerMask) == ObjectTag::SmallInteger;
	}

	inline bool isCharacter() const
	{
		return (uintValue & ObjectTag::CharacterMask) == ObjectTag::Character;
	}

	inline bool isSmallFloat() const
	{
	#ifdef LODTALK_HAS_SMALLFLOAT
		return (uintValue & ObjectTag::SmallFloatMask) == ObjectTag::SmallFloat;
	#else
		return false;
	#endif
	}

	inline bool isPointer() const
	{
		return (uintValue & ObjectTag::PointerMask) == ObjectTag::Pointer;
	}

	inline bool isIndexableNativeData() const
	{
		return isPointer() && header->objectFormat >= OF_INDEXABLE_NATIVE_FIRST;
	}

	void *getFirstFieldPointer() const
	{
		if(!isPointer())
			return nullptr;
		if(header->slotCount == 255)
			return pointer + sizeof(ObjectHeader) + 8;
		return pointer + sizeof(ObjectHeader);
	}

    void *getFirstIndexableFieldPointer() const
	{
		if(!isPointer())
			return nullptr;
		if(header->slotCount == 255)
			return pointer + sizeof(ObjectHeader) + 8;
		return pointer + sizeof(ObjectHeader) + getFixedSlotCount()*sizeof(void*);
	}

	inline SmallIntegerValue decodeSmallInteger() const
	{
		assert(isSmallInteger());
		return intValue >> ObjectTag::SmallIntegerShift;
	}

	static constexpr Oop encodeSmallInteger(SmallIntegerValue integer)
	{
		return Oop((integer << ObjectTag::SmallIntegerShift) | ObjectTag::SmallInteger);
	}

	inline int decodeCharacter() const
	{
		assert(isCharacter());
		return intValue >> ObjectTag::CharacterShift;
	}

	static constexpr Oop encodeCharacter(int character)
	{
		return Oop((character << ObjectTag::CharacterShift) | ObjectTag::Character);
	}

    inline double decodeSmallFloat() const
    {
        LODTALK_UNIMPLEMENTED();
    }

    static inline Oop encodeSmallFloat(double value)
    {
        LODTALK_UNIMPLEMENTED();
    }

	bool operator==(const Oop &o) const
	{
		return pointer == o.pointer;
	}

	bool operator!=(const Oop &o) const
	{
		return pointer != o.pointer;
	}

	bool operator<(const Oop &o) const
	{
		return intValue < o.intValue;
	}

	static constexpr Oop trueObject()
	{
		return Oop(reinterpret_cast<uint8_t*> (&TrueObject));
	}

	static constexpr Oop falseObject()
	{
		return Oop(reinterpret_cast<uint8_t*> (&FalseObject));
	}

	inline size_t getSlotCount() const
	{
		if(header->slotCount == 255)
		{
			uint64_t *theSlotCount = reinterpret_cast<uint64_t*> (pointer + sizeof(ObjectHeader));
			return *theSlotCount;
		}

		return header->slotCount;
	}

	inline bool isIndexable() const
	{
		return variableSlotSizeFor((ObjectFormat)header->objectFormat) != 0;
	}

    size_t getFixedSlotCount() const;

	inline size_t getNumberOfElements() const
	{
		auto slotCount = getSlotCount();
		auto format = header->objectFormat;
		if(format == OF_EMPTY)
			return 0;

		if(format < OF_INDEXABLE_64)
        {
            if(format == OF_VARIABLE_SIZE_IVARS)
                return slotCount - getFixedSlotCount();
			return slotCount;
        }

		if(format >= OF_INDEXABLE_8)
		{
			auto slotBytes = slotCount * sizeof(void*);
			auto extraBits = format & 7;
			if(extraBits)
				return slotBytes - sizeof(void*) + extraBits;
			else
				return slotBytes;
		}

		LODTALK_UNIMPLEMENTED();
		abort();
	}

	union
	{
		uint8_t *pointer;
		ObjectHeader *header;
		uintptr_t uintValue;
		intptr_t intValue;
	};
};

// Ensure the object oriented pointer is a pointer.
static_assert(sizeof(Oop) == sizeof(void*), "Oop structure has to be a pointer.");

inline constexpr Oop nilOop()
{
	return Oop();
}

inline constexpr Oop trueOop()
{
	return Oop::trueObject();
}

inline constexpr Oop falseOop()
{
	return Oop::falseObject();
}

//// The compiled method header format.
// Smallinteger tag: 1 bit
// Number of literals: 16 bits
// Has primitive : 1 bit
// Number of temporals: 6 bits
// Number of arguments: 4 bits
struct CompiledMethodHeader
{
	static const size_t LiteralMask = (1<<16) -1;
	static const size_t LiteralShift = 1;
	static const size_t HasPrimitiveBit = (1<<17);
	static const size_t NeedsLargeFrameBit = (1<<18);
	static const size_t TemporalShift = 19;
	static const size_t TemporalMask = (1<<6) - 1;
	static const size_t ArgumentShift = 25;
	static const size_t ArgumentMask = (1<<4) - 1;
	static const size_t ReservedBit = 1<<29;
	static const size_t FlagBit = 1<<30;
	static const size_t AlternateBytecodeBit = 1<<31;

	constexpr CompiledMethodHeader(Oop oop) : oop(oop) {}

	static CompiledMethodHeader create(size_t literalCount, size_t temporalCount, size_t argumentCount)
	{
		assert(literalCount <= LiteralMask);
		assert(temporalCount <= TemporalMask);
		assert(argumentCount <= ArgumentMask);

		return Oop::fromRawUIntPtr(1 |
		((literalCount & LiteralMask) << LiteralShift) |
		((temporalCount & TemporalMask) << TemporalShift) |
		((argumentCount & ArgumentMask) << ArgumentShift));
	}

	size_t getLiteralCount() const
	{
		return (oop.uintValue >> LiteralShift) & LiteralMask;
	}

	size_t getTemporalCount() const
	{
		return (oop.uintValue >> TemporalShift) & TemporalMask;
	}

	size_t getArgumentCount() const
	{
		return (oop.uintValue >> ArgumentShift) & ArgumentMask;
	}

	bool needsLargeFrame() const
	{
		return oop.uintValue | NeedsLargeFrameBit;
	}

	void setLargeFrame(bool value)
	{
		if(value)
			oop.uintValue |= NeedsLargeFrameBit;
		else
			oop.uintValue &= ~NeedsLargeFrameBit;
	}

	Oop oop;
};

// Object memory
uint8_t *allocateObjectMemory(size_t objectSize);
ObjectHeader *newObject(size_t fixedSlotCount, size_t indexableSize, ObjectFormat format, int classIndex, int identityHash = -1);

/**
 * Object header when the slot count is greater or equal to 255.
 */
struct BigObjectHeader
{
	ObjectHeader header;
	uint64_t slotCount;
};

enum SpecialObjectIndex
{
	SOI_Nil = 0,
	SOI_True,
	SOI_False,
};

enum SpecialClassesIndex
{
#define SPECIAL_CLASS_NAME(className) \
	SCI_ ## className, \
	SMCI_ ## className,
#include "SpecialClasses.inc"
#undef SPECIAL_CLASS_NAME
};

class ClassDescription;

// Gets the identity hash of an Object
inline int identityHashOf(Oop obj)
{
	if(obj.isSmallInteger())
		return obj.decodeSmallInteger();
	else if(obj.isCharacter())
		return obj.decodeCharacter();
	else if(obj.isSmallFloat())
		return obj.decodeSmallFloat();
	return obj.header->identityHash;
}

inline bool identityOopEquals(Oop a, Oop b)
{
	return a == b;
}

inline int classIndexOf(Oop obj)
{
	if(obj.isSmallInteger())
		return SCI_SmallInteger;
	if(obj.isCharacter())
		return SCI_Character;
	if(obj.isSmallFloat())
		return SCI_SmallFloat;

	return obj.header->classIndex;
}

template<typename X>
X identityFunction(const X &x)
{
	return x;
}

// Oop reference that can be updated by the GC.
class OopRef
{
public:
	OopRef()
	{
		registerSelf();
	}


	OopRef(const Oop &object)
		: oop(object)
	{
		registerSelf();
	}

	OopRef(const OopRef &ref)
		: oop(ref.oop)
	{
		registerSelf();
	}

	~OopRef()
	{
		unregisterSelf();
	}

	Oop oop;

	bool isNil() const
	{
		return oop.isNil();
	}

	OopRef &operator=(const Oop &oop)
	{
		this->oop = oop;
		return *this;
	}

	OopRef &operator=(const OopRef &o)
	{
		oop = o.oop;
		return *this;
	}

	bool operator<(const OopRef &o) const
	{
		return oop < o.oop;
	}

	bool operator<(const Oop &o) const
	{
		return oop < o;
	}

	bool operator==(const OopRef &o) const
	{
		return oop == o.oop;
	}

	bool operator!=(const OopRef &o) const
	{
		return oop == o.oop;
	}

	bool operator==(const Oop &o) const
	{
		return oop == o;
	}

	bool operator!=(const Oop &o) const
	{
		return oop == o;
	}

private:
	friend class GarbageCollector;

	void registerSelf();
	void unregisterSelf();

	// This is used by the GC
	OopRef* prevReference_;
	OopRef* nextReference_;
};

// Reference smart pointer
template<typename T>
class Ref
{
public:
	Ref() {}
	Ref(decltype(nullptr) ni) {}
	Ref(const Ref<T> &o)
		: reference(o.getOop()) {}

	template<typename U>
	Ref(const Ref<U> &o)
	{
		reset(o.get());
	}

	Ref(T* p)
	{
		reset(p);
	}

	static Ref<T> fromOop(Oop oop)
	{
		Ref<T> result;
		result.reference = oop;
		return result;
	}

	void reset(T *pointer=(T*)&NilObject)
	{
		internalSet(pointer);
	}

	T *operator->() const
	{
		assert(!isNil());
		return reinterpret_cast<T*> (reference.oop.pointer);
	}

	T *get() const
	{
		return reinterpret_cast<T*> (reference.oop.pointer);
	}

	Oop getOop() const
	{
		return reference.oop;
	}

	template<typename U>
	Ref<T> &operator=(const Ref<U> &o)
	{
		internalSet(o.get());
		return *this;
	}

	template<typename U>
	Ref<T> &operator=(U *o)
	{
		internalSet(o);
		return *this;
	}

	bool isSmallInteger() const
	{
		return getOop().isSmallInteger();
	}

	bool isSmallFloat() const
	{
		return getOop().isSmallFloat();
	}

	bool isCharacter() const
	{
		return getOop().isCharacter();
	}

	bool isNil() const
	{
		return getOop().isNil();
	}

private:
	void internalSet(T *pointer)
	{
		reference = Oop::fromPointer(pointer);
	}

	OopRef reference;
};

template<typename T>
Ref<T> makeRef(T *pointer)
{
	return Ref<T> (pointer);
}

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

bool isClassOrMetaclass(Oop oop);
bool isMetaclass(Oop oop);
bool isClass(Oop oop);

// Message send
Oop sendDNUMessage(Oop receiver, Oop selector, int argumentCount, Oop *arguments);
Oop lookupMessage(Oop receiver, Oop selector);
Oop sendMessage(Oop receiver, Oop selector, int argumentCount, Oop *arguments);

Oop makeByteString(const std::string &content);
Oop makeByteSymbol(const std::string &content);
Oop makeSelector(const std::string &content);

Oop sendBasicNew(Oop clazz);
Oop sendBasicNewWithSize(Oop clazz, size_t size);

// Garbage collection interface
void disableGC();
void enableGC();

void registerGCRoot(Oop *gcroot, size_t size);
void unregisterGCRoot(Oop *gcroot);

void registerNativeObject(Oop object);

class WithoutGC
{
public:
    WithoutGC()
    {
        disableGC();
    }

    ~WithoutGC()
    {
        enableGC();
    }
};

// Global variables
Oop setGlobalVariable(const char *name, Oop value);
Oop setGlobalVariable(Oop name, Oop value);

Oop getGlobalFromName(const char *name);
Oop getGlobalFromSymbol(Oop symbol);

Oop getGlobalValueFromName(const char *name);
Oop getGlobalValueFromSymbol(Oop symbol);

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

// Message sending
template<typename... Args>
Oop sendMessageOopArgs(Oop receiver, Oop selector, Args... args)
{
	Oop argArray[] = {
		args...
	};
	return sendMessage(receiver, selector, sizeof...(args), argArray);
}

} // End of namespace Lodtalk

#endif //LODTALK_OBJECT_MODEL_HPP_
