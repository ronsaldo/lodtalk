#include "Lodtalk/ClassFactory.hpp"
#include "Lodtalk/VMContext.hpp"
#include "ClassFactoryRegistry.hpp"

namespace Lodtalk
{

ClassFactoryRegistry::ClassFactoryRegistry()
{
}

ClassFactoryRegistry::~ClassFactoryRegistry()
{
}

ClassFactoryRegistry *ClassFactoryRegistry::uniqueInstance = nullptr;
ClassFactoryRegistry *ClassFactoryRegistry::get()
{
    if(!uniqueInstance)
        uniqueInstance = new ClassFactoryRegistry();
    return uniqueInstance;
}

void ClassFactoryRegistry::registerFactory(AbstractClassFactory *factory)
{
    factories.insert(factory);
}

void ClassFactoryRegistry::unregisterFactory(AbstractClassFactory *factory)
{
    factories.erase(factory);
}

void ClassFactoryRegistry::registerVMContext(VMContext *context)
{
    for (auto factory : factories)
        context->instanceClassFactory(factory);
}

void ClassFactoryRegistry::unregisterVMContext(VMContext *context)
{
}

LODTALK_VM_EXPORT void registerClassFactory(AbstractClassFactory *factory)
{
    ClassFactoryRegistry::get()->registerFactory(factory);
}

LODTALK_VM_EXPORT void unregisterClassFactory(AbstractClassFactory *factory)
{
    ClassFactoryRegistry::get()->unregisterFactory(factory);
}

} // End of namespace Lodtalk
