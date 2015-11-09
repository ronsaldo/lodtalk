#ifndef LODTALK_COLLECTIONS_HPP_
#define LODTALK_COLLECTIONS_HPP_

#include <stddef.h>
#include <string>
#include "Lodtalk/Object.hpp"

namespace Lodtalk
{

/**
 * Collection
 */
class LODTALK_VM_EXPORT Collection: public Object
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * SequenceableCollection
 */
class LODTALK_VM_EXPORT SequenceableCollection: public Collection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * ArrayedCollection
 */
class LODTALK_VM_EXPORT ArrayedCollection: public SequenceableCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * Array
 */
class LODTALK_VM_EXPORT Array: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

	static Array *basicNativeNew(VMContext *context, size_t indexableSize);
};

/**
 * ByteArray
 */
class LODTALK_VM_EXPORT ByteArray: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * FloatArray
 */
class LODTALK_VM_EXPORT FloatArray: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * WordArray
 */
class LODTALK_VM_EXPORT WordArray: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * IntegerArray
 */
class LODTALK_VM_EXPORT IntegerArray: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * String
 */
class LODTALK_VM_EXPORT String: public ArrayedCollection
{
public:
    static SpecialNativeClassFactory Factory;

};

/**
 * ByteString
 */
class LODTALK_VM_EXPORT ByteString: public String
{
public:
    static SpecialNativeClassFactory Factory;

	static ByteString *basicNativeNew(VMContext *context, size_t indexableSize);

	static ByteString *fromNativeRange(VMContext *context, const char *start, size_t size);
    static ByteString *fromNativeReverseRange(VMContext *context, const char *start, ptrdiff_t size);
	static Ref<ByteString> fromNative(VMContext *context, const std::string &native);

	static Oop splitVariableNames(VMContext *context, const std::string &string);

	std::string getString();

    static int stSplitVariableNames(InterpreterProxy *interpreter);
};

/**
 * WideString
 */
class LODTALK_VM_EXPORT WideString: public String
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * Symbol
 */
class LODTALK_VM_EXPORT Symbol: public String
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * ByteSymbol
 */
class LODTALK_VM_EXPORT ByteSymbol: public Symbol
{
public:
    static SpecialNativeClassFactory Factory;

	static Object *basicNativeNew(VMContext *context, size_t indexableSize);

	static Oop fromNative(VMContext *context, const std::string &native);
	static Oop fromNativeRange(VMContext *context, const char *star, size_t size);

	std::string getString();
};

/**
 * WideSymbol
 */
class LODTALK_VM_EXPORT WideSymbol: public Symbol
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * HashedCollection
 */
class LODTALK_VM_EXPORT HashedCollection: public Collection
{
public:
    static SpecialNativeClassFactory Factory;

    void initialize()
    {
        capacityObject = Oop::encodeSmallInteger(0);
        tallyObject = Oop::encodeSmallInteger(0);
        keyValues = (Array*)&NilObject;
    }

	size_t getCapacity() const
	{
		return capacityObject.decodeSmallInteger();
	}

	size_t getTally() const
	{
		return tallyObject.decodeSmallInteger();
	}

	Oop *getHashTableKeyValues() const
	{
		assert(keyValues);
		return reinterpret_cast<Oop*> (keyValues->getFirstFieldPointer());
	}

protected:
	void setKeyCapacity(VMContext *context, size_t keyCapacity)
	{
		keyValues = Array::basicNativeNew(context, keyCapacity);
	}

	template<typename KF, typename HF, typename EF>
	ptrdiff_t findKeyPosition(Oop key, const KF &keyFunction, const HF &hashFunction, const EF &equals)
	{
		auto capacity = getCapacity();
		if(capacity == 0)
			return -1;

		auto keyHash = hashFunction(key);
		auto startPosition = keyHash % capacity;
		auto keyValuesArray = getHashTableKeyValues();

		// Search from the hash position to the end.
		for(auto i = startPosition; i < capacity; ++i)
		{
			// Check no association
			auto keyValue = keyValuesArray[i];
			if(isNil(keyValue))
				return i;

			// Check the key
			auto slotKey = keyFunction(keyValue);
			if(equals(key, slotKey))
				return i;
		}

		// Search from the start to the hash position.
		for(auto i = 0; i < startPosition; ++i)
		{
			// Check no association
			auto keyValue = keyValuesArray[i];
			if(isNil(keyValue))
				return i;

			// Check the key
			auto slotKey = keyFunction(keyValuesArray[i]);
			if(equals(key, slotKey))
				return i;
		}

		// Not found.
		return -1;
	}

