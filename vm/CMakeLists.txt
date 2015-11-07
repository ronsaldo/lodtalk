BISON_TARGET(LodtalkParser Parser.y ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
FLEX_TARGET(LodtalkScanner Scanner.l ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.hpp")
ADD_FLEX_BISON_DEPENDENCY(LodtalkScanner LodtalkParser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(Lodtalk_SRC
     AST.cpp
     AST.hpp
     BytecodeSets.cpp
     BytecodeSets.hpp
     Collections.cpp
     Collections.hpp
     Compiler.cpp
     Compiler.hpp
     Exception.cpp
     Exception.hpp
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
     MPL.hpp
     Object.cpp
     Object.hpp
     ObjectModel.cpp
     Parser.y
     ParserScannerInterface.cpp
     ParserScannerInterface.hpp
     RAII.hpp
     Scanner.l
     SistaV1BytecodeSet.inc
     SpecialClasses.inc
     StackInterpreter.cpp
     StackInterpreter.hpp
     StackMemory.cpp
     StackMemory.hpp
     Synchronization.hpp
     VMContext.cpp
     ${BISON_LodtalkParser_OUTPUTS}
     ${FLEX_LodtalkScanner_OUTPUTS}
)

add_library(Lodtalk SHARED ${Lodtalk_SRC})
