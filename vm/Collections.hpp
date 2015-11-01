#ifndef LODTALK_COLLECTIONS_HPP_
#define LODTALK_COLLECTIONS_HPP_

#include <stddef.h>
#include <string>
#include "Object.hpp"

namespace Lodtalk
{

/**
 * Collection
 */
class Collection: public Object
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * SequenceableCollection
 */
class SequenceableCollection: public Collection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * ArrayedCollection
 */
class ArrayedCollection: public SequenceableCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * Array
 */
class Array: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
	static Array *basicNativeNew(size_t indexableSize);
};

/**
 * ByteArray
 */
class ByteArray: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * FloatArray
 */
class FloatArray: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * WordArray
 */
class WordArray: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * IntegerArray
 */
class IntegerArray: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * String
 */
class String: public ArrayedCollection
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * ByteString
 */
class ByteString: public String
{
	LODTALK_NATIVE_CLASS();
public:
	static ByteString *basicNativeNew(size_t indexableSize);

	static ByteString *fromNativeRange(const char *start, size_t size);
    static ByteString *fromNativeReverseRange(const char *start, ptrdiff_t size);
	static Ref<ByteString> fromNative(const std::string &native);

	Oop stSplitVariableNames();
	static Oop splitVariableNames(const std::string &string);

	std::string getString();
};

/**
 * WideString
 */
class WideString: public String
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * Symbol
 */
class Symbol: public String
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * ByteSymbol
 */
class ByteSymbol: public Symbol
{
	LODTALK_NATIVE_CLASS();
public:
	static Object *basicNativeNew(size_t indexableSize);

	static Oop fromNative(const std::string &native);
	static Oop fromNativeRange(const char *star, size_t size);

	std::string getString();
};

/**
 * WideSymbol
 */
class WideSymbol: public Symbol
{
	LODTALK_NATIVE_CLASS();
public:
};

/**
 * HashedCollection
 */
