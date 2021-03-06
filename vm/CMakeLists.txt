BISON_TARGET(LodtalkParser Parser.y ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
FLEX_TARGET(LodtalkScanner Scanner.l ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.hpp")
ADD_FLEX_BISON_DEPENDENCY(LodtalkScanner LodtalkParser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DBUILDING_LODTALK_VM)

set(LodtalkVM_HEADERS
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/ClassBuilder.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/ClassFactory.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Collections.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Definitions.h"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Exception.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/InterpreterProxy.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Math.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Object.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/ObjectModel.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/SpecialClasses.inc"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/Synchronization.hpp"
  "${LODTALK_VM_INCLUDE_DIR}/Lodtalk/VMContext.hpp"
)

set(LodtalkVM_SRC
     AST.cpp
     AST.hpp
     BytecodeSets.cpp
     BytecodeSets.hpp
     ClassBuilder.cpp
     ClassFactoryRegistry.cpp
     ClassFactoryRegistry.hpp
     Collections.cpp
     Compiler.cpp
     Compiler.hpp
     Exception.cpp
     FileSystem.cpp
     FileSystem.hpp
     InputOutput_unix.cpp
     InputOutput_win32.cpp
     InputOutput.hpp
     MemoryManager.cpp
     MemoryManager.hpp
     Method.cpp
     Method.hpp
     MethodBuilder.cpp
     MethodBuilder.hpp
     Object.cpp
     ObjectModel.cpp
     Parser.y
     ParserScannerInterface.cpp
     ParserScannerInterface.hpp
     RAII.hpp
     Scanner.l
     SistaV1BytecodeSet.inc
     SpecialRuntimeObjects.cpp
     SpecialRuntimeObjects.hpp
     StackInterpreter.cpp
     StackInterpreter.hpp
     StackMemory.cpp
     StackMemory.hpp
     VMContext.cpp
     ${BISON_LodtalkParser_OUTPUTS}
     ${FLEX_LodtalkScanner_OUTPUTS}
)

add_library(LodtalkVM SHARED ${LodtalkVM_SRC} ${LodtalkVM_HEADERS})