	template<typename KF, typename HF, typename EF>
	void increaseSize(VMContext *context, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		tallyObject.intValue += 2;

		// Do not use more than 80% of the capacity
		if(tallyObject.intValue > capacityObject.intValue*4/5)
			increaseCapacity(context, keyFunction, hashFunction, equalityFunction);
	}

	template<typename KF, typename HF, typename EF>
	void increaseCapacity(VMContext *context, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		size_t newCapacity = getCapacity()*2;
		if(!newCapacity)
			newCapacity = 16;

		setCapacity(context, newCapacity, keyFunction, hashFunction, equalityFunction);
	}

	template<typename KF, typename HF, typename EF>
	Oop internalKeyValueAtOrNil(Oop key, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(key, keyFunction, hashFunction, equalityFunction);
		if(position < 0)
			return nilOop();

		auto oldKeyValue = getHashTableKeyValues()[position];
		if(isNil(oldKeyValue))
			return nilOop();
		return oldKeyValue;
	}

	template<typename KF, typename HF, typename EF>
	void internalPutKeyValue(VMContext *context, Oop keyValue, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(keyFunction(keyValue), keyFunction, hashFunction, equalityFunction);
		if(position < 0)
		{
            OopRef keyValueRef(context, keyValue);
			increaseCapacity(context, keyFunction, hashFunction, equalityFunction);
			return internalPutKeyValue(context, keyValueRef.oop, keyFunction, hashFunction, equalityFunction);
		}

		// Put the key and value.
		auto keyValueArray = getHashTableKeyValues();
		auto oldKeyValue = keyValueArray[position];
		keyValueArray[position] = keyValue;

		// Increase the size.
		if(isNil(oldKeyValue))
			increaseSize(context, keyFunction, hashFunction, equalityFunction);
	}

	template<typename KF, typename HF, typename EF>
	void setCapacity(VMContext *context, size_t newCapacity, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		// Store temporarily the data.
		Ref<Array> oldKeyValues(context, keyValues);
		size_t oldCapacity = capacityObject.decodeSmallInteger();

		// Create the new capacity.
		capacityObject = Oop::encodeSmallInteger(newCapacity);
		tallyObject = Oop::encodeSmallInteger(0);
		setKeyCapacity(context, newCapacity);

		// Add back the old objects.
		if(!oldKeyValues.isNil())
		{
			Oop *oldKeyValuesOops = reinterpret_cast<Oop *> (oldKeyValues->getFirstFieldPointer());
			for(size_t i = 0; i < oldCapacity; ++i)
			{
				auto oldKeyValue = oldKeyValuesOops[i];
				if(!isNil(oldKeyValue))
					internalPutKeyValue(context, oldKeyValue, keyFunction, hashFunction, equalityFunction);
			}
		}
	}

	Oop capacityObject;
	Oop tallyObject;
	Array* keyValues;
};

/**
 * Dictionary
 */
class LODTALK_VM_EXPORT Dictionary: public HashedCollection
{
public:
    static SpecialNativeClassFactory Factory;
};

/**
 * MethodDictionary
 */
class LODTALK_VM_EXPORT MethodDictionary: public Dictionary
{
public:
    static SpecialNativeClassFactory Factory;

	static MethodDictionary* basicNativeNew(VMContext *context);

	Oop atOrNil(Oop key)
	{
		return internalAtOrNil(key);
	}

	Oop atPut(VMContext *context, Oop key, Oop value)
	{
		internalAtPut(context, key, value);
		return value;
	}

	Oop *getHashTableKeys() const
	{
		assert(keyValues);
		return reinterpret_cast<Oop*> (keyValues->getFirstFieldPointer());
	}

	Oop *getHashTableValues() const
	{
		assert(values);
		return reinterpret_cast<Oop*> (values->getFirstFieldPointer());
	}