class HashedCollection: public Collection
{
	LODTALK_NATIVE_CLASS();
public:
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
	void setKeyCapacity(size_t keyCapacity)
	{
		keyValues = Array::basicNativeNew(keyCapacity);
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
	void increaseSize(const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		tallyObject.intValue += 2;

		// Do not use more than 80% of the capacity
		if(tallyObject.intValue > capacityObject.intValue*4/5)
			increaseCapacity(keyFunction, hashFunction, equalityFunction);
	}

	template<typename KF, typename HF, typename EF>
	void increaseCapacity(const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		size_t newCapacity = getCapacity()*2;
		if(!newCapacity)
			newCapacity = 16;

		setCapacity(newCapacity, keyFunction, hashFunction, equalityFunction);
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
	void internalPutKeyValue(Oop keyValue, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(keyFunction(keyValue), keyFunction, hashFunction, equalityFunction);
		if(position < 0)
		{
            OopRef keyValueRef = keyValue;
			increaseCapacity(keyFunction, hashFunction, equalityFunction);
			return internalPutKeyValue(keyValueRef.oop, keyFunction, hashFunction, equalityFunction);
		}

		// Put the key and value.
		auto keyValueArray = getHashTableKeyValues();
		auto oldKeyValue = keyValueArray[position];
		keyValueArray[position] = keyValue;

		// Increase the size.
		if(isNil(oldKeyValue))
			increaseSize(keyFunction, hashFunction, equalityFunction);
	}

	template<typename KF, typename HF, typename EF>
	void setCapacity(size_t newCapacity, const KF &keyFunction, const HF &hashFunction, const EF &equalityFunction)
	{
		// Store temporarily the data.
		Ref<Array> oldKeyValues(keyValues);
		size_t oldCapacity = capacityObject.decodeSmallInteger();

		// Create the new capacity.
		capacityObject = Oop::encodeSmallInteger(newCapacity);
		tallyObject = Oop::encodeSmallInteger(0);
		setKeyCapacity(newCapacity);

		// Add back the old objects.
		if(!oldKeyValues.isNil())
		{
			Oop *oldKeyValuesOops = reinterpret_cast<Oop *> (oldKeyValues->getFirstFieldPointer());
			for(size_t i = 0; i < oldCapacity; ++i)
			{
				auto oldKeyValue = oldKeyValuesOops[i];
				if(!isNil(oldKeyValue))
					internalPutKeyValue(oldKeyValue, keyFunction, hashFunction, equalityFunction);
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
class Dictionary: public HashedCollection
{
	LODTALK_NATIVE_CLASS();
};

/**
 * MethodDictionary
 */
class MethodDictionary: public Dictionary
{
	LODTALK_NATIVE_CLASS();
public:
	static MethodDictionary* basicNativeNew();

	template<typename T>
	void addMethod(const T &methodDescriptor)
	{
		atPut(methodDescriptor.getSelector(), methodDescriptor.getMethod());
	}

	Oop atOrNil(Oop key)
	{
		return internalAtOrNil(key);
	}

	Oop atPut(Oop key, Oop value)
	{
		internalAtPut(key, value);
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

protected:
	MethodDictionary()
	{
		object_header_ = ObjectHeader::specialNativeClass(generateIdentityHash(this), SCI_MethodDictionary, 4);
		values = (Array*)&NilObject;
	}

	void increaseSize()
	{
		tallyObject.intValue += 2;

		// Do not use more than 80% of the capacity
		if(tallyObject.intValue > capacityObject.intValue*4/5)
			increaseCapacity();
	}

	void increaseCapacity()
	{
		size_t newCapacity = getCapacity()*2;
		if(!newCapacity)
			newCapacity = 16;

		setCapacity(newCapacity);
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

	void internalAtPut(Oop key, Oop value)
	{
		// If a slot was not found, try to increase the capacity.
		auto position = findKeyPosition(key, identityFunction<Oop>, identityHashOf, identityOopEquals);
		if(position < 0)
		{
            OopRef keyRef = key;
            OopRef valueRef = value;
			increaseCapacity();
            return internalAtPut(keyRef.oop, valueRef.oop);
		}

		// Put the key and value.
		auto keyArray = getHashTableKeys();
		auto valueArray = getHashTableValues();
		auto oldKey = keyArray[position];
		keyArray[position] = key;
		valueArray[position] = value;

		// Increase the size.
		if(isNil(oldKey))
			increaseSize();
	}

	void setCapacity(size_t newCapacity)
	{
		// Store temporarily the data.
		Ref<Array> oldKeys(keyValues);
		Ref<Array> oldValues(values);
		size_t oldCapacity = capacityObject.decodeSmallInteger();

		// Create the new capacity.
		capacityObject = Oop::encodeSmallInteger(newCapacity);
		tallyObject = Oop::encodeSmallInteger(0);
		setKeyCapacity(newCapacity);
		setValueCapacity(newCapacity);

		// Add back the old objects.
		if(!oldKeys.isNil())
		{
			Oop *oldKeysOops = reinterpret_cast<Oop *> (oldKeys->getFirstFieldPointer());
			Oop *oldValuesOops = reinterpret_cast<Oop *> (oldValues->getFirstFieldPointer());
			for(size_t i = 0; i < oldCapacity; ++i)
			{
				auto oldKey = oldKeysOops[i];
				if(!isNil(oldKey))
					internalAtPut(oldKey, oldValuesOops[i]);
			}
		}
	}

	void setValueCapacity(size_t valueCapacity)
	{
		values = Array::basicNativeNew(valueCapacity);
	}

	Array* values;
};

/**
 * IdentityDictionary
 */
class IdentityDictionary: public Dictionary
{
	LODTALK_NATIVE_CLASS();
public:
	struct End {};

	Oop putAssociation(Oop assoc)
	{
		internalPutKeyValue(assoc, getLookupKeyKey, identityHashOf, identityOopEquals);
        return assoc;
	}

	Oop getAssociationOrNil(Oop key)
	{
		return internalKeyValueAtOrNil(key, getLookupKeyKey, identityHashOf, identityOopEquals);
	}

	void putNativeAssociation(Association *assoc)
	{
		putAssociation(Oop::fromPointer(assoc));
	}

	Association *getNativeAssociationOrNil(Oop key)
	{
		return reinterpret_cast<Association *> (getAssociationOrNil(key).pointer);
	}
};

/**
 * SystemDictionary
 */
class SystemDictionary: public IdentityDictionary
{
	LODTALK_NATIVE_CLASS();
public:
	struct End {};

    static SystemDictionary *create();
};

} // End of namespace Lodtalk

#endif //LODTALK_COLLECTIONS_HPP_
