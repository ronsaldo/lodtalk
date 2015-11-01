#ifndef LODTALK_STACK_INTERPRETER_HPP
#define LODTALK_STACK_INTERPRETER_HPP

#include "Method.hpp"

namespace Lodtalk
{

Oop interpretCompiledMethod(CompiledMethod *method, Oop receiver, int argumentCount, Oop *arguments);
Oop interpretBlockClosure(BlockClosure *closure, int argumentCount, Oop *arguments);

} // End of namespace Lodtalk

#endif //LODTALK_STACK_INTERPRETER_HPP
