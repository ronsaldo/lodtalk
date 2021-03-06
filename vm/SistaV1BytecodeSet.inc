/***
This is a copy of the original documentation about this bytecode set designed by Clement Bera and Eliot Miranda.
-------------------------------------------------------------------------------------------------------------------------------
IRSistaV1BytecodeGenerator encodes a bytecode set for Sista, the Speculative Inlining Smalltalk Architecture, a project by Clément Bera and Eliot Miranda.  Sista is an optimizer that exists in the Smalltalk image, /not/ in the VM,  and optimizes by substituting normal bytecoded methods by optimized bytecoded methods that may use special bytecodes for which the Cogit can generate faster code.  These bytecodes eliminate overheads such as bounds checks or polymorphic code (indexing Array, ByteArray, String etc).  But the bulk of the optimization performed is in inlining blocks and sends for the common path.  This bytecode set therefore differs from a normal Smalltalk set in providing a set of inlined primitives that do not validate their arguments that the compiler generates only when it can prove that the primitives' arguments are valid.

The basic scheme is that the Cogit generates code containing performance counters.  When these counters trip, a callback into the image is performed, at which point Sista analyses some portion of the stack, looking at performance data for the methods on the stack, and optimises based on the stack and performance data.  Execution then resumes in the optimized code.

The Sista Cogit (e.g. SistaStackToRegisterMappingCogit) adds counters to conditional branches.  Each branch has an executed and a taken count.  On execution the executed count is decremented and if the count goes below zero the VM sends a message at a special index in the specialObjectsArray (as of writing, conditionalCounterTrippedOn:).  Then if the branch is taken the taken count is decremented.  The two counter values allow the Sista optimizer to collect basic block execution paths and to know what are the "hot" paths through execution that are worth agressively optimizing.  Since conditional branches are about 1/6 as frequent as sends, and since they can be used to determine the hot path through code, they are a better choice to count than, for example, method or block entry.

The VM provides a primitive that fills an Array with the state of the counters, and the state of each linked send in a method.  Tthe optimizer obtains the branch and send data for a method via this primitive.

1 Byte Bytecodes
	0-15		0000 iiii 			Push Receiver Variable #iiii
	16-31		0001 iiii			Push Literal Variable #iiii
	32-63		001 iiiii				Push Literal #iiiii
	64-71		01000 iii			Push Temp #iii
	72-75		010010 ii			Push Temp #ii + 8
	76			01001100			Push Receiver
	77			01001101			Push true
	78			01001110			Push false
	79			01001111			Push nil
	80			01010000			Push 0
	81			01010001			Push 1
*	82			01010010			Push thisContext, (then e.g. Extend 1 = push thisProcess)
	83			01010011			Duplicate Stack Top
	85-87		010101 ii			UNASSIGNED
	88-91		010110 ii			Return Receiver/true/false/nil
	92			01011100			Return top
	93			01011101			BlockReturn nil
*	94			01011110			BlockReturn Top [* return from enclosing block N, N = Extend A, then jump by Ext B ]
	95			01011111			Nop
	96-111		0110 iiii			Send Arithmetic Message #iiii #(#+ #- #< #> #'<=' #'>=' #= #'~=' #* #/ #'\\' #@ #bitShift: #'//' #bitAnd: #bitOr:)
	112-119	01110 iii			Send Special Message #iii #(#at: #at:put: #size #next #nextPut: #atEnd #'==' class)
	120		01111000			UNASSIGNED (was: blockCopy:)
	121		01111001			Send Special Message #value
	122-123	0111101 i			Send Special Message #i #(#value: #do:)
	124-127	011111 ii			Send Special Message #ii #(#new #new: #x #y))
	128-143	1000 iiii			Send Literal Selector #iiii With 0 Argument
	144-159	1001 iiii			Send Literal Selector #iiii With 1 Arguments
	160-175	1010 iiii			Send Literal Selector #iiii With 2 Arguments
	176-183	10110 iii			Jump iii + 1 (i.e., 1 through 8)
	184-191	10111 iii			Pop and Jump 0n True iii +1 (i.e., 1 through 8)
	192-199	11000 iii			Pop and Jump 0n False iii +1 (i.e., 1 through 8)
	200-207	11001 iii			Pop and Store Receiver Variable #iii
	208-215	11010 iii			Pop and Store Temporary Variable #iii
	216		11011000			Pop Stack Top
	217		11011001			UNASSIGNED
	218-219	1101101 i			UNASSIGNED
	220-223	110111 ii			UNASSIGNED

2 Byte Bytecodes
*	224		11100000	aaaaaaaa	Extend A (Ext A = Ext A prev * 256 + Ext A)
*	225		11100001	bbbbbbbb	Extend B (Ext B = Ext B prev * 256 + Ext B)
*	226		11100010	iiiiiiii		Push Receiver Variable #iiiiiiii (+ Extend A * 256)
*	227		11100011	iiiiiiii		Push Literal Variable #iiiiiiii (+ Extend A * 256)
*	228		11100100	iiiiiiii		Push Literal #iiiiiiii (+ Extend A * 256)
	229		11100101	iiiiiiii		Push Temporary Variable #iiiiiiii
	230		11100110	iiiiiiii		PushNClosureTemps iiiiiiii
*	231		11100111	iiiiiiii		Push Integer #iiiiiiii (+ Extend B * 256, where bbbbbbbb = sddddddd, e.g. -32768 = i=0, a=0, s=1)
*	232		11101000	iiiiiiii		Push Character #iiiiiiii (+ Extend B * 256) (move ot 230)
	233		11101001	jkkkkkkk	Push (Array new: kkkkkkk) (j = 0) &	Pop kkkkkkk elements into: (Array new: kkkkkkk) (j = 1)
**	234		11101010	iiiiijjj		Send Literal Selector #iiiii (+ Extend A * 32) with jjj (+ Extend B * 8) Arguments
**	235		11101011	iiiiijjj		Send To Superclass Literal Selector #iiiii (+ Extend A * 32) with jjj (+ Extend B * 8) Arguments
*	236		11101100	iiiiiiii		trap on behavior/array of behavior #iiiiiiii (+ Extend B * 256, where Extend B >= 0)
*	237		11101101	iiiiiiii		Jump #iiiiiiii (+ Extend B * 256, where bbbbbbbb = sddddddd, e.g. -32768 = i=0, a=0, s=1)
*	238		11101110	iiiiiiii		Pop and Jump 0n True #iiiiiiii (+ Extend B * 256, where Extend B >= 0)
*	239		11101111	iiiiiiii		Pop and Jump 0n False #iiiiiiii (+ Extend B * 256, where Extend B >= 0)
*	240		11110000	iiiiiiii		Pop and Store Receiver Variable #iiiiiii (+ Extend A * 256)
*	241		11110001	iiiiiiii		Pop and Store Literal Variable #iiiiiiii (+ Extend A * 256)
*	242		11110010	iiiiiiii		Pop and Store Temporary Variable #iiiiiiii
	243		11110011	iiiiiiii		Store Receiver Variable #iiiiiii (+ Extend A * 256)
*	244		11110100	iiiiiiii		Store Literal Variable #iiiiiiii (+ Extend A * 256)
*	245		11110110	iiiiiiii		Store Temporary Variable #iiiiiiii
	246-247	1111011 i	xxxxxxxx	UNASSIGNED

3 Byte Bytecodes
	248		11111000 	iiiiiiii	mjjjjjjj	Call Primitive #iiiiiiii + (jjjjjjj * 256) m=1 means inlined primitive, no hard return after execution.
	249		11111001 	xxxxxxxx	syyyyyyy	Reserved for Push Float
**	250		11111010 	eeiiikkk		jjjjjjjj		Push Closure Num Copied iii (+ExtA//16*8) Num Args kkk (+ ExtA\\16*8) BlockSize jjjjjjjj (+ExtB*256). ee = num extensions
	251		11111011 	kkkkkkkk	jjjjjjjj		Push Temp At kkkkkkkk In Temp Vector At: jjjjjjjj
	252		11111100 	kkkkkkkk	jjjjjjjj		Store Temp At kkkkkkkk In Temp Vector At: jjjjjjjj
	253		11111101 	kkkkkkkk	jjjjjjjj		Pop and Store Temp At kkkkkkkk In Temp Vector At: jjjjjjjj
	254-255	1111111i	xxxxxxxx	yyyyyyyy	UNASSIGNED

The Cal Primitive Bytecode specifies either a primitive in the primitive table (m=0) or an inlined primitive (m=1). Non-inlined primtiives from the primitive table have index (jjjjjjj * 256) + iiiiiiii and return from the method if they succeed.  This bytecode is only valid as the first bytecode of a method.  Inline primitives have index (jjjjjjj * 256) + iiiiiiii, cannot fail, and do not return when they succeed, yielding a result (typically on top of stack after popping their arguments, but possibly in a byte data stack, for example for unboxed floating-point primitives).

We define the following inlined primitives:
0	unchecked SmallInteger #+.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
1	unchecked SmallInteger #-.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
2	unchecked SmallInteger #*.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
3	unchecked SmallInteger #/.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
4	unchecked SmallInteger #//.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
5	unchecked SmallInteger #\\.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
6	unchecked SmallInteger #rem:.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)

16	unchecked SmallInteger #bitAnd:.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
17	unchecked SmallInteger #bitOr:.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
18	unchecked SmallInteger #bitXor:.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)
19	unchecked SmallInteger #bitShift:.  Both arguments are SmallIntegers and the result fits in a SmallInteger (* depends on word size)

32	unchecked SmallInteger #>.  Both arguments are SmallIntegers
33	unchecked SmallInteger #<.  Both arguments are SmallIntegers
34	unchecked SmallInteger #>=.  Both arguments are SmallIntegers
35	unchecked SmallInteger #<=.  Both arguments are SmallIntegers
36	unchecked SmallInteger #=.  Both arguments are SmallIntegers
37	unchecked SmallInteger #!=.  Both arguments are SmallIntegers

64	unchecked Pointer Object>>at:.		The receiver is guaranteed to be a pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger
65	unchecked Byte Object>>at:.			The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The result is a SmallInteger.
66	unchecked Word Object>>at:.			The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The result is a SmallInteger.
67	unchecked DoubleWord Object>>at:.	The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The result is a SmallInteger or a LargePositiveInteger.
67	unchecked QuadWord Object>>at:.	The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The result is a SmallInteger or a LargePositiveInteger.

80	unchecked Pointer Object>>at:put:.		The receiver is guaranteed to be a pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger
81	unchecked Byte Object>>at:put:.			The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The argument is a SmallInteger.  The primitive stores the least significant 8 bits.
82	unchecked Word Object>>at:put:.			The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The argument is a SmallInteger.  The primitive stores the least significant 16 bits.
83	unchecked DoubleWord Object>>at:put:.	The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The argument is a SmallInteger.  The primitive stores the least significant 32 bits.
83	unchecked QuadWord Object>>at:put:.	The receiver is guaranteed to be a non-pointer object.  The 0-relative (1-relative?) index is an in-range SmallInteger.  The argument is a SmallInteger.  The primitive stores the least significant 64 bits.

-------------------------------------------------------------------------------------------------------------------------------
*/

