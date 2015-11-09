#ifndef LODTALK_INPUT_OUTPUT_HPP
#define LODTALK_INPUT_OUTPUT_HPP

#include "Lodtalk/Object.hpp"
#include "Lodtalk/Collections.hpp"

namespace Lodtalk
{

/**
 * Operating system, input and output interface
 */
class OSIO: public Object
{
public:
    static NativeClassFactory Factory;

	static int stStdout(InterpreterProxy *interpreter);
	static int stStdin(InterpreterProxy *interpreter);
	static int stStderr(InterpreterProxy *interpreter);

	static int stWriteOffsetSizeTo(InterpreterProxy *interpreter);
};

} // End of namespace Lodtalk

#endif //LODTALK_INPUT_OUTPUT_HPP
