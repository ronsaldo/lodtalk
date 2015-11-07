#ifndef LODTALK_INTERPRETER_PROXY_HPP_
#define LODTALK_INTERPRETER_PROXY_HPP_

#include "Definitions.h"

namespace Lodtalk
{
/**
 * VM Context
 */
class InterpreterProxy
{
public:
    virtual VMContext *getContext() = 0;
};

} // End of namespace VMContext
#endif //LODTALK_INTERPRETER_PROXY_HPP_
