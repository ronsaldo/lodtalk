#ifndef LODTALK_STACK_INTERPRETER_HPP
#define LODTALK_STACK_INTERPRETER_HPP

#include "Lodtalk/InterpreterProxy.hpp"
#include "Method.hpp"

namespace Lodtalk
{
class StackInterpreter;
class VMContext;
class StackMemory;

//Oop interpretCompiledMethod(VMContext *context, CompiledMethod *method, Oop receiver, int argumentCount, Oop *arguments);
//Oop interpretBlockClosure(VMContext *context, BlockClosure *closure, int argumentCount, Oop *arguments);

} // End of namespace Lodtalk

#endif //LODTALK_STACK_INTERPRETER_HPP
