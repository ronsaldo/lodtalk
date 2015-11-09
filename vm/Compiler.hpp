#ifndef LODTALK_COMPILER_HPP
#define LODTALK_COMPILER_HPP

#include "AST.hpp"

namespace Lodtalk
{
class InterpreterProxy;

// Script class
class ScriptContext: public Object
{
public:
    static SpecialNativeClassFactory Factory;

	static int stSetCurrentCategory(InterpreterProxy *proxy);
	static int stSetCurrentClass(InterpreterProxy *proxy);
	static int stAddFunction(InterpreterProxy *proxy);
	static int stAddMethod(InterpreterProxy *proxy);
	static int stExecuteFileNamed(InterpreterProxy *proxy);

	Oop currentCategory;
	Oop currentClass;
	Oop globalContextClass;
	Oop basePath;
};

// Method AST handle
class MethodASTHandle: public Object
{
public:
    static SpecialNativeClassFactory Factory;

    static MethodASTHandle *basicNativeNew(VMContext *context, AST::MethodAST *ast);

	AST::MethodAST *ast;
};


// Compiler interface
int executeDoIt(InterpreterProxy *interpreter, const std::string &code);
int executeScript(InterpreterProxy *interpreter, const std::string &code, const std::string &name = "unnamed", const std::string &basePath = ".");
int executeScriptFromFile(InterpreterProxy *interpreter, FILE *file, const std::string &name = "unnamed", const std::string &basePath = ".");
int executeScriptFromFileNamed(InterpreterProxy *interpreter, const std::string &fileName);

}

#endif //LODTALK_COMPILER_HPP
