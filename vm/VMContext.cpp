#include "Lodtalk/VMContext.hpp"
#include "Compiler.hpp"

namespace Lodtalk
{

class VMContextImpl
{
public:
};

VMContext::VMContext(VMContextImpl *impl)
    : impl(impl)
{
}

VMContext::~VMContext()
{
    delete impl;
}

void VMContext::executeDoIt(const std::string &code)
{
    Lodtalk::executeDoIt(code);
}

void VMContext::executeScript(const std::string &code, const std::string &name, const std::string &basePath)
{
    Lodtalk::executeScript(code, name, basePath);
}

void VMContext::executeScriptFromFile(FILE *file, const std::string &name, const std::string &basePath)
{
    Lodtalk::executeScriptFromFile(file, name, basePath);
}

void VMContext::executeScriptFromFileNamed(const std::string &fileName)
{
    Lodtalk::executeScriptFromFileNamed(fileName);
}

LODTALK_VM_EXPORT VMContext *createVMContext()
{
    return new VMContext(new VMContextImpl());
}

} // End of namespace Lodtalk
