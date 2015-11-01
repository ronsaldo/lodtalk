#include <vector>
#include "BytecodeSets.hpp"

namespace Lodtalk
{

static std::vector<std::string> sistaByteCodeNames;
const std::string &getSistaBytecodeName(int bytecode)
{
    if(sistaByteCodeNames.empty())
    {
        char buffer[64];
        sistaByteCodeNames.resize(256, "unssuported");

        #define SISTAV1_INSTRUCTION_RANGE(name, range_first, range_end) \
        for(int i = range_first; i <= range_end; ++i) {\
            sprintf(buffer, #name ": %d", i - range_first); \
            sistaByteCodeNames[i] = buffer; \
        }
        #define SISTAV1_INSTRUCTION(name, opcode) \
            sistaByteCodeNames[opcode] = #name;

        #include "SistaV1BytecodeSet.inc"

        #undef SISTAV1_INSTRUCTION_RANGE
        #undef SISTAV1_INSTRUCTION

    }

    return sistaByteCodeNames[bytecode];
}

int getSistaBytecodeSize(int bytecode)
{
    if(bytecode < 224)
        return 1;
    if(bytecode < 248)
        return 2;
    return 3;
}

void dumpSistaBytecode(uint8_t *buffer, size_t size)
{
    size_t pos = 0;
    while(pos < size)
    {
        auto opcode = buffer[pos++];
        auto size = getSistaBytecodeSize(opcode);
        printf("%s", getSistaBytecodeName(opcode).c_str());
        if(size > 1)
            printf(" %d", buffer[pos++]);
        if(size > 2)
            printf(" %d", buffer[pos++]);
        printf("\n");
    }
}
} // End of namspace
