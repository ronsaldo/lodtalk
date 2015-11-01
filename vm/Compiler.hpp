#ifndef LODTALK_COMPILER_HPP
#define LODTALK_COMPILER_HPP

#include "AST.hpp"

namespace Lodtalk
{
// Script class
class ScriptContext: public Object
{
	ScriptContext() {}
	LODTALK_NATIVE_CLASS();
public:
	Oop setCurrentCategory(Oop category);
	Oop setCurrentClass(Oop classObject);
	Oop addFunction(Oop functionAst);
	Oop addMethod(Oop methodAst);
	Oop executeFileNamed(Oop fileName);

	Oop currentCategory;
	Oop currentClass;
	Oop globalContextClass;
	Oop basePath;
};

// Method AST handle
class MethodASTHandle: public Object
{
	MethodASTHandle() {}
	LODTALK_NATIVE_CLASS();
public:

	AST::MethodAST *ast;
};


// Compiler interface
Oop executeDoIt(const std::string &code);
Oop executeScript(const std::string &code, const std::string &name = "unnamed", const std::string &basePath = ".");
Oop executeScriptFromFile(FILE *file, const std::string &name = "unnamed", const std::string &basePath = ".");
Oop executeScriptFromFileNamed(const std::string &fileName);

}

#endif //LODTALK_COMPILER_HPP
