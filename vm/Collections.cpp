#include <string.h>
#include <map>
#include <vector>
#include "Collections.hpp"
#include "Method.hpp"

namespace Lodtalk
{
// Collection
LODTALK_BEGIN_CLASS_SIDE_TABLE(Collection)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Collection)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Collection, Object, OF_EMPTY, 0);

// SequenceableCollection
LODTALK_BEGIN_CLASS_SIDE_TABLE(SequenceableCollection)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(SequenceableCollection)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(SequenceableCollection, Collection, OF_EMPTY, 0);

// ArrayedCollection
LODTALK_BEGIN_CLASS_SIDE_TABLE(ArrayedCollection)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ArrayedCollection)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(ArrayedCollection, SequenceableCollection, OF_EMPTY, 0);

// Array
Array *Array::basicNativeNew(size_t indexableSize)
{
	return reinterpret_cast<Array*> (newObject(0, indexableSize, OF_VARIABLE_SIZE_NO_IVARS, SCI_Array));
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Array)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Array)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Array, ArrayedCollection, OF_VARIABLE_SIZE_NO_IVARS, 0);

// ByteArray
LODTALK_BEGIN_CLASS_SIDE_TABLE(ByteArray)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ByteArray)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(ByteArray, ArrayedCollection, OF_INDEXABLE_8, 0);

// FloatArray
LODTALK_BEGIN_CLASS_SIDE_TABLE(FloatArray)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(FloatArray)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(FloatArray, ArrayedCollection, OF_INDEXABLE_32, 0);

// WordArray
LODTALK_BEGIN_CLASS_SIDE_TABLE(WordArray)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(WordArray)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(WordArray, ArrayedCollection, OF_INDEXABLE_32, 0);

// IntegerArray
LODTALK_BEGIN_CLASS_SIDE_TABLE(IntegerArray)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(IntegerArray)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(IntegerArray, ArrayedCollection, OF_INDEXABLE_32, 0);

// String
LODTALK_BEGIN_CLASS_SIDE_TABLE(String)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(String)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(String, ArrayedCollection, OF_EMPTY, 0);

// ByteString
ByteString *ByteString::basicNativeNew(size_t indexableSize)
{
	return reinterpret_cast<ByteString*> (newObject(0, indexableSize, OF_INDEXABLE_8, SCI_ByteString));
}

Ref<ByteString> ByteString::fromNative(const std::string &native)
{
	auto result = basicNativeNew(native.size());
	memcpy(result->getFirstFieldPointer(), native.data(), native.size());
	return Ref<ByteString> (reinterpret_cast<ByteString*> (result));
}

ByteString *ByteString::fromNativeRange(const char *start, size_t size)
{
	auto result = basicNativeNew(size);
	memcpy(result->getFirstFieldPointer(), start, size);
	return reinterpret_cast<ByteString*> (result);
}

ByteString *ByteString::fromNativeReverseRange(const char *start, ptrdiff_t size)
{
    auto result = basicNativeNew(size);
    auto dest = result->getFirstFieldPointer();
    for(ptrdiff_t i = 0; i < size; ++i)
        dest[i] = start[size - i - 1];
	return reinterpret_cast<ByteString*> (result);
}

std::string ByteString::getString()
{
	auto begin = getFirstFieldPointer();
	return std::string(begin, begin + getNumberOfElements());
}

Oop ByteString::splitVariableNames(const std::string &string)
{
	std::vector<std::pair<int, int>> varNameIndices;

	auto data = string.data();
	auto size = string.size();
	int tokenStart = -1;

	for(size_t i = 0; i < size; ++i)
	{
		auto c = data[i];
		if(tokenStart < 0)
		{
			// I am not in a token.
			if(c > ' ')
				tokenStart = i;
		}
		else
		{
			// Inside token.
			if(c <= ' ')
			{
				varNameIndices.push_back(std::make_pair(tokenStart, i - tokenStart));
				tokenStart = -1;
			}
		}
	}

	// Push the last token.
	if(tokenStart >= 0)
		varNameIndices.push_back(std::make_pair(tokenStart, size - tokenStart));

	// Allocate the result.
	Ref<Array> result = Array::basicNativeNew(varNameIndices.size());
	for(size_t i = 0; i < varNameIndices.size(); ++i)
	{
		auto &startSize = varNameIndices[i];
		auto subString = ByteSymbol::fromNativeRange((char*)data + startSize.first, startSize.second);
		auto resultData = reinterpret_cast<Oop*> (result->getFirstFieldPointer());
		resultData[i] = subString;
	}

	return result.getOop();
}

