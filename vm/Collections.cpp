#include <string.h>
#include <map>
#include <vector>
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/Collections.hpp"
#include "Lodtalk/InterpreterProxy.hpp"
#include "MemoryManager.hpp"
#include "Method.hpp"

namespace Lodtalk
{
// Collection
SpecialNativeClassFactory Collection::Factory("Collection", SCI_Collection, &Object::Factory, [](ClassBuilder &builder) {
});

// SequenceableCollection
SpecialNativeClassFactory SequenceableCollection::Factory("SequenceableCollection", SCI_SequenceableCollection, &Collection::Factory, [](ClassBuilder &builder) {
});

// ArrayedCollection
SpecialNativeClassFactory ArrayedCollection::Factory("ArrayedCollection", SCI_ArrayedCollection, &SequenceableCollection::Factory, [](ClassBuilder &builder) {
});

// Array
Array *Array::basicNativeNew(VMContext *context, size_t indexableSize)
{
	return reinterpret_cast<Array*> (context->newObject(0, indexableSize, OF_VARIABLE_SIZE_NO_IVARS, SCI_Array));
}

SpecialNativeClassFactory Array::Factory("Array", SCI_Array, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableSizeWithoutInstanceVariables();
});

// ByteArray
SpecialNativeClassFactory ByteArray::Factory("ByteArray", SCI_ByteArray, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// FloatArray
SpecialNativeClassFactory FloatArray::Factory("FloatArray", SCI_FloatArray, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits32();
});

// WordArray
SpecialNativeClassFactory WordArray::Factory("WordArray", SCI_WordArray, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits32();
});

// IntegerArray
SpecialNativeClassFactory IntegerArray::Factory("IntegerArray", SCI_IntegerArray, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits32();
});

// String
SpecialNativeClassFactory String::Factory("String", SCI_String, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
});

// ByteString
ByteString *ByteString::basicNativeNew(VMContext *context, size_t indexableSize)
{
	return reinterpret_cast<ByteString*> (context->newObject(0, indexableSize, OF_INDEXABLE_8, SCI_ByteString));
}

Ref<ByteString> ByteString::fromNative(VMContext *context, const std::string &native)
{
	auto result = basicNativeNew(context, native.size());
	memcpy(result->getFirstFieldPointer(), native.data(), native.size());
	return Ref<ByteString> (context, reinterpret_cast<ByteString*> (result));
}

ByteString *ByteString::fromNativeRange(VMContext *context, const char *start, size_t size)
{
	auto result = basicNativeNew(context, size);
	memcpy(result->getFirstFieldPointer(), start, size);
	return reinterpret_cast<ByteString*> (result);
}

ByteString *ByteString::fromNativeReverseRange(VMContext *context, const char *start, ptrdiff_t size)
{
    auto result = basicNativeNew(context, size);
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

Oop ByteString::splitVariableNames(VMContext *context, const std::string &string)
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
				tokenStart = (int)i;
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
	Ref<Array> result(context, Array::basicNativeNew(context, varNameIndices.size()));
	for(size_t i = 0; i < varNameIndices.size(); ++i)
	{
		auto &startSize = varNameIndices[i];
		auto subString = ByteSymbol::fromNativeRange(context, (char*)data + startSize.first, startSize.second);
		auto resultData = reinterpret_cast<Oop*> (result->getFirstFieldPointer());
		resultData[i] = subString;
	}

	return result.getOop();
}

int ByteString::stSplitVariableNames(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    Oop self = interpreter->getReceiver();
    Oop result = splitVariableNames(interpreter->getContext(), reinterpret_cast<ByteString*> (self.pointer)->getString());
    return interpreter->returnOop(result);
}

SpecialNativeClassFactory ByteString::Factory("ByteString", SCI_ByteString, &String::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();

    builder
        .addMethod("splitForVariableNames", &ByteString::stSplitVariableNames);
});

// WideString
SpecialNativeClassFactory WideString::Factory("WideString", SCI_WideString, &ArrayedCollection::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits32();
});

// Symbol
SpecialNativeClassFactory Symbol::Factory("Symbol", SCI_Symbol, &String::Factory, [](ClassBuilder &builder) {
});

// ByteSymbol
std::string ByteSymbol::getString()
{
	auto begin = getFirstFieldPointer();
	return std::string(begin, begin + getNumberOfElements());
}

Object *ByteSymbol::basicNativeNew(VMContext *context, size_t indexableSize)
{
	return reinterpret_cast<Object*> (context->newObject(0, indexableSize, OF_INDEXABLE_8, SCI_ByteSymbol));
}