/**
 * These definitions are used by Lodtalk to implement the bytecode set.
 */

// One byte instructions
SISTAV1_INSTRUCTION_RANGE(PushReceiverVariableShort, 0, 15)
SISTAV1_INSTRUCTION_RANGE(PushLiteralVariableShort, 16, 31)
SISTAV1_INSTRUCTION_RANGE(PushLiteralShort, 32, 63)
SISTAV1_INSTRUCTION_RANGE(PushTempShort, 64, 75)
SISTAV1_INSTRUCTION(PushReceiver, 76)
SISTAV1_INSTRUCTION(PushTrue, 77)
SISTAV1_INSTRUCTION(PushFalse, 78)
SISTAV1_INSTRUCTION(PushNil, 79)
SISTAV1_INSTRUCTION(PushZero, 80)
SISTAV1_INSTRUCTION(PushOne, 81)
SISTAV1_INSTRUCTION(PushThisContext, 82)
SISTAV1_INSTRUCTION(DuplicateStackTop, 83)

SISTAV1_INSTRUCTION(ReturnReceiver, 88)
SISTAV1_INSTRUCTION(ReturnTrue, 89)
SISTAV1_INSTRUCTION(ReturnFalse, 90)
SISTAV1_INSTRUCTION(ReturnNil, 91)
SISTAV1_INSTRUCTION(ReturnTop, 92)
SISTAV1_INSTRUCTION(BlockReturnNil, 93)
SISTAV1_INSTRUCTION(BlockReturnTop, 94)
SISTAV1_INSTRUCTION(Nop, 95)

