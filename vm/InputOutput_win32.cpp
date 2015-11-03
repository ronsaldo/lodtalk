#ifdef _WIN32

#include <stdio.h>
#include "InputOutput.hpp"
#include "Method.hpp"
namespace Lodtalk
{

Oop OSIO::stStdout(Oop clazz)
{
	return Oop::encodeSmallInteger(0);
}

Oop OSIO::stStdin(Oop clazz)
{
	return Oop::encodeSmallInteger(1);
}

Oop OSIO::stStderr(Oop clazz)
{
	return Oop::encodeSmallInteger(2);
}

Oop OSIO::writeOffsetSizeTo(Oop clazz, Oop bufferOop, Oop offsetOop, Oop sizeOop, Oop fileOop)
{
    return Oop::encodeSmallInteger(-1);
	/*// Check the arguments.
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
	return Oop::encodeSmallInteger(res);*/
}

LODTALK_BEGIN_CLASS_SIDE_TABLE(OSIO)
	LODTALK_METHOD("stdout", OSIO::stStdout)
	LODTALK_METHOD("stdin", OSIO::stStdin)
	LODTALK_METHOD("stderr", OSIO::stStderr)
	LODTALK_METHOD("write:offset:size:to:", OSIO::writeOffsetSizeTo)
LODTALK_END_CLASS_SIDE_TABLE()

LODTALK_BEGIN_CLASS_TABLE(OSIO)
LODTALK_END_CLASS_TABLE()

LODTALK_SPECIAL_SUBCLASS_DEFINITION(OSIO, Object, OF_EMPTY, 0);

}

#endif //_WIN32
