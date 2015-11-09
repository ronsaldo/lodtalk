#ifndef CLASS_FACTORY_REGISTRY_HPP_
#define CLASS_FACTORY_REGISTRY_HPP_

#include <unordered_set>
#include "Lodtalk/ClassFactory.hpp"

namespace Lodtalk
{
class VMContext;

/**
 * Class factory registry
 */
class ClassFactoryRegistry
{
public:
    typedef std::unordered_set<AbstractClassFactory*> Factories;
    ~ClassFactoryRegistry();

    void registerFactory(AbstractClassFactory *factory);
    void unregisterFactory(AbstractClassFactory *factory);

    void registerVMContext(VMContext *context);
    void unregisterVMContext(VMContext *context);

    static ClassFactoryRegistry *get();

private:
    ClassFactoryRegistry();

    static ClassFactoryRegistry *uniqueInstance;
    Factories factories;
};

} // End of namespace Lodtalk

#endif //CLASS_FACTORY_REGISTRY_HPP_