// Arithmetic messages.
SISTAV1_INSTRUCTION(SpecialMessageAdd, 96)
SISTAV1_INSTRUCTION(SpecialMessageMinus, 97)
SISTAV1_INSTRUCTION(SpecialMessageLessThan, 98)
SISTAV1_INSTRUCTION(SpecialMessageGreaterThan, 99)
SISTAV1_INSTRUCTION(SpecialMessageLessEqual, 100)
SISTAV1_INSTRUCTION(SpecialMessageGreaterEqual, 101)
SISTAV1_INSTRUCTION(SpecialMessageEqual, 102)
SISTAV1_INSTRUCTION(SpecialMessageNotEqual, 103)
SISTAV1_INSTRUCTION(SpecialMessageMultiply, 104)
SISTAV1_INSTRUCTION(SpecialMessageDivide, 105)
SISTAV1_INSTRUCTION(SpecialMessageRemainder, 106)
SISTAV1_INSTRUCTION(SpecialMessageMakePoint, 107)
SISTAV1_INSTRUCTION(SpecialMessageBitShift, 108)
SISTAV1_INSTRUCTION(SpecialMessageIntegerDivision, 109)
SISTAV1_INSTRUCTION(SpecialMessageBitAnd, 110)
SISTAV1_INSTRUCTION(SpecialMessageBitOr, 111)

