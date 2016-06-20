#ifndef LODTALK_CLASS_FACTORY_HPP
#define LODTALK_CLASS_FACTORY_HPP

#include "Lodtalk/Definitions.h"

namespace Lodtalk
{
class ClassBuilder;
class InterpreterProxy;

/**
 * Abstract class factory
 */
class AbstractClassFactory
{
public:
    virtual int getSpecialClassIndex() const = 0;

    virtual AbstractClassFactory *getSuperClass() const = 0;

    virtual const char *getName() const = 0;
    virtual void build(ClassBuilder &builder) = 0;
};

/**
 * Registers a global class factory.
 */
LODTALK_VM_EXPORT void registerClassFactory(AbstractClassFactory *factory);

/**
 * Unregisters a global class factory.
 */
LODTALK_VM_EXPORT void unregisterClassFactory(AbstractClassFactory *factory);

/**
 * Native class factory.
 */
class LODTALK_VM_EXPORT NativeClassFactory: public AbstractClassFactory
{
public:
    typedef void (*BuildFunction) (ClassBuilder &builder);

    NativeClassFactory(const char *name, AbstractClassFactory *superClass, const BuildFunction &buildFunction)
        : name(name), superClass(superClass), buildFunction(buildFunction)
    {
        registerClassFactory(this);
    }

    ~NativeClassFactory()
    {
        unregisterClassFactory(this);
    }

    virtual int getSpecialClassIndex() const override
    {
        return -1;
    }

    virtual const char *getName() const override
    {
        return name;
    }

    virtual AbstractClassFactory *getSuperClass() const override
    {
        return superClass;
    }

    virtual void build(ClassBuilder &builder) override
    {
        buildFunction(builder);
    }

private:
    const char *name;
    AbstractClassFactory *superClass;
    BuildFunction buildFunction;
};

/**
 * Special native class factory
 */
class LODTALK_VM_EXPORT SpecialNativeClassFactory: public NativeClassFactory
{
public:
    SpecialNativeClassFactory(const char *name, int specialClassIndex, AbstractClassFactory *superClass, const BuildFunction &buildFunction)
        : NativeClassFactory(name, superClass, buildFunction), specialClassIndex(specialClassIndex)
    {}

    virtual int getSpecialClassIndex() const override
    {
        return specialClassIndex;
    }

private:
    int specialClassIndex;
};

} // End of namespace
#endif //LODTALK_CLASS_FACTORY_HPP
