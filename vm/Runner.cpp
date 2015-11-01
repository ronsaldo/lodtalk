#include <string>
#include <stdio.h>
#include <string.h>
#include "Compiler.hpp"

using namespace Lodtalk;

void printHelp()
{
}

void loadKernel()
{
    executeScriptFromFileNamed("runtime/runtime.lodtalk");
}

int main(int argc, const char *argv[])
{
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
        executeScriptFromFile(stdin, "stdin");
    }
    else
    {
        // Execute the script from the file.
        executeScriptFromFileNamed(scriptFilename);
    }
    
    // Call the main function.
    auto globalContext = getGlobalContext();
    sendMessageOopArgs(globalContext, makeSelector("main"));
    return 0;
}
