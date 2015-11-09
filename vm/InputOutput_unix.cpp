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
    return interpreter->returnSmallInteger(STDOUT_FILENO);
}

int OSIO::stStdin(InterpreterProxy *interpreter)
{
    return interpreter->returnSmallInteger(STDOUT_FILENO);
}

int OSIO::stStderr(InterpreterProxy *interpreter)
{
    return interpreter->returnSmallInteger(STDOUT_FILENO);
}

// Oop bufferOop, Oop offsetOop, Oop sizeOop, Oop fileOop
int OSIO::stWriteOffsetSizeTo(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 4)
        return interpreter->primitiveFailed();

    Oop bufferOop = interpreter->getTemporary(0);
    Oop offsetOop = interpreter->getTemporary(1);
    Oop sizeOop = interpreter->getTemporary(2);
    Oop fileOop = interpreter->getTemporary(3);

	// Check the arguments.
	if(!bufferOop.isIndexableNativeData() || !offsetOop.isSmallInteger() || !sizeOop.isSmallInteger() || !fileOop.isSmallInteger())
		return interpreter->primitiveFailed();

	// Decode the arguments
	auto buffer = reinterpret_cast<uint8_t *> (bufferOop.getFirstFieldPointer());
	auto offset = offsetOop.decodeSmallInteger();
	auto size = sizeOop.decodeSmallInteger();
	auto file = fileOop.decodeSmallInteger();

	// Validate some of the arguments.
	if(offset < 0)
		return interpreter->primitiveFailed();

	// Perform the write
	auto res = write(file, buffer + offset, size);
	return interpreter->returnSmallInteger(res);
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
