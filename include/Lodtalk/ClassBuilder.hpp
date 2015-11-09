#ifndef LODTALK_CLASS_BUILDER_HPP
#define LODTALK_CLASS_BUILDER_HPP

#include <unordered_map>
#include <vector>
#include "Lodtalk/Definitions.h"
#include "Lodtalk/ObjectModel.hpp"
#include "Lodtalk/Object.hpp"

namespace Lodtalk
{
class InterpreterProxy;

typedef int (*PrimitiveFunction) (InterpreterProxy *proxy);

/**
 * A class builder
 */
class LODTALK_VM_EXPORT ClassBuilder
{
public:
    ClassBuilder(VMContext *context, Class *clazz, Metaclass *metaclass)
        : context(context), clazz(context, clazz), metaclass(context, metaclass)
    {
    }

    ClassBuilder &setName(const char *name);;
    ClassBuilder &setSuperclass(Class *superClass, Metaclass *metaSuper);

    ClassBuilder &variableSizeWithoutInstanceVariables()
    {
        clazz->format = Oop::encodeSmallInteger(OF_VARIABLE_SIZE_NO_IVARS);
        return *this;
    }

    ClassBuilder &variableSizeWithInstanceVariables()
    {
        clazz->format = Oop::encodeSmallInteger(OF_VARIABLE_SIZE_IVARS);
        return *this;
    }

    ClassBuilder &variableBits8()
    {
        clazz->format = Oop::encodeSmallInteger(OF_INDEXABLE_8);
        return *this;
    }

    ClassBuilder &variableBits16()
    {
        clazz->format = Oop::encodeSmallInteger(OF_INDEXABLE_16);
        return *this;
    }

    ClassBuilder &variableBits32()
    {
        clazz->format = Oop::encodeSmallInteger(OF_INDEXABLE_32);
        return *this;
    }

    ClassBuilder &variableBits64()
    {
        clazz->format = Oop::encodeSmallInteger(OF_INDEXABLE_64);
        return *this;
    }

    ClassBuilder &compiledMethodFormat()
    {
        clazz->format = Oop::encodeSmallInteger(OF_COMPILED_METHOD);
        return *this;
    }

    ClassBuilder &addClassMethod(const char *name, PrimitiveFunction primitive)
    {
        classSidePrimitiveMethods[name] = primitive;
        return *this;
    }

    ClassBuilder &addMethod(const char *name, PrimitiveFunction primitive)
    {
        primitiveMethods[name] = primitive;
        return *this;
    }

    ClassBuilder &addInstanceVariable(const char *name)
    {
        instanceVariableNames.push_back(name);
        return *this;
    }

    template<typename... Args>
    ClassBuilder &addInstanceVariables(const char *name, Args... args)
    {
        addInstanceVariable(name);
        addInstanceVariables(args...);
        return *this;
    }

    ClassBuilder &addInstanceVariables()
    {
        return *this;
    }

    void finish();

private:
    void createClassMethodDict();
    void createMetaclassMethodDict();

    VMContext *context;
    Ref<Class> clazz;
    Ref<Metaclass> metaclass;
    std::unordered_map<std::string, PrimitiveFunction> primitiveMethods;
    std::unordered_map<std::string, PrimitiveFunction> classSidePrimitiveMethods;
    std::vector<std::string> instanceVariableNames;
};

} // End of namespace Lodtalk

#endif //LODTALK_CLASS_BUILDER_HPP
