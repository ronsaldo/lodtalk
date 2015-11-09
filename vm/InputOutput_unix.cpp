#if defined(__linux__)
#include <unistd.h>
#include <stdio.h>
#include "Lodtalk/InterpreterProxy.hpp"
#include "InputOutput.hpp"
#include "Method.hpp"

namespace Lodtalk
{

int OSIO::stStdout(InterpreterProxy *interpreter)
{
    interpreter->pushSmallInteger(STDOUT_FILENO);
	return interpreter->returnTop();
}

int OSIO::stStdin(InterpreterProxy *interpreter)
{
    interpreter->pushSmallInteger(STDOUT_FILENO);
	return interpreter->returnTop();
}

int OSIO::stStderr(InterpreterProxy *interpreter)
{
    interpreter->pushSmallInteger(STDOUT_FILENO);
	return interpreter->returnTop();
}

// Oop clazz, Oop bufferOop, Oop offsetOop, Oop sizeOop, Oop fileOop
int OSIO::stWriteOffsetSizeTo(InterpreterProxy *interpreter)
{
    LODTALK_UNIMPLEMENTED();
    /*
	// Check the arguments.
	if(!bufferOop.isIndexableNativeData() || !offsetOop.isSmallInteger() || !sizeOop.isSmallInteger() || !fileOop.isSmallInteger())
		return Oop::encodeSmallInteger(-1);

	// Decode the arguments
	auto buffer = reinterpret_cast<uint8_t *> (bufferOop.getFirstFieldPointer());
	auto offset = offsetOop.decodeSmallInteger();
	auto size = sizeOop.decodeSmallInteger();
	auto file = fileOop.decodeSmallInteger();

	// Validate some of the arguments.
	if(offset < 0)
		return Oop::encodeSmallInteger(-1);

	// Perform the write
	auto res = write(file, buffer + offset, size);
	return Oop::encodeSmallInteger(res);
    */
}

NativeClassFactory OSIO::Factory("OSIO", &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addClassMethod("stdout", OSIO::stStdout)
        .addClassMethod("stdin", OSIO::stStdout)
        .addClassMethod("stderr", OSIO::stStdout)
        .addClassMethod("write:offset:size:to:", OSIO::stWriteOffsetSizeTo);
});
}

#endif
