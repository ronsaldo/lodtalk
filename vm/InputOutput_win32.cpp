#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Lodtalk/InterpreterProxy.hpp"
#include "InputOutput.hpp"
#include "Method.hpp"
namespace Lodtalk
{

int OSIO::stStdout(InterpreterProxy *interpreter)
{
    return interpreter->returnExternalHandle(GetStdHandle(STD_OUTPUT_HANDLE));
}

int OSIO::stStdin(InterpreterProxy *interpreter)
{
    return interpreter->returnExternalHandle(GetStdHandle(STD_INPUT_HANDLE));
}

int OSIO::stStderr(InterpreterProxy *interpreter)
{
    return interpreter->returnExternalHandle(GetStdHandle(STD_ERROR_HANDLE));
}

int OSIO::stWriteOffsetSizeTo(InterpreterProxy *interpreter)
{
    if(interpreter->getArgumentCount() != 4)
        return interpreter->primitiveFailed();

    Oop bufferOop = interpreter->getTemporary(0);
    Oop offsetOop = interpreter->getTemporary(1);
    Oop sizeOop = interpreter->getTemporary(2);
    Oop fileOop = interpreter->getTemporary(3);

	// Check the arguments.
	if(!bufferOop.isIndexableNativeData() || !offsetOop.isSmallInteger() || !sizeOop.isSmallInteger() || !fileOop.isExternalHandle())
		return interpreter->primitiveFailed();

	// Decode the arguments
	auto buffer = reinterpret_cast<uint8_t*> (bufferOop.getFirstFieldPointer());
	auto offset = offsetOop.decodeSmallInteger();
	auto size = sizeOop.decodeSmallInteger();
	auto file = fileOop.decodeExternalHandle();

	// Validate some of the arguments.
	if(offset < 0)
		return interpreter->primitiveFailed();

	// Perform the write
    DWORD written;
    auto success = WriteFile(file, buffer + offset, size, &written, nullptr);
    if (success == FALSE)
        return interpreter->primitiveFailedWithCode(GetLastError());

    return interpreter->returnSmallInteger(written);
}

NativeClassFactory OSIO::Factory("OSIO", &Object::Factory, [](ClassBuilder &builder) {
    builder
        .addClassMethod("stdout", OSIO::stStdout)
        .addClassMethod("stdin", OSIO::stStdout)
        .addClassMethod("stderr", OSIO::stStdout)
        .addClassMethod("write:offset:size:to:", OSIO::stWriteOffsetSizeTo);
});

}

#endif //_WIN32
