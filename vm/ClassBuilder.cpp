#include "Lodtalk/ClassBuilder.hpp"
#include "Lodtalk/Collections.hpp"
#include "Lodtalk/VMContext.hpp"
#include "Method.hpp"

namespace Lodtalk
{
ClassBuilder &ClassBuilder::setName(const char *name)
{
    clazz->name = ByteSymbol::fromNative(context, name);
    context->setGlobalVariable(clazz->name, clazz.getOop());
    return *this;
}

ClassBuilder &ClassBuilder::setSuperclass(Class *superClass, Metaclass *metaSuper)
{
    // Copy the format from the super class
    clazz->superclass = superClass;
    if(!isNil(superClass))
    {
        clazz->format = superClass->format;
        clazz->fixedVariableCount = superClass->fixedVariableCount;
    }
    else
    {
        clazz->format = Oop::encodeSmallInteger(OF_EMPTY);
        clazz->fixedVariableCount = Oop::encodeSmallInteger(0);
    }

    // Set the meta class super class.
    metaclass->superclass = metaSuper;

    return *this;
}

void ClassBuilder::finish()
{
    clazz->instanceVariables = Oop::fromPointer(Array::basicNativeNew(context, instanceVariableNames.size()));
    for(size_t i = 0; i < instanceVariableNames.size(); ++i)
    {
        auto varNameSymbol = ByteSymbol::fromNative(context, instanceVariableNames[i]);
        auto array = reinterpret_cast<Oop*> (clazz->instanceVariables.getFirstFieldPointer());
        array[i] = varNameSymbol;
    }

    // Set the fixed variable count.
    if(!instanceVariableNames.empty())
    {
        clazz->fixedVariableCount = Oop::encodeSmallInteger(clazz->fixedVariableCount.decodeSmallInteger() + instanceVariableNames.size());
        if(clazz->format.decodeSmallInteger() == OF_EMPTY)
            clazz->format = Oop::encodeSmallInteger(OF_FIXED_SIZE);
    }

    createClassMethodDict();
    createMetaclassMethodDict();
}

void ClassBuilder::createClassMethodDict()
{
    clazz->methodDict = MethodDictionary::basicNativeNew(context);
    OopRef selector(context);
    OopRef method(context);
    for(auto &selectorAndMethod : primitiveMethods)
    {
        selector = ByteSymbol::fromNative(context, selectorAndMethod.first);
        method = Oop::fromPointer(NativeMethod::create(context, selectorAndMethod.second));
        clazz->methodDict->atPut(context, selector.oop, method.oop);
    }
}

void ClassBuilder::createMetaclassMethodDict()
{
    metaclass->methodDict = MethodDictionary::basicNativeNew(context);
    OopRef selector(context);
    OopRef method(context);
    for(auto &selectorAndMethod : classSidePrimitiveMethods)
    {
        selector = ByteSymbol::fromNative(context, selectorAndMethod.first);
        method = Oop::fromPointer(NativeMethod::create(context, selectorAndMethod.second));
        metaclass->methodDict->atPut(context, selector.oop, method.oop);
    }
}

} // End of namespace Lodtalk