Oop ByteString::stSplitVariableNames()
{
	return splitVariableNames(getString());
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(ByteString)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ByteString)
    LODTALK_METHOD("splitForVariableNames", &ByteString::stSplitVariableNames)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(ByteString, String, OF_INDEXABLE_8, 0);

// WideString
LODTALK_BEGIN_CLASS_SIDE_TABLE(WideString)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(WideString)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(WideString, String, OF_INDEXABLE_32, 0);

// Symbol
std::string ByteSymbol::getString()
{
	auto begin = getFirstFieldPointer();
	return std::string(begin, begin + getNumberOfElements());
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(Symbol)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Symbol)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Symbol, String, OF_EMPTY, 0);

// ByteSymbol
LODTALK_BEGIN_CLASS_SIDE_TABLE(ByteSymbol)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(ByteSymbol)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(ByteSymbol, Symbol, OF_INDEXABLE_8, 0);

typedef std::map<std::string, OopRef > ByteSymbolDictionary;
static ByteSymbolDictionary *byteSymbolDictionary;

Object *ByteSymbol::basicNativeNew(size_t indexableSize)
{
	return reinterpret_cast<Object*> (newObject(0, indexableSize, OF_INDEXABLE_8, SCI_ByteSymbol));
}

Oop ByteSymbol::fromNative(const std::string &native)
{
	if(!byteSymbolDictionary)
		byteSymbolDictionary = new ByteSymbolDictionary();

	// Find existing internation
	auto it = byteSymbolDictionary->find(native);
	if(it != byteSymbolDictionary->end())
		return it->second.oop;

	// Create the byte symbol
	auto result = basicNativeNew(native.size());
	memcpy(result->getFirstFieldPointer(), native.data(), native.size());

	// Store in the internation dictionary.
    auto resultOop = Oop::fromPointer(result);
	(*byteSymbolDictionary)[native] = resultOop;
	return resultOop;
}

Oop ByteSymbol::fromNativeRange(const char *start, size_t size)
{
	return fromNative(std::string(start, start + size));
}

// WideSymbol
LODTALK_BEGIN_CLASS_SIDE_TABLE(WideSymbol)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(WideSymbol)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(WideSymbol, Symbol, OF_INDEXABLE_32, 0);

// HashedCollection
LODTALK_BEGIN_CLASS_SIDE_TABLE(HashedCollection)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(HashedCollection)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(HashedCollection, Collection, OF_FIXED_SIZE, 3,
"capacity tally keyValues");

// Dictionary
LODTALK_BEGIN_CLASS_SIDE_TABLE(Dictionary)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(Dictionary)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(Dictionary, HashedCollection, OF_FIXED_SIZE, 3);

// MethodDictionary
MethodDictionary* MethodDictionary::basicNativeNew()
{
	auto res = reinterpret_cast<MethodDictionary*> (newObject(4, 0, OF_FIXED_SIZE, SCI_MethodDictionary));
	res->capacityObject = Oop::encodeSmallInteger(0);
	res->tallyObject = Oop::encodeSmallInteger(0);
	return res;
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(MethodDictionary)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(MethodDictionary)
    LODTALK_METHOD("atOrNil:", &MethodDictionary::atOrNil)
    LODTALK_METHOD("at:put:", &MethodDictionary::atPut)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_INSTANCE_VARIABLES(MethodDictionary, Dictionary, OF_FIXED_SIZE, 4,
"values");

// IdentityDictionary
LODTALK_BEGIN_CLASS_SIDE_TABLE(IdentityDictionary)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(IdentityDictionary)
    LODTALK_METHOD("putAssociation:", &IdentityDictionary::putAssociation)
    LODTALK_METHOD("associationAtOrNil:", &IdentityDictionary::getAssociationOrNil)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(IdentityDictionary, Dictionary, OF_FIXED_SIZE, 3);

// SystemDictionary
SystemDictionary *SystemDictionary::create()
{
    auto res = reinterpret_cast<SystemDictionary*> (newObject(3, 0, OF_FIXED_SIZE, SCI_SystemDictionary));
    res->initialize();
    return res;
}
LODTALK_BEGIN_CLASS_SIDE_TABLE(SystemDictionary)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(SystemDictionary)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(SystemDictionary, IdentityDictionary, OF_FIXED_SIZE, 3);

} // End of namespace Lodtalk
