#ifndef LODTALK_INPUT_OUTPUT_HPP
#define LODTALK_INPUT_OUTPUT_HPP

#include "Object.hpp"
#include "Collections.hpp"

namespace Lodtalk
{

/**
 * Operating system, input and output interface
 */
class OSIO: public Object
{
	LODTALK_NATIVE_CLASS();
public:
	static Oop stStdout(Oop clazz);
	static Oop stStdin(Oop clazz);
	static Oop stStderr(Oop clazz);
	
	static Oop writeOffsetSizeTo(Oop clazz, Oop buffer, Oop offset, Oop size, Oop file);
};

} // End of namespace Lodtalk

#endif //LODTALK_INPUT_OUTPUT_HPP
