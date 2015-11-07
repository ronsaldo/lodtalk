#ifndef LODTALK_CLASS_FACTORY_HPP
#define LODTALK_CLASS_FACTORY_HPP

#include <functional>
#include "Lodtalk/ClassBuilder.hpp"

namespace Lodtalk
{
/**
 * Abstract class factory
 */
class AbstractClassFactory
{
public:
    virtual int getSpecialClassIndex() const = 0;

    virtual const char *getName() const = 0;
    virtual void build(ClassBuilder &builder) = 0;
};

/**
 * Registers a global class factory.
 */
void registerClassFactory(AbstractClassFactory *factory);

/**
 * Unregisters a global class factory.
 */
void unregisterClassFactory(AbstractClassFactory *factory);

/**
 * Native class factory.
 */
class NativeClassFactory: public AbstractClassFactory
{
public:
    typedef std::function<void (ClassBuilder &builder)> BuildFunction;

    NativeClassFactory(AbstractClassFactory *superClass, const BuildFunction &buildFunction)
        : superClass(superClass), buildFunction(buildFunction)
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

    virtual void build(ClassBuilder &builder) override
    {
        buildFunction(builder);
    }

private:
    AbstractClassFactory *superClass;
    BuildFunction buildFunction;
};

/**
 * Special native class factory
 */
class SpecialNativeClassFactory: public NativeClassFactory
{
public:
    SpecialNativeClassFactory(int specialClassIndex, AbstractClassFactory *superClass, const BuildFunction &buildFunction)
        : NativeClassFactory(superClass, buildFunction), specialClassIndex(specialClassIndex)
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
