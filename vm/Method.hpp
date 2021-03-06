#ifndef LODTALK_METHOD_HPP_
#define LODTALK_METHOD_HPP_

#include <type_traits>
#include <vector>
#include "Lodtalk/ClassBuilder.hpp"
#include "Lodtalk/Object.hpp"
#include "Lodtalk/Collections.hpp"

namespace Lodtalk
{
class NativeMethodWrapper;

/**
 * Additional method state.
 */
class AdditionalMethodState: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static AdditionalMethodState *basicNew(VMContext *context, size_t size);

    Oop getMethod()
    {
        return getElements()[0];
    }

    Oop getSelector()
    {
        return getElements()[1];
    }

    void setMethod(const Oop &method)
    {
        getElements()[0] = method;
    }

    void setSelector(const Oop &selector)
    {
        getElements()[1] = selector;
    }

    Oop *getIndexableElements()
    {
        return getElements() + 2;
    }

private:
    Oop *getElements()
    {
        return reinterpret_cast<Oop*> (getFirstFieldPointer());
    }
};

/**
 * Compiled method
 */
class CompiledMethod: public ByteArray
{
public:
    static SpecialNativeClassFactory Factory;

	static CompiledMethod *newMethodWithHeader(VMContext *context, size_t numberOfBytes, CompiledMethodHeader header);

	CompiledMethodHeader *getHeader()
	{
		return reinterpret_cast<CompiledMethodHeader*> (getFirstFieldPointer());
	}

	size_t getLiteralCount()
	{
		return getHeader()->getLiteralCount();
	}

	size_t getTemporalCount()
	{
		return getHeader()->getTemporalCount();
	}

	size_t getArgumentCount()
	{
		return getHeader()->getArgumentCount();
	}

	size_t getFirstPC()
	{
		return (getHeader()->getLiteralCount() + 1)*sizeof(void*);
	}

	uint8_t *getFirstBCPointer()
	{
		return reinterpret_cast<uint8_t*> (getFirstFieldPointer() + getFirstPC());
	}

	size_t getFirstPCOffset()
	{
		return getFirstBCPointer() - reinterpret_cast<uint8_t*> (this);
	}

    size_t getByteDataSize()
    {
        return getNumberOfElements() - getFirstPC();
    }

	Oop *getFirstLiteralPointer()
	{
		return reinterpret_cast<Oop *> (getFirstFieldPointer() + sizeof(void*));
	}

	Oop getSelector()
	{
		auto selectorIndex = getLiteralCount() - 2;
		auto selectorOop = getFirstLiteralPointer()[selectorIndex];
        if(selectorOop.header->classIndex == SCI_AdditionalMethodState)
        {
            auto methodState = reinterpret_cast<AdditionalMethodState*> (selectorOop.pointer);
            selectorOop = methodState->getSelector();
        }

		return selectorOop;
	}

	Oop getClassBinding()
	{
		auto classBindingIndex = getLiteralCount() - 1;
		return getFirstLiteralPointer()[classBindingIndex];
	}

    Oop getMethodClass()
    {
        auto association = reinterpret_cast<Association*> (getClassBinding().pointer);
        assert(!isNil(association));
        return association->value;
    }

    Oop dump();

    bool hasPrimitive()
    {
        return getHeader()->hasPrimitive();
    }

    int getPrimitiveIndex()
    {
        if (hasPrimitive())
        {
            auto bc = getFirstBCPointer();
            return bc[1] | (bc[2] << 8);
        }
        else
        {
            return 0;
        }
    }

    static int stNewMethodWithHeader(InterpreterProxy *interpreter);

    static int stObjectAt(InterpreterProxy *interpreter);
    static int stObjectAtPut(InterpreterProxy *interpreter);

    static int stDump(InterpreterProxy *interpreter);

};

/**
 * Native method
 */
class NativeMethod: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static NativeMethod *create(VMContext *context, PrimitiveFunction primitive);

    PrimitiveFunction primitive;
};

/**
 * InstructionStream
 */
class InstructionStream: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static const int InstructionStreamVariableCount = 2;

    static const int SenderIndex = 0;
    static const int PCIndex = 1;

    Oop sender;
    Oop pc;
};

/**
 * Context
 */
class Context: public InstructionStream
{
public:
    static SpecialNativeClassFactory Factory;

    static const int ContextVariableCount = InstructionStreamVariableCount + 5;
    static const size_t LargeContextSlots = 62;
    static const size_t SmallContextSlots = 22;

    static const int StackPointerIndex = InstructionStreamVariableCount + 0;

    static Context *create(VMContext *context, size_t slotCount);

    bool isMarriedOrWidowed()
    {
        return sender.isSmallInteger();
    }

    Oop getReceiver()
    {
        return receiver;
    }

    Oop stackp;
    CompiledMethod *method;
    Oop closureOrNil;
    Oop receiver;
    Oop argumentCount;
    Oop data[];
};

/**
 * Block closure
 */
class BlockClosure: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static const int BlockClosureVariableCount = 3;

    static BlockClosure *create(VMContext *context, int numcopied);

    inline Oop *getData()
    {
        return reinterpret_cast<Oop*> (getFirstFieldPointer());
    }

    inline Context *getOuterContext()
    {
        return reinterpret_cast<Context*> (getData()[0].pointer);
    }

    inline Oop getReceiver()
    {
        return getOuterContext()->getReceiver();
    }

    inline size_t getArgumentCount()
    {
        return numArgs.decodeSmallInteger();
    }

    Oop outerContext;
    Oop startpc;
    Oop numArgs;
    Oop copiedData[];
};

/**
 * Message
 */
class Message: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static Message *create(VMContext *context);

    static const int MessageVariableCount = 3;

    Oop selector;
    Oop args;
    Oop lookupClass;
};

} // End of namespace Lodtalk

#endif //LODTALK_METHOD_HPP_
