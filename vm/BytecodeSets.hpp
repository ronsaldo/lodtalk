#ifndef LODTALK_BYTECODE_SETS_HPP
#define LODTALK_BYTECODE_SETS_HPP

#include <string>

namespace Lodtalk
{

/**
 * SistaV1 bytecode set.
 */
namespace SistaV1BytecodeSet
{
constexpr int Send_ArgumentCountMask = 7;
constexpr int Send_ArgumentCountCount = 8;

constexpr int Send_LiteralIndexShift = 3;
constexpr int Send_LiteralIndexMask = (1<<5) - 1;
constexpr int Send_LiteralIndexCount = (1<<5);

constexpr int PushClosure_NumExtensionsMask = 3;
constexpr int PushClosure_NumExtensionsShift = 6;
constexpr int PushClosure_NumCopiedMask = 3;
constexpr int PushClosure_NumCopiedShift = 7;
constexpr int PushClosure_NumArgsMask = 7;
constexpr int PushClosure_NumArgsShift = 0;

#define SISTAV1_INSTRUCTION_RANGE(name, range_first, range_end) \
	constexpr int name##First = range_first; \
	constexpr int name##Last = range_end; \
	constexpr int name##RangeSize = range_end - range_first + 1;

#define SISTAV1_INSTRUCTION(name, opcode) \
	constexpr int name = opcode;

#include "SistaV1BytecodeSet.inc"

#undef SISTAV1_INSTRUCTION_RANGE
#undef SISTAV1_INSTRUCTION
};

namespace BytecodeSet = SistaV1BytecodeSet;

const std::string &getSistaBytecodeName(int bytecode);
int getSistaBytecodeSize(int bytecode);
void dumpSistaBytecode(uint8_t *buffer, size_t size);

} // End of namespace Lodtalk

#endif //LODTALK_BYTECODE_SETS_HPP
