#ifndef LODTALK_OBJECT_MODEL_HPP_
#define LODTALK_OBJECT_MODEL_HPP_

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include "Lodtalk/Definitions.h"

#if UINTPTR_MAX > UINT32_MAX
#define OBJECT_MODEL_SPUR_64 1
#define LODTALK_HAS_SMALLFLOAT 1
#else
#define OBJECT_MODEL_SPUR_32 1
#endif

namespace Lodtalk
{
class VMContext;
LODTALK_VM_EXPORT VMContext *getCurrentContext();

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

    // Dummy constants
    static const uintptr_t SmallFloat = 0;
    static const uintptr_t SmallFloatMask = 0;
    static const uintptr_t SmallFloatShift = 0
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
    BasicNew,
    BasicNewSize,

    // Does not understand
    DoesNotUnderstand,
    NativeMethodFailed,

    SpecialMessageCount,
    SpecialMessageOptimizedCount = BasicNew,
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
#include "Lodtalk/SpecialClasses.inc"
#undef SPECIAL_CLASS_NAME
    SpecialClassTableSize
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
    unsigned int slotCount : 8;
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

extern LODTALK_VM_EXPORT UndefinedObject NilObject;
extern LODTALK_VM_EXPORT True TrueObject;
extern LODTALK_VM_EXPORT False FalseObject;

struct LODTALK_VM_EXPORT Oop
{
private:
	constexpr Oop(uint8_t *pointer) : pointer(pointer) {}
	constexpr Oop(intptr_t intValue) : intValue(intValue) {}
	constexpr Oop(int, uintptr_t uintValue) : uintValue(uintValue) {}

public:
	constexpr Oop() : pointer(reinterpret_cast<uint8_t*> (&NilObject)) {}

	static Oop fromPointer(void *pointer)
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

    inline bool isFloat() const
    {
        return isSmallFloat() || (isPointer() && header->classIndex == SCI_Float);
    }

    inline bool isFloatOrInt() const
    {
        return isSmallInteger() || isFloat();
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

    void *getFirstIndexableFieldPointer(VMContext *context) const
	{
		if(!isPointer())
			return nullptr;
		if(header->slotCount == 255)
			return pointer + sizeof(ObjectHeader) + 8;
		return pointer + sizeof(ObjectHeader) + getFixedSlotCount(context)*sizeof(void*);
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
		return int(intValue >> ObjectTag::CharacterShift);
	}

	static constexpr Oop encodeCharacter(int character)
	{
		return Oop((character << ObjectTag::CharacterShift) | ObjectTag::Character);
	}

    inline double decodeSmallFloat() const
    {
        LODTALK_UNIMPLEMENTED();
    }

    inline double decodeFloat() const
    {
        assert(isFloat());
        if (isSmallFloat())
            return decodeSmallFloat();
        return reinterpret_cast<double*> (&header[1])[0];
    }

    inline double decodeFloatOrInt() const
    {
        assert(isFloatOrInt());
        if (isSmallInteger())
            return (double)decodeSmallInteger();
        return decodeFloat();
    }

    static inline Oop encodeSmallFloat(double value)
    {
        LODTALK_UNIMPLEMENTED();
    }

    inline bool isExternalHandle() const
    {
        return isPointer() && header->classIndex == SCI_ExternalHandle;
    }

    inline bool isExternalPointer() const
    {
        return isPointer() && header->classIndex == SCI_ExternalPointer;
    }

    inline void *decodeExternalHandle()
    {
        assert(isExternalHandle());
        return *reinterpret_cast<void**> (getFirstFieldPointer());
    }

    inline void *decodeExternalPointer()
    {
        assert(isExternalPointer());
        return *reinterpret_cast<void**> (getFirstFieldPointer());
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

    size_t getFixedSlotCount(VMContext *context) const;

	inline size_t getNumberOfElements() const
	{
		auto slotCount = getSlotCount();
		auto format = header->objectFormat;
		if(format == OF_EMPTY)
			return 0;

		if(format < OF_INDEXABLE_64)
			return slotCount;

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
	}

    inline size_t getNumberOfVariableElements(VMContext *context) const
	{
        auto result = getNumberOfElements();
        if(header->objectFormat == OF_VARIABLE_SIZE_IVARS)
            return result - getFixedSlotCount(context);
        return result;
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
struct LODTALK_VM_EXPORT CompiledMethodHeader
{
	static const size_t LiteralMask = (1u<<16) -1;
	static const size_t LiteralShift = 1u;
	static const size_t HasPrimitiveBit = (1u<<17);
	static const size_t NeedsLargeFrameBit = (1u<<18);
	static const size_t TemporalShift = 19u;
	static const size_t TemporalMask = (1u<<6) - 1;
	static const size_t ArgumentShift = 25u;
	static const size_t ArgumentMask = (1u<<4) - 1;
	static const size_t ReservedBit = 1u<<29;
	static const size_t FlagBit = 1u<<30;
	static const size_t AlternateBytecodeBit = 1u<<31;

	constexpr CompiledMethodHeader(Oop oop) : oop(oop) {}

	static CompiledMethodHeader create(size_t literalCount, size_t temporalCount, size_t argumentCount, size_t extraFlags)
	{
		assert(literalCount <= LiteralMask);
		assert(temporalCount <= TemporalMask);
		assert(argumentCount <= ArgumentMask);

		return Oop::fromRawUIntPtr(1 |
		((literalCount & LiteralMask) << LiteralShift) |
		((temporalCount & TemporalMask) << TemporalShift) |
		((argumentCount & ArgumentMask) << ArgumentShift) |
        extraFlags);
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
		return (oop.uintValue | NeedsLargeFrameBit) != 0;
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

/**
 * Object header when the slot count is greater or equal to 255.
 */
struct BigObjectHeader
{
	ObjectHeader header;
	uint64_t slotCount;
};

class ClassDescription;

// Gets the identity hash of an Object
inline int identityHashOf(Oop obj)
{
	if(obj.isSmallInteger())
		return (int)obj.decodeSmallInteger();
    else if (obj.isCharacter())
        return obj.decodeCharacter();
    else if (obj.isSmallFloat())
        return int(obj.uintValue >> ObjectTag::SmallFloatShift);
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
class LODTALK_VM_EXPORT OopRef
{
public:
	OopRef()
        : context_(getCurrentContext())
	{
		registerSelf();
	}

	OopRef(const Oop &object)
		: oop(object), context_(getCurrentContext())
	{
		registerSelf();
	}

    OopRef(VMContext *context)
		: context_(context)
	{
		registerSelf();
	}

    OopRef(VMContext *context, const Oop &object)
		: oop(object), context_(context)
	{
		registerSelf();
	}

	OopRef(const OopRef &ref)
		: oop(ref.oop), context_(ref.context_)
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

	OopRef &operator=(const Oop &newOop)
	{
		this->oop = newOop;
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
    VMContext *context_;
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

    Ref(VMContext *context)
        : reference(context)
	{
	}

    Ref(VMContext *context, T* p)
        : reference(context)
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

    void reset(VMContext *context, T *pointer=(T*)&NilObject)
	{
		internalSet(context, pointer);
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

} // End of namespace Lodtalk

#endif //LODTALK_OBJECT_MODEL_HPP_
