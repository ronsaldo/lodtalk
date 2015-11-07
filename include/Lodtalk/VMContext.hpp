#ifndef LODTALK_VMCONTEXT_HPP_
#define LODTALK_VMCONTEXT_HPP_

#include <string>
#include <stdio.h>
#include "Lodtalk/Definitions.h"
#include "Lodtalk/ObjectModel.hpp"

namespace Lodtalk
{
class VMContextImpl;

/**
 * VM Context
 */
class LODTALK_VM_EXPORT VMContext
{
public:
    VMContext(VMContextImpl *impl);
    ~VMContext();

    void executeDoIt(const std::string &code);
    void executeScript(const std::string &code, const std::string &name = "unnamed", const std::string &basePath = ".");
    void executeScriptFromFile(FILE *file, const std::string &name = "unnamed", const std::string &basePath = ".");
    void executeScriptFromFileNamed(const std::string &fileName);

private:
    VMContextImpl *impl;
};

LODTALK_VM_EXPORT VMContext *createVMContext();

} // End of namespace VMContext
#endif //LODTALK_VMCONTEXT_HPP_
