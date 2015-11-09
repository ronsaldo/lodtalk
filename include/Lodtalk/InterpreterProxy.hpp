#ifndef LODTALK_INTERPRETER_PROXY_HPP_
#define LODTALK_INTERPRETER_PROXY_HPP_

#include "Lodtalk/Definitions.h"
#include "Lodtalk/VMContext.hpp"

namespace Lodtalk
{
/**
 * VM Context
 */
class InterpreterProxy
{
public:
    virtual VMContext *getContext() = 0;

    // Stack manipulation
	virtual void pushOop(Oop oop) = 0;
    virtual void pushSmallInteger(SmallIntegerValue value) = 0;

	virtual Oop &stackOopAt(size_t index) = 0;
	virtual Oop &stackTop() = 0;
    virtual Oop popOop() = 0;
    virtual void popMultiplesOops(int count) = 0;
    virtual void duplicateStackTop() = 0;

    virtual Oop getMethod() = 0;

	virtual Oop &receiver() = 0;
    virtual Oop getReceiver() = 0;

	virtual Oop &thisContext() = 0;
    virtual Oop getThisContext() = 0;

    // Returning
    virtual int returnTop() = 0;
    virtual int returnReceiver() = 0;
    virtual int returnSmallInteger(SmallIntegerValue value) = 0;

    // Temporaries
    virtual size_t getArgumentCount() = 0;
    virtual size_t getTemporaryCount() = 0;

    virtual Oop getTemporary(size_t index) = 0;
    virtual void setTemporary(size_t index, Oop value) = 0;

    // Primitive return
    virtual int primitiveFailed() = 0;
    virtual int primitiveFailedWithCode(int errorCode) = 0;

    // Basic new
    virtual void sendBasicNew() = 0;
    virtual void sendBasicNewWithSize(size_t size) = 0;

    // Message send.
    virtual void sendMessage(int argumentCount) = 0;
    virtual void sendMessageWithSelector(Oop selector, int argumentCount) = 0;
};

} // End of namespace VMContext
#endif //LODTALK_INTERPRETER_PROXY_HPP_
