BISON_TARGET(LodtalkParser Parser.y ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
FLEX_TARGET(LodtalkScanner Scanner.l ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.hpp")
ADD_FLEX_BISON_DEPENDENCY(LodtalkScanner LodtalkParser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(Lodtalk_SRC
     AST.cpp
     BytecodeSets.cpp
     Collections.cpp
     Compiler.cpp
     Exception.cpp
     FileSystem.cpp
     InputOutput.cpp
     Method.cpp
     MethodBuilder.cpp
     MemoryManager.cpp
     Object.cpp
     ObjectModel.cpp
     ParserScannerInterface.cpp
     StackInterpreter.cpp
     StackMemory.cpp
     ${BISON_LodtalkParser_OUTPUTS}
     ${FLEX_LodtalkScanner_OUTPUTS}
)

add_library(Lodtalk SHARED ${Lodtalk_SRC})

add_executable(LodtalkRunner Runner.cpp)
target_link_libraries(LodtalkRunner Lodtalk)
