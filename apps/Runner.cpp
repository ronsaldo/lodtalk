#include <string>
#include <stdio.h>
#include <string.h>
#include "Lodtalk/VMContext.hpp"
#include "Lodtalk/InterpreterProxy.hpp"

using namespace Lodtalk;
VMContext *context = nullptr;

void printHelp()
{
    printf("LodtalkRunner\n");
}

void loadKernel()
{
    //context->executeScriptFromFileNamed("runtime/runtime.lodtalk");
}

int main(int argc, const char *argv[])
{
    context = createVMContext();

    std::string scriptFilename;

    for(int i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-help") ||
           !strcmp(argv[i], "-h"))
        {
            printHelp();
            return 0;
        }
        else
        {
            scriptFilename = argv[i];
        }
    }

    if(scriptFilename.empty())
    {
        printHelp();
        return -1;
    }

    // Execute the kernel script
    loadKernel();

    // Execute the source script.
    if(scriptFilename == "-")
    {
        // Execute script from the standard input.
        context->executeScriptFromFile(stdin, "stdin");
    }
    else
    {
        // Execute the script from the file.
        context->executeScriptFromFileNamed(scriptFilename);
    }

    // Call the main function.
    context->withInterpreter([&](InterpreterProxy *interpreter) {
        interpreter->pushOop(context->getGlobalContext());
        interpreter->sendMessageWithSelector(context->makeSelector("main"), 0);
        interpreter->popOop();
    });
    return 0;
}