// Object accessing
SISTAV1_INSTRUCTION(SpecialMessageAt, 112)
SISTAV1_INSTRUCTION(SpecialMessageAtPut, 113)
SISTAV1_INSTRUCTION(SpecialMessageSize, 114)
SISTAV1_INSTRUCTION(SpecialMessageNext, 115)
SISTAV1_INSTRUCTION(SpecialMessageNextPut, 116)
SISTAV1_INSTRUCTION(SpecialMessageAtEnd, 117)
SISTAV1_INSTRUCTION(SpecialMessageIdentityEqual, 118)
SISTAV1_INSTRUCTION(SpecialMessageClass, 119)

// Block evaluation
SISTAV1_INSTRUCTION(SpecialMessageValue, 121)
SISTAV1_INSTRUCTION(SpecialMessageValueArg, 122)
SISTAV1_INSTRUCTION(SpecialMessageDo, 123)

// Object instantiation
SISTAV1_INSTRUCTION(SpecialMessageNew, 124)
SISTAV1_INSTRUCTION(SpecialMessageNewArray, 125)
SISTAV1_INSTRUCTION(SpecialMessageX, 126)
SISTAV1_INSTRUCTION(SpecialMessageY, 127)

SISTAV1_INSTRUCTION_RANGE(SendShortArgs0, 128, 143)
SISTAV1_INSTRUCTION_RANGE(SendShortArgs1, 144, 159)
SISTAV1_INSTRUCTION_RANGE(SendShortArgs2, 160, 175)

SISTAV1_INSTRUCTION_RANGE(JumpShort, 176, 183)
SISTAV1_INSTRUCTION_RANGE(JumpOnTrueShort, 184, 191)
SISTAV1_INSTRUCTION_RANGE(JumpOnFalseShort, 192, 199)
SISTAV1_INSTRUCTION_RANGE(PopStoreReceiverVariableShort, 200, 207)
SISTAV1_INSTRUCTION_RANGE(PopStoreTemporalVariableShort, 208, 215)
SISTAV1_INSTRUCTION(PopStackTop, 216)

// 2 Byte instructions
SISTAV1_INSTRUCTION(ExtendA, 224)
SISTAV1_INSTRUCTION(ExtendB, 225)
SISTAV1_INSTRUCTION(PushReceiverVariable, 226)
SISTAV1_INSTRUCTION(PushLiteralVariable, 227)
SISTAV1_INSTRUCTION(PushLiteral, 228)
SISTAV1_INSTRUCTION(PushTemporary, 229)
SISTAV1_INSTRUCTION(PushNTemps, 230)
SISTAV1_INSTRUCTION(PushInteger, 231)
SISTAV1_INSTRUCTION(PushCharacter, 232)
SISTAV1_INSTRUCTION(PushArrayWithElements, 233)
SISTAV1_INSTRUCTION(Send, 234)
SISTAV1_INSTRUCTION(SuperSend, 235)
SISTAV1_INSTRUCTION(TrapOnBehavior, 236)
SISTAV1_INSTRUCTION(Jump, 237)
SISTAV1_INSTRUCTION(JumpOnTrue, 238)
SISTAV1_INSTRUCTION(JumpOnFalse, 239)
SISTAV1_INSTRUCTION(PopStoreReceiverVariable, 240)
SISTAV1_INSTRUCTION(PopStoreLiteralVariable, 241)
SISTAV1_INSTRUCTION(PopStoreTemporalVariable, 242)
SISTAV1_INSTRUCTION(StoreReceiverVariable, 243)
SISTAV1_INSTRUCTION(StoreLiteralVariable, 244)
SISTAV1_INSTRUCTION(StoreTemporalVariable, 245)

// 3 Byte instructions
SISTAV1_INSTRUCTION(CallPrimitive, 248)
SISTAV1_INSTRUCTION(PushClosure, 250)
SISTAV1_INSTRUCTION(PushTemporaryInVector, 251)
SISTAV1_INSTRUCTION(StoreTemporalInVector, 252)
SISTAV1_INSTRUCTION(PopStoreTemporalInVector, 253)