Oop ByteSymbol::fromNative(VMContext *context, const std::string &native)
{
	// Find existing internation
    auto &dict = context->getMemoryManager()->getSymbolDictionary();
	auto it = dict.find(native);
	if(it != dict.end())
		return it->second;

	// Create the byte symbol
	auto result = basicNativeNew(context, native.size());
	memcpy(result->getFirstFieldPointer(), native.data(), native.size());

	// Store in the internation dictionary.
    auto resultOop = Oop::fromPointer(result);
	dict[native] = resultOop;
	return resultOop;
}

Oop ByteSymbol::fromNativeRange(VMContext *context, const char *start, size_t size)
{
	return fromNative(context, std::string(start, start + size));
}

SpecialNativeClassFactory ByteSymbol::Factory("ByteSymbol", SCI_ByteSymbol, &Symbol::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// WideSymbol
SpecialNativeClassFactory WideSymbol::Factory("WideSymbol", SCI_WideSymbol, &Symbol::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits32();
});

// HashedCollection
SpecialNativeClassFactory HashedCollection::Factory("HashedCollection", SCI_HashedCollection, &Collection::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("capacity", "tally", "keyValues");
});

// Dictionary
SpecialNativeClassFactory Dictionary::Factory("Dictionary", SCI_Dictionary, &HashedCollection::Factory, [](ClassBuilder &builder) {
});

// MethodDictionary
MethodDictionary* MethodDictionary::basicNativeNew(VMContext *context)
{
	auto res = reinterpret_cast<MethodDictionary*> (context->newObject(4, 0, OF_FIXED_SIZE, SCI_MethodDictionary));
	res->capacityObject = Oop::encodeSmallInteger(0);
	res->tallyObject = Oop::encodeSmallInteger(0);
	return res;
}

int MethodDictionary::stAtOrNil(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    Oop selfOop = interpreter->getReceiver();
    Oop key = interpreter->getTemporary(0);
    auto self = reinterpret_cast<MethodDictionary*> (selfOop.pointer);
    return interpreter->returnOop(self->atOrNil(key));
}

int MethodDictionary::stAtPut(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 2)
        return interpreter->primitiveFailed();

    Oop selfOop = interpreter->getReceiver();
    Oop key = interpreter->getTemporary(0);
    Oop value = interpreter->getTemporary(1);
    auto self = reinterpret_cast<MethodDictionary*> (selfOop.pointer);
    return interpreter->returnOop(self->atPut(interpreter->getContext(), key, value));
}

SpecialNativeClassFactory MethodDictionary::Factory("MethodDictionary", SCI_MethodDictionary, &Dictionary::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("values");

    builder
        .addMethod("atOrNil:", MethodDictionary::stAtOrNil)
        .addMethod("at:put:", MethodDictionary::stAtPut);
});

// IdentityDictionary
int IdentityDictionary::stPutAssociation(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    Oop selfOop = interpreter->getReceiver();
    Oop association = interpreter->getTemporary(0);
    auto self = reinterpret_cast<IdentityDictionary*> (selfOop.pointer);
    self->putAssociation(interpreter->getContext(), association);
    return interpreter->returnOop(interpreter->getTemporary(0));
}

int IdentityDictionary::stAssociationAtOrNil(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 1)
        return interpreter->primitiveFailed();

    Oop selfOop = interpreter->getReceiver();
    Oop key = interpreter->getTemporary(0);
    auto self = reinterpret_cast<IdentityDictionary*> (selfOop.pointer);
    return interpreter->returnOop(self->getAssociationOrNil(key));
}

SpecialNativeClassFactory IdentityDictionary::Factory("IdentityDictionary", SCI_IdentityDictionary, &Dictionary::Factory, [](ClassBuilder &builder) {
    builder
        .addMethod("putAssociation:", IdentityDictionary::stPutAssociation)
        .addMethod("associationAtOrNil:", IdentityDictionary::stAssociationAtOrNil);
});

// SystemDictionary
SystemDictionary *SystemDictionary::create(VMContext *context)
{
    auto res = reinterpret_cast<SystemDictionary*> (context->newObject(3, 0, OF_FIXED_SIZE, SCI_SystemDictionary));
    res->initialize();
    return res;
}

SpecialNativeClassFactory SystemDictionary::Factory("SystemDictionary", SCI_SystemDictionary, &IdentityDictionary::Factory, [](ClassBuilder &builder) {
});

} // End of namespace Lodtalk
