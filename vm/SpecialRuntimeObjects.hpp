#ifndef LODTALK_VM_SPECIAL_RUNTIME_OBJECTS_HPP_
#define LODTALK_VM_SPECIAL_RUNTIME_OBJECTS_HPP_

#include <vector>
#include "Lodtalk/ObjectModel.hpp"
#include "Lodtalk/VMContext.hpp"

namespace Lodtalk
{

// Special runtime objects
class SpecialRuntimeObjects
{
public:
	SpecialRuntimeObjects(VMContext *context);
	~SpecialRuntimeObjects();

    void initialize();

	void createSpecialObjectTable();
	void createSpecialClassTable();
    void registerGCRoots();

	std::vector<Oop> specialObjectTable;
	std::vector<ClassDescription*> specialClassTable;

    size_t blockActivationSelectorFirst;
    size_t blockActivationSelectorCount;
    size_t specialMessageSelectorFirst;
    size_t specialMessageSelectorCount;
    size_t compilerMessageSelectorFirst;
    size_t compilerMessageSelectorCount;

private:
    Oop makeSelector(const char *string);

    VMContext *context;
};

} // End of namespace Lodtalk

#endif //LODTALK_VM_SPECIAL_RUNTIME_OBJECTS_HPP_
