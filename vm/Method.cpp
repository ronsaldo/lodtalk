#include "Lodtalk/Object.hpp"
#include "Lodtalk/Collections.hpp"
#include "Method.hpp"
#include "StackInterpreter.hpp"
#include "BytecodeSets.hpp"

namespace Lodtalk
{

// AdditionalMethodState
AdditionalMethodState *AdditionalMethodState::basicNew(VMContext *context, size_t size)
{
    return reinterpret_cast<AdditionalMethodState*> (context->newObject(2, size, OF_VARIABLE_SIZE_IVARS, SCI_AdditionalMethodState));
}

SpecialNativeClassFactory AdditionalMethodState::Factory("AdditionalMethodState", SCI_AdditionalMethodState, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .variableSizeWithInstanceVariables()
        .addInstanceVariables("selector", "method");

});

// CompiledMethod
CompiledMethod *CompiledMethod::newMethodWithHeader(VMContext *context, size_t numberOfBytes, CompiledMethodHeader header)
{
	// Add the method header size
	numberOfBytes += sizeof(void*);

	// Compute the some masks
	const size_t wordSize = sizeof(void*);
	const size_t wordMask = wordSize -1;

	// Compute the number of slots
	size_t slotCount = ((numberOfBytes + wordSize - 1) & (~wordMask)) / wordSize;
	size_t extraFormatBits = numberOfBytes & wordMask;
	assert(slotCount*wordSize >= numberOfBytes);

	// Compute the header size.
	auto headerSize = sizeof(ObjectHeader);
	if(slotCount >= 255)
		headerSize += 8;
	auto bodySize = slotCount*sizeof(void*);
	auto objectSize = headerSize + bodySize;

	// Allocate the compiled method
	auto methodData = context->allocateObjectMemory(objectSize);
	auto objectHeader = reinterpret_cast<ObjectHeader*> (methodData);

	// Set the method header
	objectHeader->slotCount = slotCount < 255 ? slotCount : 255;
	objectHeader->identityHash = generateIdentityHash(methodData);
	objectHeader->objectFormat = (unsigned int)(OF_COMPILED_METHOD + extraFormatBits);
	objectHeader->classIndex = SCI_CompiledMethod;
	if(slotCount >= 255)
	{
		auto bigHeader = reinterpret_cast<BigObjectHeader*> (objectHeader);
		bigHeader->slotCount = slotCount;
	}

	// Initialize the literals to nil
	auto methodBody = reinterpret_cast<Oop*> (methodData + headerSize);
	auto literalCount = header.getLiteralCount();
	for(size_t i = 0; i < literalCount + 1; ++i )
		methodBody[i] = nilOop();

	// Set the method header
	auto methodHeader = reinterpret_cast<CompiledMethodHeader*> (methodBody);
	*methodHeader = header;

	// Return the compiled method.
	return reinterpret_cast<CompiledMethod*> (methodData);
}

Oop CompiledMethod::dump()
{
    dumpSistaBytecode(getFirstBCPointer(), getByteDataSize());
    return selfOop();
}

int CompiledMethod::stNewMethodWithHeader(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 2)
        return interpreter->primitiveFailed();

    auto sizeOop = interpreter->getTemporary(0);
    CompiledMethodHeader header = interpreter->getTemporary(1);
    auto method = newMethodWithHeader(interpreter->getContext(), sizeOop.decodeSmallInteger(), header);
    return interpreter->returnOop(Oop::fromPointer(method));
}

int CompiledMethod::stDump(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 0)
        return interpreter->primitiveFailed();

    auto selfOop = interpreter->getReceiver();
    auto self = reinterpret_cast<CompiledMethod*> (selfOop.pointer);
    self->dump();
    return interpreter->returnReceiver();
}

SpecialNativeClassFactory CompiledMethod::Factory("CompiledMethod", SCI_CompiledMethod, &ByteArray::Factory, [](ClassBuilder &builder) {
    builder
        .compiledMethodFormat();

    builder
        .addPrimitiveClassMethod(79, "newMethod:header:", CompiledMethod::stNewMethodWithHeader);

    builder
        .addMethod("dump", CompiledMethod::stDump);
});

// NativeMethod
NativeMethod *NativeMethod::create(VMContext *context, PrimitiveFunction primitive)
{
    auto result = reinterpret_cast<NativeMethod*> (context->newObject(0, sizeof(primitive), OF_INDEXABLE_8, SCI_NativeMethod));
    result->primitive = primitive;
    return result;
}

SpecialNativeClassFactory NativeMethod::Factory("NativeMethod", SCI_NativeMethod, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .variableBits8();
});

// InstructionStream
SpecialNativeClassFactory InstructionStream::Factory("InstructionStream", SCI_InstructionStream, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("sender", "pc");
});

// Context
SpecialNativeClassFactory Context::Factory("Context", SCI_Context, &InstructionStream::Factory, [](ClassBuilder &builder) {
    builder
        .variableSizeWithInstanceVariables()
        .addInstanceVariables("stackp", "method", "closureOrNil", "receiver");
});

Context *Context::create(VMContext *context, size_t slotCount)
{
    return reinterpret_cast<Context*> (context->newObject(ContextVariableCount, slotCount, OF_VARIABLE_SIZE_IVARS, SCI_Context));
}

// BlockClosure
BlockClosure *BlockClosure::create(VMContext *context, int numcopied)
{
    return reinterpret_cast<BlockClosure*> (context->newObject(BlockClosureVariableCount, numcopied, OF_VARIABLE_SIZE_IVARS, SCI_BlockClosure));
}

SpecialNativeClassFactory BlockClosure::Factory("BlockClosure", SCI_BlockClosure, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .variableSizeWithInstanceVariables()
        .addInstanceVariables("outerContext", "startpc", "numArgs");
});

// Message
Message *Message::create(VMContext *context)
{
    return reinterpret_cast<Message*> (context->newObject(MessageVariableCount, 0, OF_FIXED_SIZE, SCI_Message));
}

SpecialNativeClassFactory Message::Factory("Message", SCI_Message, &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addInstanceVariables("selector", "args", "lookupClass");
});

} // End of namespace Lodtalk