    static int stAtOrNil(InterpreterProxy *interpreter);
    static int stAtPut(InterpreterProxy *interpreter);

protected:
	MethodDictionary()
	{
		object_header_ = ObjectHeader::specialNativeClass(generateIdentityHash(this), SCI_MethodDictionary, 4);
		values = (Array*)&NilObject;
	}

	void increaseSize(VMContext *context)
	{
		tallyObject.intValue += 2;

		// Do not use more than 80% of the capacity
		if(tallyObject.intValue > capacityObject.intValue*4/5)
			increaseCapacity(context);
	}

	void increaseCapacity(VMContext *context)
	{
		size_t newCapacity = getCapacity()*2;
		if(!newCapacity)
			newCapacity = 16;

		setCapacity(context, newCapacity);
	}

	Oop internalAtOrNil(Oop key)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(key, identityFunction<Oop>, identityHashOf, identityOopEquals);
		if(position < 0)
			return nilOop();

		auto oldKey = getHashTableKeys()[position];
		if(isNil(oldKey))
			return nilOop();
		return getHashTableValues()[position];
	}

	void internalAtPut(VMContext *context, Oop key, Oop value)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(key, identityFunction<Oop>, identityHashOf, identityOopEquals);
		if(position < 0)
		{
            OopRef keyRef(context, key);
            OopRef valueRef(context, value);
			increaseCapacity(context);
            return internalAtPut(context, keyRef.oop, valueRef.oop);
		}

		// Put the key and value.
		auto keyArray = getHashTableKeys();
		auto valueArray = getHashTableValues();
		auto oldKey = keyArray[position];
		keyArray[position] = key;
		valueArray[position] = value;

		// Increase the size.
		if(isNil(oldKey))
			increaseSize(context);
	}

	void setCapacity(VMContext *context, size_t newCapacity)
	{
		// Store temporarily the data.
		Ref<Array> oldKeys(context, keyValues);
		Ref<Array> oldValues(context, values);
		size_t oldCapacity = capacityObject.decodeSmallInteger();

		// Create the new capacity.
		capacityObject = Oop::encodeSmallInteger(newCapacity);
		tallyObject = Oop::encodeSmallInteger(0);
		setKeyCapacity(context, newCapacity);
		setValueCapacity(context, newCapacity);

		// Add back the old objects.
		if(!oldKeys.isNil())
		{
			Oop *oldKeysOops = reinterpret_cast<Oop *> (oldKeys->getFirstFieldPointer());
			Oop *oldValuesOops = reinterpret_cast<Oop *> (oldValues->getFirstFieldPointer());
			for(size_t i = 0; i < oldCapacity; ++i)
			{
				auto oldKey = oldKeysOops[i];
				if(!isNil(oldKey))
					internalAtPut(context, oldKey, oldValuesOops[i]);
			}
		}
	}

	void setValueCapacity(VMContext *context, size_t valueCapacity)
	{
		values = Array::basicNativeNew(context, valueCapacity);
	}

	Array* values;
};

/**
 * IdentityDictionary
 */
class LODTALK_VM_EXPORT IdentityDictionary: public Dictionary
{
public:
    static SpecialNativeClassFactory Factory;

	struct End {};

	Oop putAssociation(VMContext *context, Oop assoc)
	{
		internalPutKeyValue(context, assoc, getLookupKeyKey, identityHashOf, identityOopEquals);
        return assoc;
	}

	Oop getAssociationOrNil(Oop key)
	{
		return internalKeyValueAtOrNil(key, getLookupKeyKey, identityHashOf, identityOopEquals);
	}

	void putNativeAssociation(VMContext *context, Association *assoc)
	{
		putAssociation(context, Oop::fromPointer(assoc));
	}

	Association *getNativeAssociationOrNil(Oop key)
	{
		return reinterpret_cast<Association *> (getAssociationOrNil(key).pointer);
	}

    static int stPutAssociation(InterpreterProxy *interpreter);
    static int stAssociationAtOrNil(InterpreterProxy *interpreter);
};

/**
 * SystemDictionary
 */
class LODTALK_VM_EXPORT SystemDictionary: public IdentityDictionary
{
public:
    static SpecialNativeClassFactory Factory;

	struct End {};

    static SystemDictionary *create(VMContext *context);
};

} // End of namespace Lodtalk

#endif //LODTALK_COLLECTIONS_HPP_
