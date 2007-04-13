// Copyright (c) 2006, Rusty Wagner
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that
// the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this list of conditions and the
//      following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
//      the following disclaimer in the documentation and/or other materials provided with the distribution.
//    * Neither the name of the author nor the names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stddef.h>
#include "asmx86.h"

#ifdef WIN32
#define restrict
#else
#define __fastcall
#endif

#define DEC_FLAG_LOCK					0x0020
#define DEC_FLAG_REP					0x0040
#define DEC_FLAG_REP_COND				0x0080
#define DEC_FLAG_BYTE					0x0100
#define DEC_FLAG_FLIP_OPERANDS			0x0200
#define DEC_FLAG_IMM_SX					0x0400
#define DEC_FLAG_OPERATION_OP_SIZE		0x1000
#define DEC_FLAG_FORCE_16BIT			0x2000
#define DEC_FLAG_INVALID_IN_64BIT		0x4000
#define DEC_FLAG_DEFAULT_TO_64BIT		0x8000

#define DEC_FLAG_REG_RM_SIZE_MASK		0x03
#define DEC_FLAG_REG_RM_2X_SIZE			0x01
#define DEC_FLAG_REG_RM_FAR_SIZE		0x02
#define DEC_FLAG_REG_RM_NO_SIZE			0x03


#ifdef __cplusplus
namespace x86
{
#endif
	enum RepPrefix
	{
		REP_PREFIX_NONE = 0,
		REP_PREFIX_REPNE,
		REP_PREFIX_REPE
	};
#ifndef __cplusplus
	typedef enum RepPrefix RepPrefix;
#endif

	struct DecodeState
	{
		Instruction* result;
		InstructionOperand* operand0;
		InstructionOperand* operand1;
		const uint8* opcodeStart;
		const uint8* opcode;
		uint64 addr;
		size_t len;
		uint8 opSize, finalOpSize, addrSize;
		uint32 flags;
		bool invalid;
		bool opPrefix;
		RepPrefix rep;
		bool using64, rex;
		bool rexRM1, rexRM2, rexReg;
		int64* ripRelFixup;
	};
#ifndef __cplusplus
	typedef struct DecodeState DecodeState;
#endif

#include "asmx86str.h"

	typedef void (__fastcall *DecodingFunction)(DecodeState* restrict state);

	static void __fastcall InvalidDecode(DecodeState* restrict state);
	static void __fastcall DecodeTwoByte(DecodeState* restrict state);
	static void __fastcall DecodeFpu(DecodeState* restrict state);
	static void __fastcall DecodeNoOperands(DecodeState* restrict state);
	static void __fastcall DecodeRegRM(DecodeState* restrict state);
	static void __fastcall DecodeRegRMImm(DecodeState* restrict state);
	static void __fastcall DecodeRMRegImm8(DecodeState* restrict state);
	static void __fastcall DecodeRMRegCL(DecodeState* restrict state);
	static void __fastcall DecodeEaxImm(DecodeState* restrict state);
	static void __fastcall DecodePushPopSeg(DecodeState* restrict state);
	static void __fastcall DecodeOpReg(DecodeState* restrict state);
	static void __fastcall DecodeEaxOpReg(DecodeState* restrict state);
	static void __fastcall DecodeOpRegImm(DecodeState* restrict state);
	static void __fastcall DecodeNop(DecodeState* restrict state);
	static void __fastcall DecodeImm(DecodeState* restrict state);
	static void __fastcall DecodeImm16Imm8(DecodeState* restrict state);
	static void __fastcall DecodeEdiDx(DecodeState* restrict state);
	static void __fastcall DecodeDxEsi(DecodeState* restrict state);
	static void __fastcall DecodeRelImm(DecodeState* restrict state);
	static void __fastcall DecodeRelImmAddrSize(DecodeState* restrict state);
	static void __fastcall DecodeGroupRM(DecodeState* restrict state);
	static void __fastcall DecodeGroupRMImm(DecodeState* restrict state);
	static void __fastcall DecodeGroupRMImm8V(DecodeState* restrict state);
	static void __fastcall DecodeGroupRMOne(DecodeState* restrict state);
	static void __fastcall DecodeGroupRMCl(DecodeState* restrict state);
	static void __fastcall DecodeGroupF6F7(DecodeState* restrict state);
	static void __fastcall DecodeGroupFF(DecodeState* restrict state);
	static void __fastcall DecodeGroup0F00(DecodeState* restrict state);
	static void __fastcall DecodeGroup0F01(DecodeState* restrict state);
	static void __fastcall DecodeRMSRegV(DecodeState* restrict state);
	static void __fastcall DecodeRM8(DecodeState* restrict state);
	static void __fastcall DecodeRMV(DecodeState* restrict state);
	static void __fastcall DecodeFarImm(DecodeState* restrict state);
	static void __fastcall DecodeEaxAddr(DecodeState* restrict state);
	static void __fastcall DecodeEdiEsi(DecodeState* restrict state);
	static void __fastcall DecodeEdiEax(DecodeState* restrict state);
	static void __fastcall DecodeEaxEsi(DecodeState* restrict state);
	static void __fastcall DecodeAlEbxAl(DecodeState* restrict state);
	static void __fastcall DecodeEaxImm8(DecodeState* restrict state);
	static void __fastcall DecodeEaxDx(DecodeState* restrict state);
	static void __fastcall Decode3DNow(DecodeState* restrict state);
	static void __fastcall DecodeSSEHalf128(DecodeState* restrict state);
	static void __fastcall DecodeSSEHalf64And128(DecodeState* restrict state);
	static void __fastcall DecodeSSEHalfMem64(DecodeState* restrict state);
	static void __fastcall DecodeMovUps(DecodeState* restrict state);
	static void __fastcall DecodeMovLps(DecodeState* restrict state);
	static void __fastcall DecodeMovHps(DecodeState* restrict state);
	static void __fastcall DecodeRegCR(DecodeState* restrict state);
	static void __fastcall DecodeMovSXZX8(DecodeState* restrict state);
	static void __fastcall DecodeMovSXZX16(DecodeState* restrict state);
	static void __fastcall DecodeMem16(DecodeState* restrict state);
	static void __fastcall DecodeMem32(DecodeState* restrict state);
	static void __fastcall DecodeMem64(DecodeState* restrict state);
	static void __fastcall DecodeMem80(DecodeState* restrict state);
	static void __fastcall DecodeMemFloatEnv(DecodeState* restrict state);
	static void __fastcall DecodeMemFloatSave(DecodeState* restrict state);
	static void __fastcall DecodeFPUReg(DecodeState* restrict state);
	static void __fastcall DecodeFPURegST0(DecodeState* restrict state);
	static void __fastcall DecodeRegGroupNoOperands(DecodeState* restrict state);
	static void __fastcall DecodeRegGroupAX(DecodeState* restrict state);
	static void __fastcall DecodeCmpXch8B(DecodeState* restrict state);


	enum EncodingType
	{
		ENC_INVALID = 0,
		ENC_TWO_BYTE, ENC_FPU,
		ENC_NO_OPERANDS, ENC_OP_SIZE, ENC_OP_SIZE_DEF64, ENC_OP_SIZE_NO64,
		ENC_REG_RM_8, ENC_RM_REG_8, ENC_RM_REG_8_LOCK,
		ENC_RM_REG_16,
		ENC_REG_RM_V, ENC_RM_REG_V, ENC_RM_REG_V_LOCK,
		ENC_REG_RM2X_V, ENC_REG_RM_IMM_V, ENC_REG_RM_IMMSX_V,
		ENC_REG_RM_0, ENC_REG_RM_F,
		ENC_RM_REG_IMM8_V, ENC_RM_REG_CL_V,
		ENC_EAX_IMM_8, ENC_EAX_IMM_V,
		ENC_PUSH_POP_SEG,
		ENC_OP_REG_V, ENC_OP_REG_V_DEF64, ENC_EAX_OP_REG_V,
		ENC_OP_REG_IMM_8, ENC_OP_REG_IMM_V,
		ENC_NOP,
		ENC_IMM_V_DEF64, ENC_IMMSX_V_DEF64,
		ENC_IMM_8, ENC_IMM_16, ENC_IMM16_IMM8,
		ENC_EDI_DX_8_REP, ENC_EDI_DX_OP_SIZE_REP, ENC_DX_ESI_8_REP, ENC_DX_ESI_OP_SIZE_REP,
		ENC_RELIMM_8_DEF64, ENC_RELIMM_V_DEF64,
		ENC_RELIMM_8_ADDR_SIZE_DEF64,
		ENC_GROUP_RM_8, ENC_GROUP_RM_V, ENC_GROUP_RM_8_LOCK,
		ENC_GROUP_RM_0,
		ENC_GROUP_RM_IMM_8, ENC_GROUP_RM_IMM_8_LOCK,
		ENC_GROUP_RM_IMM_8_NO64_LOCK, ENC_GROUP_RM_IMM8_V,
		ENC_GROUP_RM_IMM_V, ENC_GROUP_RM_IMM_V_LOCK,
		ENC_GROUP_RM_IMMSX_V_LOCK,
		ENC_GROUP_RM_ONE_8, ENC_GROUP_RM_ONE_V,
		ENC_GROUP_RM_CL_8, ENC_GROUP_RM_CL_V,
		ENC_GROUP_F6, ENC_GROUP_F7, ENC_GROUP_FF,
		ENC_GROUP_0F00, ENC_GROUP_0F01,
		ENC_RM_SREG_V, ENC_SREG_RM_V,
		ENC_RM_8, ENC_RM_V_DEF64,
		ENC_FAR_IMM_NO64,
		ENC_EAX_ADDR_8, ENC_EAX_ADDR_V, ENC_ADDR_EAX_8, ENC_ADDR_EAX_V,
		ENC_EDI_ESI_8_REP, ENC_EDI_ESI_OP_SIZE_REP, ENC_ESI_EDI_8_REPC, ENC_ESI_EDI_OP_SIZE_REPC,
		ENC_EDI_EAX_8_REP, ENC_EDI_EAX_OP_SIZE_REP, ENC_EAX_ESI_8_REP, ENC_EAX_ESI_OP_SIZE_REP,
		ENC_EAX_EDI_8_REPC, ENC_EAX_EDI_OP_SIZE_REPC,
		ENC_AL_EBX_AL,
		ENC_EAX_IMM8_8, ENC_EAX_IMM8_V, ENC_IMM8_EAX_8, ENC_IMM8_EAX_V,
		ENC_EAX_DX_8, ENC_EAX_DX_V, ENC_DX_EAX_8, ENC_DX_EAX_V,
		ENC_3DNOW,
		ENC_SSE_HALF_128, ENC_SSE_HALF_64_128, ENC_SSE_HALF_MEM_64,
		ENC_MOVUPS, ENC_MOVUPS_FLIP,
		ENC_MOVLPS, ENC_MOVHPS,
		ENC_REG_CR, ENC_CR_REG,
		ENC_MOVSXZX_8, ENC_MOVSXZX_16,
		ENC_MEM_16, ENC_MEM_32, ENC_MEM_64, ENC_MEM_80,
		ENC_MEM_FLOATENV, ENC_MEM_FLOATSAVE,
		ENC_FPUREG, ENC_ST0_FPUREG, ENC_FPUREG_ST0,
		ENC_REGGROUP_NO_OPERANDS, ENC_REGGROUP_AX,
		ENC_CMPXCH8B,
		ENC_COUNT
	};
#ifndef __cplusplus
	typedef enum EncodingType EncodingType;
#endif


	struct DecodeDef
	{
		DecodingFunction func;
		uint16 flags;
	};
#ifndef __cplusplus
	typedef struct DecodeDef DecodeDef;
#endif


	static const DecodeDef decoders[ENC_COUNT] =
	{
		{InvalidDecode, 0},
		{DecodeTwoByte, 0}, {DecodeFpu, 0},
		{DecodeNoOperands, 0}, {DecodeNoOperands, DEC_FLAG_OPERATION_OP_SIZE},
		{DecodeNoOperands, DEC_FLAG_DEFAULT_TO_64BIT | DEC_FLAG_OPERATION_OP_SIZE},
		{DecodeNoOperands, DEC_FLAG_INVALID_IN_64BIT | DEC_FLAG_OPERATION_OP_SIZE},
		{DecodeRegRM, DEC_FLAG_BYTE}, {DecodeRegRM, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS},
		{DecodeRegRM, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_LOCK},
		{DecodeRegRM, DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_FORCE_16BIT},
		{DecodeRegRM, 0}, {DecodeRegRM, DEC_FLAG_FLIP_OPERANDS}, {DecodeRegRM, DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_LOCK},
		{DecodeRegRM, DEC_FLAG_REG_RM_2X_SIZE}, {DecodeRegRMImm, 0}, {DecodeRegRMImm, DEC_FLAG_IMM_SX},
		{DecodeRegRM, DEC_FLAG_REG_RM_NO_SIZE}, {DecodeRegRM, DEC_FLAG_REG_RM_FAR_SIZE},
		{DecodeRMRegImm8, 0}, {DecodeRMRegCL, 0},
		{DecodeEaxImm, DEC_FLAG_BYTE}, {DecodeEaxImm, 0},
		{DecodePushPopSeg, 0},
		{DecodeOpReg, 0}, {DecodeOpReg, DEC_FLAG_DEFAULT_TO_64BIT}, {DecodeEaxOpReg, 0},
		{DecodeOpRegImm, DEC_FLAG_BYTE}, {DecodeOpRegImm, 0},
		{DecodeNop, 0},
		{DecodeImm, DEC_FLAG_DEFAULT_TO_64BIT}, {DecodeImm, DEC_FLAG_IMM_SX | DEC_FLAG_DEFAULT_TO_64BIT},
		{DecodeImm, DEC_FLAG_BYTE}, {DecodeImm, DEC_FLAG_FORCE_16BIT}, {DecodeImm16Imm8, 0},
		{DecodeEdiDx, DEC_FLAG_BYTE | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP}, {DecodeEdiDx, DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP},
		{DecodeDxEsi, DEC_FLAG_BYTE | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP}, {DecodeDxEsi, DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP},
		{DecodeRelImm, DEC_FLAG_BYTE | DEC_FLAG_DEFAULT_TO_64BIT}, {DecodeRelImm, DEC_FLAG_DEFAULT_TO_64BIT},
		{DecodeRelImmAddrSize, DEC_FLAG_BYTE | DEC_FLAG_DEFAULT_TO_64BIT},
		{DecodeGroupRM, DEC_FLAG_BYTE}, {DecodeGroupRM, 0}, {DecodeGroupRM, DEC_FLAG_BYTE | DEC_FLAG_LOCK},
		{DecodeGroupRM, DEC_FLAG_REG_RM_NO_SIZE},
		{DecodeGroupRMImm, DEC_FLAG_BYTE}, {DecodeGroupRMImm, DEC_FLAG_BYTE | DEC_FLAG_LOCK},
		{DecodeGroupRMImm, DEC_FLAG_BYTE | DEC_FLAG_INVALID_IN_64BIT | DEC_FLAG_LOCK},
		{DecodeGroupRMImm8V, 0},
		{DecodeGroupRMImm, 0}, {DecodeGroupRMImm, DEC_FLAG_LOCK}, {DecodeGroupRMImm, DEC_FLAG_IMM_SX | DEC_FLAG_LOCK},
		{DecodeGroupRMOne, DEC_FLAG_BYTE}, {DecodeGroupRMOne, 0},
		{DecodeGroupRMCl, DEC_FLAG_BYTE}, {DecodeGroupRMCl, 0},
		{DecodeGroupF6F7, DEC_FLAG_BYTE | DEC_FLAG_LOCK}, {DecodeGroupF6F7, DEC_FLAG_LOCK}, {DecodeGroupFF, DEC_FLAG_LOCK},
		{DecodeGroup0F00, 0}, {DecodeGroup0F01, 0},
		{DecodeRMSRegV, 0}, {DecodeRMSRegV, DEC_FLAG_FLIP_OPERANDS},
		{DecodeRM8, 0}, {DecodeRMV, DEC_FLAG_DEFAULT_TO_64BIT},
		{DecodeFarImm, DEC_FLAG_INVALID_IN_64BIT},
		{DecodeEaxAddr, DEC_FLAG_BYTE}, {DecodeEaxAddr, 0},
		{DecodeEaxAddr, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS}, {DecodeEaxAddr, DEC_FLAG_FLIP_OPERANDS},
		{DecodeEdiEsi, DEC_FLAG_BYTE | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP}, {DecodeEdiEsi, DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP},
		{DecodeEdiEsi, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP_COND},
		{DecodeEdiEsi, DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP_COND},
		{DecodeEdiEax, DEC_FLAG_BYTE | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP}, {DecodeEdiEax, DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP},
		{DecodeEaxEsi, DEC_FLAG_BYTE | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP}, {DecodeEaxEsi, DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP},
		{DecodeEdiEax, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP_COND},
		{DecodeEdiEax, DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_OPERATION_OP_SIZE | DEC_FLAG_REP_COND},
		{DecodeAlEbxAl, 0},
		{DecodeEaxImm8, DEC_FLAG_BYTE}, {DecodeEaxImm8, 0},
		{DecodeEaxImm8, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS}, {DecodeEaxImm8, DEC_FLAG_FLIP_OPERANDS},
		{DecodeEaxDx, DEC_FLAG_BYTE}, {DecodeEaxDx, 0},
		{DecodeEaxDx, DEC_FLAG_BYTE | DEC_FLAG_FLIP_OPERANDS}, {DecodeEaxDx, DEC_FLAG_FLIP_OPERANDS},
		{Decode3DNow, 0},
		{DecodeSSEHalf128, 0}, {DecodeSSEHalf64And128, 0}, {DecodeSSEHalfMem64, 0},
		{DecodeMovUps, 0}, {DecodeMovUps, DEC_FLAG_FLIP_OPERANDS},
		{DecodeMovLps, 0}, {DecodeMovHps, 0},
		{DecodeRegCR, DEC_FLAG_DEFAULT_TO_64BIT | DEC_FLAG_LOCK},
		{DecodeRegCR, DEC_FLAG_FLIP_OPERANDS | DEC_FLAG_DEFAULT_TO_64BIT | DEC_FLAG_LOCK},
		{DecodeMovSXZX8, 0}, {DecodeMovSXZX16, 0},
		{DecodeMem16, 0}, {DecodeMem32, 0}, {DecodeMem64, 0}, {DecodeMem80, 0},
		{DecodeMemFloatEnv, 0}, {DecodeMemFloatSave, 0},
		{DecodeFPUReg, 0}, {DecodeFPURegST0, DEC_FLAG_FLIP_OPERANDS}, {DecodeFPURegST0, 0},
		{DecodeRegGroupNoOperands, 0}, {DecodeRegGroupAX, 0},
		{DecodeCmpXch8B, 0},
	};


	struct InstructionEncoding
	{
		uint16 operation : 9;
		uint16 encoding : 7;
	};
#ifndef __cplusplus
	typedef struct InstructionEncoding InstructionEncoding;
#endif


	static const InstructionEncoding mainOpcodeMap[256] =
	{
		{ADD, ENC_RM_REG_8_LOCK}, {ADD, ENC_RM_REG_V_LOCK}, {ADD, ENC_REG_RM_8}, {ADD, ENC_REG_RM_V}, // 0x00
		{ADD, ENC_EAX_IMM_8}, {ADD, ENC_EAX_IMM_V}, {PUSH, ENC_PUSH_POP_SEG}, {POP, ENC_PUSH_POP_SEG}, // 0x04
		{OR, ENC_RM_REG_8_LOCK}, {OR, ENC_RM_REG_V_LOCK}, {OR, ENC_REG_RM_8}, {OR, ENC_REG_RM_V}, // 0x08
		{OR, ENC_EAX_IMM_8}, {OR, ENC_EAX_IMM_V}, {PUSH, ENC_PUSH_POP_SEG}, {INVALID, ENC_TWO_BYTE}, // 0x0c
		{ADC, ENC_RM_REG_8_LOCK}, {ADC, ENC_RM_REG_V_LOCK}, {ADC, ENC_REG_RM_8}, {ADC, ENC_REG_RM_V}, // 0x10
		{ADC, ENC_EAX_IMM_8}, {ADC, ENC_EAX_IMM_V}, {PUSH, ENC_PUSH_POP_SEG}, {POP, ENC_PUSH_POP_SEG}, // 0x14
		{SBB, ENC_RM_REG_8_LOCK}, {SBB, ENC_RM_REG_V_LOCK}, {SBB, ENC_REG_RM_8}, {SBB, ENC_REG_RM_V}, // 0x18
		{SBB, ENC_EAX_IMM_8}, {SBB, ENC_EAX_IMM_V}, {PUSH, ENC_PUSH_POP_SEG}, {POP, ENC_PUSH_POP_SEG}, // 0x1c
		{AND, ENC_RM_REG_8_LOCK}, {AND, ENC_RM_REG_V_LOCK}, {AND, ENC_REG_RM_8}, {AND, ENC_REG_RM_V}, // 0x20
		{AND, ENC_EAX_IMM_8}, {AND, ENC_EAX_IMM_V}, {INVALID, ENC_INVALID}, {DAA, ENC_NO_OPERANDS}, // 0x24
		{SUB, ENC_RM_REG_8_LOCK}, {SUB, ENC_RM_REG_V_LOCK}, {SUB, ENC_REG_RM_8}, {SUB, ENC_REG_RM_V}, // 0x28
		{SUB, ENC_EAX_IMM_8}, {SUB, ENC_EAX_IMM_V}, {INVALID, ENC_INVALID}, {DAS, ENC_NO_OPERANDS}, // 0x2c
		{XOR, ENC_RM_REG_8_LOCK}, {XOR, ENC_RM_REG_V_LOCK}, {XOR, ENC_REG_RM_8}, {XOR, ENC_REG_RM_V}, // 0x30
		{XOR, ENC_EAX_IMM_8}, {XOR, ENC_EAX_IMM_V}, {INVALID, ENC_INVALID}, {AAA, ENC_NO_OPERANDS}, // 0x34
		{CMP, ENC_RM_REG_8}, {CMP, ENC_RM_REG_V}, {CMP, ENC_REG_RM_8}, {CMP, ENC_REG_RM_V}, // 0x38
		{CMP, ENC_EAX_IMM_8}, {CMP, ENC_EAX_IMM_V}, {INVALID, ENC_INVALID}, {AAS, ENC_NO_OPERANDS}, // 0x3c
		{INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, // 0x40
		{INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, {INC, ENC_OP_REG_V}, // 0x44
		{DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, // 0x48
		{DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, {DEC, ENC_OP_REG_V}, // 0x4c
		{PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, // 0x50
		{PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, {PUSH, ENC_OP_REG_V_DEF64}, // 0x54
		{POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, // 0x58
		{POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, {POP, ENC_OP_REG_V_DEF64}, // 0x5c
		{PUSHA, ENC_OP_SIZE_NO64}, {POPA, ENC_OP_SIZE_NO64}, {BOUND, ENC_REG_RM2X_V}, {ARPL, ENC_RM_REG_16}, // 0x60
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x64
		{PUSH, ENC_IMM_V_DEF64}, {IMUL, ENC_REG_RM_IMM_V}, {PUSH, ENC_IMMSX_V_DEF64}, {IMUL, ENC_REG_RM_IMMSX_V}, // 0x68
		{INSB, ENC_EDI_DX_8_REP}, {INSW, ENC_EDI_DX_OP_SIZE_REP}, {OUTSB, ENC_DX_ESI_8_REP}, {OUTSW, ENC_DX_ESI_OP_SIZE_REP}, // 0x6c
		{JO, ENC_RELIMM_8_DEF64}, {JNO, ENC_RELIMM_8_DEF64}, {JB, ENC_RELIMM_8_DEF64}, {JAE, ENC_RELIMM_8_DEF64}, // 0x70
		{JE, ENC_RELIMM_8_DEF64}, {JNE, ENC_RELIMM_8_DEF64}, {JBE, ENC_RELIMM_8_DEF64}, {JA, ENC_RELIMM_8_DEF64}, // 0x74
		{JS, ENC_RELIMM_8_DEF64}, {JNS, ENC_RELIMM_8_DEF64}, {JPE, ENC_RELIMM_8_DEF64}, {JPO, ENC_RELIMM_8_DEF64}, // 0x78
		{JL, ENC_RELIMM_8_DEF64}, {JGE, ENC_RELIMM_8_DEF64}, {JLE, ENC_RELIMM_8_DEF64}, {JG, ENC_RELIMM_8_DEF64}, // 0x7c
		{0, ENC_GROUP_RM_IMM_8_LOCK}, {0, ENC_GROUP_RM_IMM_V_LOCK}, {0, ENC_GROUP_RM_IMM_8_NO64_LOCK}, {0, ENC_GROUP_RM_IMMSX_V_LOCK}, // 0x80
		{TEST, ENC_RM_REG_8}, {TEST, ENC_RM_REG_V}, {XCHG, ENC_RM_REG_8_LOCK}, {XCHG, ENC_RM_REG_V_LOCK}, // 0x84
		{MOV, ENC_RM_REG_8}, {MOV, ENC_RM_REG_V}, {MOV, ENC_REG_RM_8}, {MOV, ENC_REG_RM_V}, // 0x88
		{MOV, ENC_RM_SREG_V}, {LEA, ENC_REG_RM_0}, {MOV, ENC_SREG_RM_V}, {POP, ENC_RM_V_DEF64}, // 0x8c
		{NOP, ENC_NOP}, {XCHG, ENC_EAX_OP_REG_V}, {XCHG, ENC_EAX_OP_REG_V}, {XCHG, ENC_EAX_OP_REG_V}, // 0x90
		{XCHG, ENC_EAX_OP_REG_V}, {XCHG, ENC_EAX_OP_REG_V}, {XCHG, ENC_EAX_OP_REG_V}, {XCHG, ENC_EAX_OP_REG_V}, // 0x94
		{CBW, ENC_OP_SIZE}, {CWD, ENC_OP_SIZE}, {CALLF, ENC_FAR_IMM_NO64}, {FWAIT, ENC_NO_OPERANDS}, // 0x98
		{PUSHF, ENC_OP_SIZE_DEF64}, {POPF, ENC_OP_SIZE_DEF64}, {SAHF, ENC_NO_OPERANDS}, {LAHF, ENC_NO_OPERANDS}, // 0x9c
		{MOV, ENC_EAX_ADDR_8}, {MOV, ENC_EAX_ADDR_V}, {MOV, ENC_ADDR_EAX_8}, {MOV, ENC_ADDR_EAX_V}, // 0xa0
		{MOVSB, ENC_EDI_ESI_8_REP}, {MOVSW, ENC_EDI_ESI_OP_SIZE_REP}, {CMPSB, ENC_ESI_EDI_8_REPC}, {CMPSW, ENC_ESI_EDI_OP_SIZE_REPC}, // 0xa4
		{TEST, ENC_EAX_IMM_8}, {TEST, ENC_EAX_IMM_V}, {STOSB, ENC_EDI_EAX_8_REP}, {STOSW, ENC_EDI_EAX_OP_SIZE_REP}, // 0xa8
		{LODSB, ENC_EAX_ESI_8_REP}, {LODSW, ENC_EAX_ESI_OP_SIZE_REP}, {SCASB, ENC_EAX_EDI_8_REPC}, {SCASW, ENC_EAX_EDI_OP_SIZE_REPC}, // 0xac
		{MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, // 0xb0
		{MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, {MOV, ENC_OP_REG_IMM_8}, // 0xb4
		{MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, // 0xb8
		{MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, {MOV, ENC_OP_REG_IMM_V}, // 0xbc
		{1, ENC_GROUP_RM_IMM_8}, {1, ENC_GROUP_RM_IMM8_V}, {RETN, ENC_IMM_16}, {RETN, ENC_NO_OPERANDS}, // 0xc0
		{LES, ENC_REG_RM_F}, {LDS, ENC_REG_RM_F}, {2, ENC_GROUP_RM_IMM_8}, {2, ENC_GROUP_RM_IMM_V}, // 0xc4
		{ENTER, ENC_IMM16_IMM8}, {LEAVE, ENC_NO_OPERANDS}, {RETF, ENC_IMM_16}, {RETF, ENC_NO_OPERANDS}, // 0xc8
		{INT3, ENC_NO_OPERANDS}, {INT, ENC_IMM_8}, {INTO, ENC_NO_OPERANDS}, {IRET, ENC_NO_OPERANDS}, // 0xcc
		{1, ENC_GROUP_RM_ONE_8}, {1, ENC_GROUP_RM_ONE_V}, {1, ENC_GROUP_RM_CL_8}, {1, ENC_GROUP_RM_CL_V}, // 0xd0
		{AAM, ENC_IMM_8}, {AAD, ENC_IMM_8}, {INVALID, ENC_INVALID}, {XLAT, ENC_AL_EBX_AL}, // 0xd4
		{0, ENC_FPU}, {1, ENC_FPU}, {2, ENC_FPU}, {3, ENC_FPU}, // 0xd8
		{4, ENC_FPU}, {5, ENC_FPU}, {6, ENC_FPU}, {7, ENC_FPU}, // 0xdc
		{LOOPNE, ENC_RELIMM_8_DEF64}, {LOOPE, ENC_RELIMM_8_DEF64}, {LOOP, ENC_RELIMM_8_DEF64}, {JCXZ, ENC_RELIMM_8_ADDR_SIZE_DEF64}, // 0xe0
		{IN, ENC_EAX_IMM8_8}, {IN, ENC_EAX_IMM8_V}, {OUT, ENC_IMM8_EAX_8}, {OUT, ENC_IMM8_EAX_V}, // 0xe4
		{CALLN, ENC_RELIMM_V_DEF64}, {JMPN, ENC_RELIMM_V_DEF64}, {JMPF, ENC_FAR_IMM_NO64}, {JMPN, ENC_RELIMM_8_DEF64}, // 0xe8
		{IN, ENC_EAX_DX_8}, {IN, ENC_EAX_DX_V}, {OUT, ENC_DX_EAX_8}, {OUT, ENC_DX_EAX_V}, // 0xec
		{INVALID, ENC_INVALID}, {INT1, ENC_NO_OPERANDS}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xf0
		{HLT, ENC_NO_OPERANDS}, {CMC, ENC_NO_OPERANDS}, {3, ENC_GROUP_F6}, {3, ENC_GROUP_F7}, // 0xf4
		{CLC, ENC_NO_OPERANDS}, {STC, ENC_NO_OPERANDS}, {CLI, ENC_NO_OPERANDS}, {STI, ENC_NO_OPERANDS}, // 0xf8
		{CLD, ENC_NO_OPERANDS}, {STD, ENC_NO_OPERANDS}, {4, ENC_GROUP_RM_8_LOCK}, {5, ENC_GROUP_FF}, // 0xfc
	};


	static const InstructionEncoding twoByteOpcodeMap[256] =
	{
		{6, ENC_GROUP_0F00}, {7, ENC_GROUP_0F01}, {LAR, ENC_REG_RM_V}, {LSL, ENC_REG_RM_V}, // 0x00
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {CLTS, ENC_NO_OPERANDS}, {INVALID, ENC_INVALID}, // 0x04
		{INVD, ENC_NO_OPERANDS}, {WBINVD, ENC_NO_OPERANDS}, {INVALID, ENC_INVALID}, {UD2, ENC_NO_OPERANDS}, // 0x08
		{INVALID, ENC_INVALID}, {8, ENC_GROUP_RM_0}, {FEMMS, ENC_NO_OPERANDS}, {0, ENC_3DNOW}, // 0x0c
		{MOVUPS, ENC_MOVUPS}, {MOVUPS, ENC_MOVUPS_FLIP}, {MOVLPS, ENC_MOVLPS}, {MOVLPS, ENC_SSE_HALF_MEM_64}, // 0x10
		{UNPCKLPS, ENC_SSE_HALF_64_128}, {UNPCKHPS, ENC_SSE_HALF_128}, {MOVHPS, ENC_MOVHPS}, {MOVHPS, ENC_SSE_HALF_MEM_64}, // 0x14
		{9, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, // 0x18
		{10, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, {10, ENC_GROUP_RM_0}, // 0x1c
		{REG_CR0, ENC_REG_CR}, {REG_DR0, ENC_REG_CR}, {REG_CR0, ENC_CR_REG}, {REG_DR0, ENC_CR_REG}, // 0x20
		{REG_TR0, ENC_REG_CR}, {INVALID, ENC_INVALID}, {REG_TR0, ENC_CR_REG}, {INVALID, ENC_INVALID}, // 0x24
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x28
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x2c
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x30
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x34
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x38
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x3c
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x40
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x44
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x48
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x4c
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x50
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x54
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x58
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x5c
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x60
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x64
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x68
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x6c
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x70
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x74
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x78
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0x7c
		{JO, ENC_RELIMM_V_DEF64}, {JNO, ENC_RELIMM_V_DEF64}, {JB, ENC_RELIMM_V_DEF64}, {JAE, ENC_RELIMM_V_DEF64}, // 0x80
		{JE, ENC_RELIMM_V_DEF64}, {JNE, ENC_RELIMM_V_DEF64}, {JBE, ENC_RELIMM_V_DEF64}, {JA, ENC_RELIMM_V_DEF64}, // 0x84
		{JS, ENC_RELIMM_V_DEF64}, {JNS, ENC_RELIMM_V_DEF64}, {JPE, ENC_RELIMM_V_DEF64}, {JPO, ENC_RELIMM_V_DEF64}, // 0x88
		{JL, ENC_RELIMM_V_DEF64}, {JGE, ENC_RELIMM_V_DEF64}, {JLE, ENC_RELIMM_V_DEF64}, {JG, ENC_RELIMM_V_DEF64}, // 0x8c
		{SETO, ENC_RM_8}, {SETNO, ENC_RM_8}, {SETB, ENC_RM_8}, {SETAE, ENC_RM_8}, // 0x90
		{SETE, ENC_RM_8}, {SETNE, ENC_RM_8}, {SETBE, ENC_RM_8}, {SETA, ENC_RM_8}, // 0x94
		{SETS, ENC_RM_8}, {SETNS, ENC_RM_8}, {SETPE, ENC_RM_8}, {SETPO, ENC_RM_8}, // 0x98
		{SETL, ENC_RM_8}, {SETGE, ENC_RM_8}, {SETLE, ENC_RM_8}, {SETG, ENC_RM_8}, // 0x9c
		{PUSH, ENC_PUSH_POP_SEG}, {POP, ENC_PUSH_POP_SEG}, {CPUID, ENC_NO_OPERANDS}, {BT, ENC_RM_REG_V}, // 0xa0
		{SHLD, ENC_RM_REG_IMM8_V}, {SHLD, ENC_RM_REG_CL_V}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xa4
		{PUSH, ENC_PUSH_POP_SEG}, {POP, ENC_PUSH_POP_SEG}, {INVALID, ENC_INVALID}, {BTS, ENC_RM_REG_V}, // 0xa8
		{SHRD, ENC_RM_REG_IMM8_V}, {SHRD, ENC_RM_REG_CL_V}, {INVALID, ENC_INVALID}, {IMUL, ENC_REG_RM_V}, // 0xac
		{CMPXCHG, ENC_RM_REG_8_LOCK}, {CMPXCHG, ENC_RM_REG_V_LOCK}, {LSS, ENC_REG_RM_F}, {BTR, ENC_RM_REG_V}, // 0xb0
		{LFS, ENC_REG_RM_F}, {LGS, ENC_REG_RM_F}, {MOVZX, ENC_MOVSXZX_8}, {MOVZX, ENC_MOVSXZX_16}, // 0xb4
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {11, ENC_GROUP_RM_IMM8_V}, {BTC, ENC_RM_REG_V}, // 0xb8
		{BSF, ENC_REG_RM_V}, {BSR, ENC_REG_RM_V}, {MOVSX, ENC_MOVSXZX_8}, {MOVSX, ENC_MOVSXZX_16}, // 0xbc
		{XADD, ENC_RM_REG_8}, {XADD, ENC_RM_REG_V}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xc0
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {CMPXCH8B, ENC_CMPXCH8B}, // 0xc4
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xc8
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xcc
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xd0
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xd4
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xd8
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xdc
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xe0
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xe4
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xe8
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xec
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xf0
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xf4
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0xf8
		{INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID} // 0xfc
	};


	static const InstructionEncoding fpuMemOpcodeMap[8][8] =
	{
		{ // 0xd8
			{FADD, ENC_MEM_32}, {FMUL, ENC_MEM_32}, {FCOM, ENC_MEM_32}, {FCOMP, ENC_MEM_32}, // 0
			{FSUB, ENC_MEM_32}, {FSUBR, ENC_MEM_32}, {FDIV, ENC_MEM_32}, {FDIVR, ENC_MEM_32} // 4
		},
		{ // 0xd9
			{FLD, ENC_MEM_32}, {INVALID, ENC_INVALID}, {FST, ENC_MEM_32}, {FSTP, ENC_MEM_32}, // 0
			{FLDENV, ENC_MEM_FLOATENV}, {FLDCW, ENC_MEM_16}, {FSTENV, ENC_MEM_FLOATENV}, {FSTCW, ENC_MEM_16} // 4
		},
		{ // 0xda
			{FIADD, ENC_MEM_32}, {FIMUL, ENC_MEM_32}, {FICOM, ENC_MEM_32}, {FICOMP, ENC_MEM_32}, // 0
			{FISUB, ENC_MEM_32}, {FISUBR, ENC_MEM_32}, {FIDIV, ENC_MEM_32}, {FIDIVR, ENC_MEM_32} // 4
		},
		{ // 0xdb
			{FILD, ENC_MEM_32}, {FISTTP, ENC_MEM_32}, {FIST, ENC_MEM_32}, {FISTP, ENC_MEM_32}, // 0
			{INVALID, ENC_INVALID}, {FLD, ENC_MEM_80}, {INVALID, ENC_INVALID}, {FSTP, ENC_MEM_80} // 4
		},
		{ // 0xdc
			{FADD, ENC_MEM_64}, {FMUL, ENC_MEM_64}, {FCOM, ENC_MEM_64}, {FCOMP, ENC_MEM_64}, // 0
			{FSUB, ENC_MEM_64}, {FSUBR, ENC_MEM_64}, {FDIV, ENC_MEM_64}, {FDIVR, ENC_MEM_64} // 4
		},
		{ // 0xdd
			{FLD, ENC_MEM_64}, {FISTTP, ENC_MEM_64}, {FST, ENC_MEM_64}, {FSTP, ENC_MEM_64}, // 0
			{FRSTOR, ENC_MEM_FLOATSAVE}, {INVALID, ENC_INVALID}, {FSAVE, ENC_MEM_FLOATSAVE}, {FSTSW, ENC_MEM_16} // 4
		},
		{ // 0xde
			{FIADD, ENC_MEM_16}, {FIMUL, ENC_MEM_16}, {FICOM, ENC_MEM_16}, {FICOMP, ENC_MEM_16}, // 0
			{FISUB, ENC_MEM_16}, {FISUBR, ENC_MEM_16}, {FIDIV, ENC_MEM_16}, {FIDIVR, ENC_MEM_16} // 4
		},
		{ // 0xdf
			{FILD, ENC_MEM_16}, {FISTTP, ENC_MEM_16}, {FIST, ENC_MEM_16}, {FISTP, ENC_MEM_16}, // 0
			{FBLD, ENC_MEM_80}, {FILD, ENC_MEM_64}, {FBSTP, ENC_MEM_80}, {FISTP, ENC_MEM_64}
		}
	};


	static const InstructionEncoding fpuRegOpcodeMap[8][8] =
	{
		{ // 0xd8
			{FADD, ENC_ST0_FPUREG}, {FMUL, ENC_ST0_FPUREG}, {FCOM, ENC_ST0_FPUREG}, {FCOMP, ENC_ST0_FPUREG}, // 0
			{FSUB, ENC_ST0_FPUREG}, {FSUBR, ENC_ST0_FPUREG}, {FDIV, ENC_ST0_FPUREG}, {FDIVR, ENC_ST0_FPUREG} // 4
		},
		{ // 0xd9
			{FLD, ENC_ST0_FPUREG}, {FXCH, ENC_ST0_FPUREG}, {12, ENC_REGGROUP_NO_OPERANDS}, {INVALID, ENC_INVALID}, // 0
			{13, ENC_REGGROUP_NO_OPERANDS}, {14, ENC_REGGROUP_NO_OPERANDS}, {15, ENC_REGGROUP_NO_OPERANDS}, {16, ENC_REGGROUP_NO_OPERANDS} // 4
		},
		{ // 0xda
			{FCMOVB, ENC_ST0_FPUREG}, {FCMOVE, ENC_ST0_FPUREG}, {FCMOVBE, ENC_ST0_FPUREG}, {FCMOVU, ENC_ST0_FPUREG}, // 0
			{INVALID, ENC_INVALID}, {17, ENC_REGGROUP_NO_OPERANDS}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID} // 4
		},
		{ // 0xdb
			{FCMOVNB, ENC_ST0_FPUREG}, {FCMOVNE, ENC_ST0_FPUREG}, {FCMOVNBE, ENC_ST0_FPUREG}, {FCMOVNU, ENC_ST0_FPUREG}, // 0
			{18, ENC_REGGROUP_NO_OPERANDS}, {FUCOMI, ENC_ST0_FPUREG}, {FCOMI, ENC_ST0_FPUREG}, {21, ENC_REGGROUP_NO_OPERANDS} // 4
		},
		{ // 0xdc
			{FADD, ENC_FPUREG_ST0}, {FMUL, ENC_FPUREG_ST0}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0
			{FSUBR, ENC_FPUREG_ST0}, {FSUB, ENC_FPUREG_ST0}, {FDIVR, ENC_FPUREG_ST0}, {FDIV, ENC_FPUREG_ST0} // 4
		},
		{ // 0xdd
			{FFREE, ENC_FPUREG}, {INVALID, ENC_INVALID}, {FST, ENC_FPUREG}, {FSTP, ENC_FPUREG}, // 0
			{FUCOM, ENC_FPUREG_ST0}, {FUCOMP, ENC_FPUREG}, {INVALID, ENC_INVALID}, {22, ENC_REGGROUP_NO_OPERANDS} // 4
		},
		{ // 0xde
			{FADDP, ENC_FPUREG_ST0}, {FMULP, ENC_FPUREG_ST0}, {INVALID, ENC_INVALID}, {19, ENC_REGGROUP_NO_OPERANDS}, // 0
			{FSUBRP, ENC_FPUREG_ST0}, {FSUBP, ENC_FPUREG_ST0}, {FDIVRP, ENC_FPUREG_ST0}, {FDIVP, ENC_FPUREG_ST0} // 4
		},
		{ // 0xdf
			{FFREEP, ENC_FPUREG}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, {INVALID, ENC_INVALID}, // 0
			{20, ENC_REGGROUP_AX}, {FUCOMIP, ENC_ST0_FPUREG}, {FCOMIP, ENC_ST0_FPUREG}, {23, ENC_REGGROUP_NO_OPERANDS} // 4
		}
	};


	static const uint16 groupOperations[24][8] =
	{
		{ADD, OR, ADC, SBB, AND, SUB, XOR, CMP}, // Group 0
		{ROL, ROR, RCL, RCR, SHL, SHR, SHL, SAR}, // Group 1
		{MOV, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 2
		{TEST, INVALID, NOT, NEG, MUL, IMUL, DIV, IDIV}, // Group 3
		{INC, DEC, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 4
		{INC, DEC, CALLN, CALLF, JMPN, JMPF, PUSH, INVALID}, // Group 5
		{SLDT, STR, LLDT, LTR, VERR, VERW, INVALID, INVALID}, // Group 6
		{SGDT, SIDT, LGDT, LIDT, SMSW, INVALID, LMSW, INVLPG}, // Group 7
		{PREFETCH, PREFETCHW, PREFETCH, PREFETCH, PREFETCH, PREFETCH, PREFETCH, PREFETCH}, // Group 8
		{PREFETCHNTA, PREFETCHT0, PREFETCHT1, PREFETCHT2, MMXNOP, MMXNOP, MMXNOP, MMXNOP}, // Group 9
		{MMXNOP, MMXNOP, MMXNOP, MMXNOP, MMXNOP, MMXNOP, MMXNOP, MMXNOP}, // Group 10
		{INVALID, INVALID, INVALID, INVALID, BT, BTS, BTR, BTC}, // Group 11
		{FNOP, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 12
		{FCHS, FABS, INVALID, INVALID, FTST, FXAM, INVALID, INVALID}, // Group 13
		{FLD1, FLDL2T, FLDL2E, FLDPI, FLDLG2, FLDLN2, FLDZ, INVALID}, // Group 14
		{F2XM1, FYL2X, FPTAN, FPATAN, FXTRACT, FPREM1, FDECSTP, FINCSTP}, // Group 15
		{FPREM, FYL2XP1, FSQRT, FSINCOS, FRNDINT, FSCALE, FSIN, FCOS}, // Group 16
		{INVALID, FUCOMPP, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 17
		{FENI, FDISI, FCLEX, FINIT, FSETPM, FRSTPM, INVALID, INVALID}, // Group 18
		{INVALID, FCOMPP, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 19
		{FSTSW, FSTDW, FSTSG, INVALID, INVALID, INVALID, INVALID, INVALID}, // Group 20
		{INVALID, INVALID, INVALID, INVALID, FRINT2, INVALID, INVALID, INVALID}, // Group 21
		{INVALID, INVALID, INVALID, INVALID, FRICHOP, INVALID, INVALID, INVALID}, // Group 22
		{INVALID, INVALID, INVALID, INVALID, FRINEAR, INVALID, INVALID, INVALID}, // Group 23
	};


	struct SparseOpEntry
	{
		uint8 opcode;
		uint16 operation;
	};
#ifndef __cplusplus
	typedef struct SparseOpEntry SparseOpEntry;
#endif

	static const SparseOpEntry sparse3DNowOpcodes[] =
	{
		{0x0c, PI2FW}, {0x0d, PI2FD},
		{0x1c, PF2IW}, {0x1d, PF2ID},
		{0x86, PFRCPV}, {0x87, PFRSQRTV}, {0x8a, PFNACC}, {0x8e, PFPNACC},
		{0x90, PFCMPGE}, {0x94, PFMIN}, {0x96, PFRCP}, {0x97, PFRSQRT}, {0x9a, PFSUB}, {0x9e, PFADD},
		{0xa0, PFCMPGT}, {0xa4, PFMAX}, {0xa6, PFRCPIT1}, {0xa7, PFRSQIT1}, {0xaa, PFSUBR}, {0xae, PFACC},
		{0xb0, PFCMPEQ}, {0xb4, PFMUL}, {0xb6, PFRCPIT2}, {0xb7, PMULHRW}, {0xbb, PSWAPD}, {0xbf, PAVGUSB}
	};


	typedef uint8 RegDef;
	static const RegDef reg8List[8] = {REG_AL, REG_CL, REG_DL, REG_BL, REG_AH, REG_CH, REG_DH, REG_BH};
	static const RegDef reg8List64[16] = {REG_AL, REG_CL, REG_DL, REG_BL, REG_SPL, REG_BPL, REG_SIL, REG_DIL,
		REG_R8B, REG_R9B, REG_R10B, REG_R11B, REG_R12B, REG_R13B, REG_R14B, REG_R15B};
	static const RegDef reg16List[16] = {REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
		REG_R8W, REG_R9W, REG_R10W, REG_R11W, REG_R12W, REG_R13W, REG_R14W, REG_R15W};
	static const RegDef reg32List[16] = {REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI,
		REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D};
	static const RegDef reg64List[16] = {REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
		REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15};
	static const RegDef mmxRegList[16] = {REG_MM0, REG_MM1, REG_MM2, REG_MM3, REG_MM4, REG_MM5, REG_MM6, REG_MM7,
		REG_MM0, REG_MM1, REG_MM2, REG_MM3, REG_MM4, REG_MM5, REG_MM6, REG_MM7};
	static const RegDef xmmRegList[16] = {REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
		REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11, REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15};
	static const RegDef fpuRegList[16] = {REG_ST0, REG_ST1, REG_ST2, REG_ST3, REG_ST4, REG_ST5, REG_ST6, REG_ST7,
		REG_ST0, REG_ST1, REG_ST2, REG_ST3, REG_ST4, REG_ST5, REG_ST6, REG_ST7};


	struct RMDef
	{
		OperandType first : 8;
		OperandType second : 8;
		SegmentRegister segment : 8;
	};
#ifndef __cplusplus
	typedef struct RMDef RMDef;
#endif


	static const RegDef* __fastcall GetByteRegList(DecodeState* restrict state)
	{
		if (state->rex)
			return reg8List64;
		return reg8List;
	}


	static const RegDef* __fastcall GetRegListForOpSize(DecodeState* restrict state)
	{
		switch (state->opSize)
		{
		case 2:
			return reg16List;
		case 4:
			return reg32List;
		case 8:
			return reg64List;
		default:
			return NULL;
		}
	}


	static const RegDef* __fastcall GetRegListForFinalOpSize(DecodeState* restrict state)
	{
		switch (state->finalOpSize)
		{
		case 1:
			return GetByteRegList(state);
		case 2:
			return reg16List;
		case 4:
			return reg32List;
		case 8:
			return reg64List;
		default:
			return NULL;
		}
	}


	static const RegDef* __fastcall GetRegListForAddrSize(DecodeState* restrict state)
	{
		switch (state->addrSize)
		{
		case 2:
			return reg16List;
		case 4:
			return reg32List;
		case 8:
			return reg64List;
		default:
			return NULL;
		}
	}

	
	static uint8 __fastcall GetFinalOpSize(DecodeState* restrict state)
	{
		if (state->flags & DEC_FLAG_BYTE)
			return 1;
		return state->opSize;
	}


	static uint8 __fastcall Read8(DecodeState* restrict state)
	{
		uint8 val;

		if (state->len < 1)
		{
			// Read past end of buffer, returning 0xcc from now on will guarantee exit
			state->invalid = true;
			state->len = 0;
			return 0xcc;
		}

		val = *(state->opcode++);
		state->len--;
		return val;
	}


	static uint8 __fastcall Peek8(DecodeState* restrict state)
	{
		uint8 result = Read8(state);
		state->opcode--;
		return result;
	}


	static uint16 __fastcall Read16(DecodeState* restrict state)
	{
		uint16 val;

		if (state->len < 2)
		{
			// Read past end of buffer
			state->invalid = true;
			state->len = 0;
			return 0;
		}

		val = *((uint16*)state->opcode);
		state->opcode += 2;
		state->len -= 2;
		return val;
	}


	static uint32 __fastcall Read32(DecodeState* restrict state)
	{
		uint32 val;

		if (state->len < 4)
		{
			// Read past end of buffer
			state->invalid = true;
			state->len = 0;
			return 0;
		}

		val = *((uint32*)state->opcode);
		state->opcode += 4;
		state->len -= 4;
		return val;
	}


	static uint64 __fastcall Read64(DecodeState* restrict state)
	{
		uint64 val;

		if (state->len < 8)
		{
			// Read past end of buffer
			state->invalid = true;
			state->len = 0;
			return 0;
		}

		val = *((uint64*)state->opcode);
		state->opcode += 8;
		state->len -= 8;
		return val;
	}


	static int64 __fastcall ReadSigned8(DecodeState* restrict state)
	{
		return (int64)(int8)Read8(state);
	}


	static int64 __fastcall ReadSigned16(DecodeState* restrict state)
	{
		return (int64)(int16)Read16(state);
	}


	static int64 __fastcall ReadSigned32(DecodeState* restrict state)
	{
		return (int64)(int32)Read32(state);
	}


	static int64 __fastcall ReadFinalOpSize(DecodeState* restrict state)
	{
		if (state->flags & DEC_FLAG_IMM_SX)
			return ReadSigned8(state);
		switch (state->finalOpSize)
		{
		case 1:
			return Read8(state);
		case 2:
			return Read16(state);
		case 4:
			return Read32(state);
		case 8:
			return ReadSigned32(state);
		}
		return 0;
	}


	static int64 __fastcall ReadAddrSize(DecodeState* restrict state)
	{
		switch (state->addrSize)
		{
		case 2:
			return Read16(state);
		case 4:
		case 8:
			return Read32(state);
		}
		return 0;
	}


	static int64 __fastcall ReadSignedFinalOpSize(DecodeState* restrict state)
	{
		switch (state->finalOpSize)
		{
		case 1:
			return ReadSigned8(state);
		case 2:
			return ReadSigned16(state);
		case 4:
		case 8:
			return ReadSigned32(state);
		}
		return 0;
	}


	static void __fastcall UpdateOperationForAddrSize(DecodeState* restrict state)
	{
		if (state->addrSize == 4)
			state->result->operation = (InstructionOperation)(state->result->operation + 1);
		else if (state->addrSize == 8)
			state->result->operation = (InstructionOperation)(state->result->operation + 2);
	}


	static void __fastcall ProcessEncoding(DecodeState* restrict state, const InstructionEncoding* encoding)
	{
		state->result->operation = (InstructionOperation)encoding->operation;

		state->flags = decoders[encoding->encoding].flags;
		if (state->using64 && (state->flags & DEC_FLAG_INVALID_IN_64BIT))
		{
			state->invalid = true;
			return;
		}
		if (state->using64 && (state->flags & DEC_FLAG_DEFAULT_TO_64BIT))
			state->opSize = state->opPrefix ? 4 : 8;
		state->finalOpSize = GetFinalOpSize(state);

		if (state->flags & DEC_FLAG_FLIP_OPERANDS)
		{
			state->operand0 = &state->result->operands[1];
			state->operand1 = &state->result->operands[0];
		}
		else
		{
			state->operand0 = &state->result->operands[0];
			state->operand1 = &state->result->operands[1];
		}

		if (state->flags & DEC_FLAG_FORCE_16BIT)
			state->finalOpSize = 2;

		if (state->flags & DEC_FLAG_OPERATION_OP_SIZE)
		{
			if (state->finalOpSize == 4)
				state->result->operation = (InstructionOperation)(state->result->operation + 1);
			else if (state->finalOpSize == 8)
				state->result->operation = (InstructionOperation)(state->result->operation + 2);
		}

		if (state->flags & DEC_FLAG_REP)
		{
			if (state->rep != REP_PREFIX_NONE)
				state->result->flags |= X86_FLAG_REP;
		}
		else if (state->flags & DEC_FLAG_REP_COND)
		{
			if (state->rep == REP_PREFIX_REPNE)
				state->result->flags |= X86_FLAG_REPNE;
			else if (state->rep == REP_PREFIX_REPE)
				state->result->flags |= X86_FLAG_REPE;
		}

		decoders[encoding->encoding].func(state);

		if (state->result->operation == INVALID)
			state->invalid = true;

		if (state->result->flags & X86_FLAG_LOCK)
		{
			// Ensure instruction allows lock and it has proper semantics
			if (!(state->flags & DEC_FLAG_LOCK))
				state->invalid = true;
			else if (state->result->operation == CMP)
				state->invalid = true;
			else if ((state->result->operands[0].operand != MEM) && (state->result->operands[1].operand != MEM))
				state->invalid = true;
		}
	}


	static void __fastcall ProcessOpcode(DecodeState* restrict state, const InstructionEncoding* map, uint8 opcode)
	{
		ProcessEncoding(state, &map[opcode]);
	}


	static SegmentRegister __fastcall GetFinalSegment(DecodeState* restrict state, SegmentRegister seg)
	{
		return (state->result->segment == SEG_DEFAULT) ? seg : state->result->segment;
	}


	static void __fastcall SetMemOperand(DecodeState* restrict state, InstructionOperand* oper, const RMDef* def, int64 immed)
	{
		oper->operand = MEM;
		oper->components[0] = def->first;
		oper->components[1] = def->second;
		oper->immediate = immed;
		oper->segment = GetFinalSegment(state, def->segment);
	}


	static void __fastcall DecodeRM(DecodeState* restrict state, InstructionOperand* rmOper, const RegDef* regList, uint8 rmSize, uint8* regOper)
	{
		uint8 rmByte = Read8(state);
		uint8 mod = rmByte >> 6;
		uint8 rm = rmByte & 7;
		InstructionOperand temp;

		if (regOper)
			*regOper = (rmByte >> 3) & 7;

		if (!rmOper)
			rmOper = &temp;

		rmOper->size = rmSize;
		if (state->addrSize == 2)
		{
			static const RMDef rm16Components[9] = {{REG_BX, REG_SI, SEG_DS}, {REG_BX, REG_DI, SEG_DS},
				{REG_BP, REG_SI, SEG_SS}, {REG_BP, REG_DI, SEG_SS}, {REG_SI, NONE, SEG_DS},
				{REG_DI, NONE, SEG_DS}, {REG_BP, NONE, SEG_SS}, {REG_BX, NONE, SEG_DS},
				{NONE, NONE, SEG_DS}};
			switch (mod)
			{
			case 0:
				if (rm == 6)
				{
					rm = 8;
					SetMemOperand(state, rmOper, &rm16Components[rm], Read16(state));
				}
				else
					SetMemOperand(state, rmOper, &rm16Components[rm], 0);
				break;
			case 1:
				SetMemOperand(state, rmOper, &rm16Components[rm], ReadSigned8(state));
				break;
			case 2:
				SetMemOperand(state, rmOper, &rm16Components[rm], ReadSigned16(state));
				break;
			case 3:
				rmOper->operand = (OperandType)regList[rm];
				break;
			}
			if (rmOper->components[0] == NONE)
				rmOper->immediate &= 0xffff;
		}
		else
		{
			const RegDef* addrRegList = GetRegListForAddrSize(state);
			uint8 rmReg1Offset = state->rexRM1 ? 8 : 0;
			uint8 rmReg2Offset = state->rexRM2 ? 8 : 0;
			SegmentRegister seg = SEG_DEFAULT;
			rmOper->operand = MEM;
			if ((mod != 3) && (rm == 4))
			{
				// SIB byte present
				uint8 sibByte = Read8(state);
				uint8 base = sibByte & 7;
				uint8 index = (sibByte >> 3) & 7;
				rmOper->scale = 1 << (sibByte >> 6);
				if ((mod != 0) || (base != 5))
					rmOper->components[0] = (OperandType)addrRegList[base + rmReg1Offset];
				if ((index + rmReg2Offset) != 4)
					rmOper->components[1] = (OperandType)addrRegList[index + rmReg2Offset];
				switch (mod)
				{
				case 0:
					if (base == 5)
						rmOper->immediate = ReadSigned32(state);
					break;
				case 1:
					rmOper->immediate = ReadSigned8(state);
					break;
				case 2:
					rmOper->immediate = ReadSigned32(state);
					break;
				}
				if (((base + rmReg1Offset) == 4) || ((base + rmReg1Offset) == 5))
					seg = SEG_SS;
				else
					seg = SEG_DS;
			}
			else
			{
				switch (mod)
				{
				case 0:
					if (rm == 5)
					{
						rmOper->immediate = ReadSigned32(state);
						if (state->addrSize == 8)
							state->ripRelFixup = &rmOper->immediate;
					}
					else
						rmOper->components[0] = (OperandType)addrRegList[rm + rmReg1Offset];
					seg = SEG_DS;
					break;
				case 1:
					rmOper->components[0] = (OperandType)addrRegList[rm + rmReg1Offset];
					rmOper->immediate = ReadSigned8(state);
					seg = (rm == 5) ? SEG_SS : SEG_DS;
					break;
				case 2:
					rmOper->components[0] = (OperandType)addrRegList[rm + rmReg1Offset];
					rmOper->immediate = ReadSigned32(state);
					seg = (rm == 5) ? SEG_SS : SEG_DS;
					break;
				case 3:
					rmOper->operand = (OperandType)regList[rm + rmReg1Offset];
					break;
				}
			}
			if (seg != SEG_DEFAULT)
				rmOper->segment = GetFinalSegment(state, seg);
		}
	}


	static void __fastcall DecodeRMReg(DecodeState* restrict state, InstructionOperand* rmOper, const RegDef* rmRegList, uint8 rmSize,
		InstructionOperand* regOper, const RegDef* regList, uint8 regSize)
	{
		uint8 reg;
		DecodeRM(state, rmOper, rmRegList, rmSize, &reg);
		if (regOper)
		{
			uint8 regOffset = state->rexReg ? 8 : 0;
			regOper->size = regSize;
			regOper->operand = (OperandType)regList[reg + regOffset];
		}
	}


	static void __fastcall SetOperandToEsEdi(DecodeState* restrict state, InstructionOperand* oper, uint8 size)
	{
		const RegDef* addrRegList = GetRegListForAddrSize(state);
		oper->operand = MEM;
		oper->components[0] = (OperandType)addrRegList[7];
		oper->size = size;
		oper->segment = SEG_ES;
	}


	static void __fastcall SetOperandToDsEsi(DecodeState* restrict state, InstructionOperand* oper, uint8 size)
	{
		const RegDef* addrRegList = GetRegListForAddrSize(state);
		oper->operand = MEM;
		oper->components[0] = (OperandType)addrRegList[6];
		oper->size = size;
		oper->segment = GetFinalSegment(state, SEG_DS);
	}


	static void __fastcall SetOperandToImmAddr(DecodeState* restrict state, InstructionOperand* oper)
	{
		oper->operand = MEM;
		oper->immediate = ReadAddrSize(state);
		oper->segment = GetFinalSegment(state, SEG_DS);
		oper->size = state->finalOpSize;
	}


	static void __fastcall SetOperandToEaxFinalOpSize(DecodeState* restrict state, InstructionOperand* oper)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		oper->operand = (OperandType)regList[0];
		oper->size = state->finalOpSize;
	}


	static void __fastcall SetOperandToOpReg(DecodeState* restrict state, InstructionOperand* oper)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		uint8 regOffset = state->rexRM1 ? 8 : 0;
		oper->operand = (OperandType)regList[(state->opcode[-1] & 7) + regOffset];
		oper->size = state->finalOpSize;
	}


	static void __fastcall SetOperandToImm(DecodeState* restrict state, InstructionOperand* oper)
	{
		oper->operand = IMM;
		oper->size = state->finalOpSize;
		oper->immediate = ReadFinalOpSize(state);
	}


	static void __fastcall SetOperandToImm8(DecodeState* restrict state, InstructionOperand* oper)
	{
		oper->operand = IMM;
		oper->size = 1;
		oper->immediate = Read8(state);
	}


	static void __fastcall SetOperandToImm16(DecodeState* restrict state, InstructionOperand* oper)
	{
		oper->operand = IMM;
		oper->size = 2;
		oper->immediate = Read16(state);
	}


	static void __fastcall InvalidDecode(DecodeState* restrict state)
	{
		state->invalid = true;
	}


	static void __fastcall DecodeTwoByte(DecodeState* restrict state)
	{
		ProcessOpcode(state, twoByteOpcodeMap, Read8(state));
	}


	static void __fastcall DecodeFpu(DecodeState* restrict state)
	{
		uint8 modRM = Peek8(state);
		uint8 reg = (modRM >> 3) & 7;
		uint8 op = (uint8)state->result->operation;

		const InstructionEncoding* map;
		if ((modRM & 0xc0) == 0xc0)
			map = fpuRegOpcodeMap[op];
		else
			map = fpuMemOpcodeMap[op];
		ProcessEncoding(state, &map[reg]);
	}


	static void __fastcall DecodeNoOperands(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeRegRM(DecodeState* restrict state)
	{
		uint8 size = state->finalOpSize;
		const RegDef* regList = GetRegListForFinalOpSize(state);
		switch (state->flags & DEC_FLAG_REG_RM_SIZE_MASK)
		{
		case 0:
			break;
		case DEC_FLAG_REG_RM_2X_SIZE:
			size *= 2;
			break;
		case DEC_FLAG_REG_RM_FAR_SIZE:
			size += 2;
			break;
		case DEC_FLAG_REG_RM_NO_SIZE:
			size = 0;
			break;
		}

		DecodeRMReg(state, state->operand1, regList, size, state->operand0, regList, state->finalOpSize);

		if ((size != state->finalOpSize) && (state->operand1->operand != MEM))
			state->invalid = true;
	}


	static void __fastcall DecodeRegRMImm(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		DecodeRMReg(state, state->operand1, regList, state->finalOpSize, state->operand0, regList, state->finalOpSize);
		SetOperandToImm(state, &state->result->operands[2]);
	}


	static void __fastcall DecodeRMRegImm8(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		DecodeRMReg(state, state->operand0, regList, state->finalOpSize, state->operand1, regList, state->finalOpSize);
		SetOperandToImm8(state, &state->result->operands[2]);
	}


	static void __fastcall DecodeRMRegCL(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		DecodeRMReg(state, state->operand0, regList, state->finalOpSize, state->operand1, regList, state->finalOpSize);
		state->result->operands[2].operand = REG_CL;
		state->result->operands[2].size = 1;
	}


	static void __fastcall DecodeEaxImm(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		SetOperandToImm(state, state->operand1);
	}


	static void __fastcall DecodePushPopSeg(DecodeState* restrict state)
	{
		int8 offset = 0;
		if (state->opcode[-1] >= 0xa0) // FS/GS
			offset = -16;
		state->operand0->operand = (OperandType)(REG_ES + (state->opcode[-1] >> 3) + offset);
		state->operand0->size = state->opSize;
	}


	static void __fastcall DecodeOpReg(DecodeState* restrict state)
	{
		SetOperandToOpReg(state, state->operand0);
	}


	static void __fastcall DecodeEaxOpReg(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		SetOperandToOpReg(state, state->operand1);
	}


	static void __fastcall DecodeOpRegImm(DecodeState* restrict state)
	{
		SetOperandToOpReg(state, state->operand0);
		state->operand1->operand = IMM;
		state->operand1->size = state->finalOpSize;
		state->operand1->immediate = (state->opSize == 8) ? Read64(state) : ReadFinalOpSize(state);
	}


	static void __fastcall DecodeNop(DecodeState* restrict state)
	{
		if (state->rexRM1)
		{
			state->result->operation = XCHG;
			DecodeEaxOpReg(state);
		}
	}


	static void __fastcall DecodeImm(DecodeState* restrict state)
	{
		SetOperandToImm(state, state->operand0);
	}


	static void __fastcall DecodeImm16Imm8(DecodeState* restrict state)
	{
		SetOperandToImm16(state, state->operand0);
		SetOperandToImm8(state, state->operand1);
	}


	static void __fastcall DecodeEdiDx(DecodeState* restrict state)
	{
		SetOperandToEsEdi(state, state->operand0, state->finalOpSize);
		state->operand1->operand = REG_DX;
		state->operand1->size = 2;
	}


	static void __fastcall DecodeDxEsi(DecodeState* restrict state)
	{
		state->operand0->operand = REG_DX;
		state->operand0->size = 2;
		SetOperandToDsEsi(state, state->operand1, state->finalOpSize);
	}


	static void __fastcall DecodeRelImm(DecodeState* restrict state)
	{
		state->operand0->operand = IMM;
		state->operand0->size = state->opSize;
		state->operand0->immediate = ReadSignedFinalOpSize(state);
		state->operand0->immediate += state->addr + (state->opcode - state->opcodeStart);
	}


	static void __fastcall DecodeRelImmAddrSize(DecodeState* restrict state)
	{
		DecodeRelImm(state);
		UpdateOperationForAddrSize(state);
	}


	static void __fastcall DecodeGroupRM(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForFinalOpSize(state);
		uint8 regField;
		DecodeRM(state, state->operand0, regList, state->finalOpSize, &regField);
		state->result->operation = (InstructionOperation)groupOperations[(int)state->result->operation][regField];
	}


	static void __fastcall DecodeGroupRMImm(DecodeState* restrict state)
	{
		DecodeGroupRM(state);
		SetOperandToImm(state, state->operand1);
	}


	static void __fastcall DecodeGroupRMImm8V(DecodeState* restrict state)
	{
		DecodeGroupRM(state);
		SetOperandToImm8(state, state->operand1);
	}


	static void __fastcall DecodeGroupRMOne(DecodeState* restrict state)
	{
		DecodeGroupRM(state);
		state->operand1->operand = IMM;
		state->operand1->size = 1;
		state->operand1->immediate = 1;
	}


	static void __fastcall DecodeGroupRMCl(DecodeState* restrict state)
	{
		DecodeGroupRM(state);
		state->operand1->operand = REG_CL;
		state->operand1->size = 1;
	}


	static void __fastcall DecodeGroupF6F7(DecodeState* restrict state)
	{
		DecodeGroupRM(state);
		if (state->result->operation == TEST)
			SetOperandToImm(state, state->operand1);
		// Check for valid locking semantics
		if ((state->result->flags & X86_FLAG_LOCK) && (state->result->operation != NOT) && (state->result->operation != NEG))
			state->invalid = true;
	}


	static void __fastcall DecodeGroupFF(DecodeState* restrict state)
	{
		if (state->using64)
		{
			// Default to 64-bit for jumps and calls
			uint8 rm = Peek8(state);
			uint8 regField = (rm >> 3) & 7;
			if ((regField >= 2) && (regField <= 5))
				state->finalOpSize = state->opSize = state->opPrefix ? 4 : 8;
		}
		DecodeGroupRM(state);
		// Check for valid far jump/call semantics
		if ((state->result->operation == CALLF) || (state->result->operation == JMPF))
		{
			if (state->operand0->operand != MEM)
				state->invalid = true;
			state->operand0->size += 2;
		}
		// Check for valid locking semantics
		if ((state->result->flags & X86_FLAG_LOCK) && (state->result->operation != INC) && (state->result->operation != DEC))
			state->invalid = true;
	}


	static void __fastcall DecodeGroup0F00(DecodeState* restrict state)
	{
		uint8 rm = Peek8(state);
		uint8 regField = (rm >> 3) & 7;
		if (regField >= 2)
			state->opSize = 2;
		DecodeGroupRM(state);
	}


	static void __fastcall DecodeGroup0F01(DecodeState* restrict state)
	{
		uint8 rm = Peek8(state);
		uint8 regField = (rm >> 3) & 7;
		if (regField < 4)
			state->opSize = state->using64 ? 10 : 6;
		else if (regField != 7)
			state->opSize = 2;
		else
			state->opSize = 1;
		DecodeGroupRM(state);
	}


	static void __fastcall DecodeRMSRegV(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForOpSize(state);
		uint8 regField;
		DecodeRM(state, state->operand0, regList, state->opSize, &regField);
		if (regField >= 6)
			state->invalid = true;
		state->operand1->operand = (OperandType)(REG_ES + regField);
		state->operand1->size = 2;
		if (state->result->operands[0].operand == REG_CS)
			state->invalid = true;
	}


	static void __fastcall DecodeRM8(DecodeState* restrict state)
	{
		const RegDef* regList = GetByteRegList(state);
		DecodeRM(state, state->operand0, regList, 1, NULL);
	}


	static void __fastcall DecodeRMV(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForOpSize(state);
		DecodeRM(state, state->operand0, regList, state->opSize, NULL);
	}


	static void __fastcall DecodeFarImm(DecodeState* restrict state)
	{
		SetOperandToImm(state, state->operand1);
		SetOperandToImm16(state, state->operand0);
	}


	static void __fastcall DecodeEaxAddr(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		SetOperandToImmAddr(state, state->operand1);
	}


	static void __fastcall DecodeEdiEsi(DecodeState* restrict state)
	{
		SetOperandToEsEdi(state, state->operand0, state->finalOpSize);
		SetOperandToDsEsi(state, state->operand1, state->finalOpSize);
	}


	static void __fastcall DecodeEdiEax(DecodeState* restrict state)
	{
		SetOperandToEsEdi(state, state->operand0, state->finalOpSize);
		SetOperandToEaxFinalOpSize(state, state->operand1);
	}


	static void __fastcall DecodeEaxEsi(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		SetOperandToDsEsi(state, state->operand1, state->finalOpSize);
	}


	static void __fastcall DecodeAlEbxAl(DecodeState* restrict state)
	{
		const RegDef* regList = GetRegListForAddrSize(state);
		state->operand0->operand = REG_AL;
		state->operand0->size = 1;
		state->operand1->operand = MEM;
		state->operand1->components[0] = (OperandType)regList[3];
		state->operand1->components[1] = REG_AL;
		state->operand1->size = 1;
		state->operand1->segment = GetFinalSegment(state, SEG_DS);
	}


	static void __fastcall DecodeEaxImm8(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		SetOperandToImm8(state, state->operand1);
	}


	static void __fastcall DecodeEaxDx(DecodeState* restrict state)
	{
		SetOperandToEaxFinalOpSize(state, state->operand0);
		state->operand1->operand = REG_DX;
		state->operand1->size = 2;
	}


	static void __fastcall Decode3DNow(DecodeState* restrict state)
	{
		uint8 op;
		int i, min, max;
		DecodeRMReg(state, state->operand1, mmxRegList, 8, state->operand0, mmxRegList, 8);
		op = Read8(state);
		for (min = 0, max = (int)(sizeof(sparse3DNowOpcodes) / sizeof(SparseOpEntry)) - 1, i = (min + max) / 2;
			min <= max; i = (min + max) / 2)
		{
			if (op > sparse3DNowOpcodes[i].opcode)
				min = i + 1;
			else if (op < sparse3DNowOpcodes[i].opcode)
				max = i - 1;
			else
			{
				state->result->operation = (InstructionOperation)sparse3DNowOpcodes[i].operation;
				break;
			}
		}
	}


	static void __fastcall DecodeSSEHalf128(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeSSEHalf64And128(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeSSEHalfMem64(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeMovUps(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeMovLps(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeMovHps(DecodeState* restrict state)
	{
	}


	static void __fastcall DecodeRegCR(DecodeState* restrict state)
	{
		const RegDef* regList;
		uint8 reg;
		if (state->opSize == 2)
			state->opSize = 4;
		regList = GetRegListForOpSize(state);
		reg = Read8(state);
		if (state->flags & X86_FLAG_LOCK)
		{
			state->flags &= ~X86_FLAG_LOCK;
			state->rexReg = true;
		}
		state->operand0->operand = regList[(reg & 7) + (state->rexRM1 ? 8 : 0)];
		state->operand0->size = state->opSize;
		state->operand1->operand = (OperandType)((int)state->result->operation + ((reg >> 3) & 7) +
			(state->rexReg ? 8 : 0));
		state->operand1->size = state->opSize;
		state->result->operation = MOV;
	}


	static void __fastcall DecodeMovSXZX8(DecodeState* restrict state)
	{
		DecodeRMReg(state, state->operand1, GetByteRegList(state), 1, state->operand0, GetRegListForOpSize(state), state->opSize);
	}


	static void __fastcall DecodeMovSXZX16(DecodeState* restrict state)
	{
		DecodeRMReg(state, state->operand1, reg16List, 2, state->operand0, GetRegListForOpSize(state), state->opSize);
	}


	static void __fastcall DecodeMem16(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, 2, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeMem32(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, 4, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeMem64(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, 8, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeMem80(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, 10, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeMemFloatEnv(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, (state->opSize == 2) ? 14 : 28, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeMemFloatSave(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, reg32List, (state->opSize == 2) ? 94 : 108, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall DecodeFPUReg(DecodeState* restrict state)
	{
		DecodeRM(state, state->operand0, fpuRegList, 10, NULL);
	}


	static void __fastcall DecodeFPURegST0(DecodeState* restrict state)
	{
		DecodeFPUReg(state);
		state->operand1->operand = REG_ST0;
		state->operand1->size = 10;
	}


	static void __fastcall DecodeRegGroupNoOperands(DecodeState* restrict state)
	{
		uint8 rmByte = Read8(state);
		state->result->operation = (InstructionOperation)groupOperations[(int)state->result->operation][rmByte & 7];
	}


	static void __fastcall DecodeRegGroupAX(DecodeState* restrict state)
	{
		DecodeRegGroupNoOperands(state);
		state->operand0->operand = REG_AX;
		state->operand0->size = 2;
	}


	static void __fastcall DecodeCmpXch8B(DecodeState* restrict state)
	{
		if (state->opSize == 2)
			state->opSize = 4;
		else if (state->opSize == 8)
			state->result->operation = CMPXCH16B;
		DecodeRM(state, state->operand0, GetRegListForOpSize(state), state->opSize * 2, NULL);
		if (state->operand0->operand != MEM)
			state->invalid = true;
	}


	static void __fastcall ProcessPrefixes(DecodeState* restrict state)
	{
		uint8 rex = 0;
		bool addrPrefix = false;

		while (!state->invalid)
		{
			uint8 prefix = Read8(state);
			if ((prefix >= 0x26) && (prefix <= 0x3e) && ((prefix & 7) == 6))
			{
				// Segment prefix
				state->result->segment = (SegmentRegister)(SEG_ES + ((prefix >> 3) - 4));
			}
			else if ((prefix == 0x64) || (prefix == 0x65))
			{
				// FS/GS prefix
				state->result->segment = (SegmentRegister)(SEG_ES + (prefix - 0x60));
			}
			else if (prefix == 0x66)
			{
				state->opPrefix = true;
				state->result->flags |= X86_FLAG_OPSIZE;
			}
			else if (prefix == 0x67)
			{
				addrPrefix = true;
				state->result->flags |= X86_FLAG_ADDRSIZE;
			}
			else if (prefix == 0xf0)
				state->flags |= X86_FLAG_LOCK;
			else if (prefix == 0xf2)
				state->rep = REP_PREFIX_REPNE;
			else if (prefix == 0xf3)
				state->rep = REP_PREFIX_REPE;
			else if (state->using64 && (prefix >= 0x40) && (prefix <= 0x4f))
			{
				// REX prefix
				rex = prefix;
				continue;
			}
			else
			{
				// Not a prefix, continue instruction processing
				state->opcode--;
				break;
			}

			// Force ignore REX unless it is the last prefix
			rex = 0;
		}

		if (state->opPrefix)
			state->opSize = (state->opSize == 2) ? 4 : 2;
		if (addrPrefix)
			state->addrSize = (state->addrSize == 4) ? 2 : 4;

		if (rex)
		{
			// REX prefix found before opcode
			state->rex = true;
			state->rexRM1 = (rex & 1) != 0;
			state->rexRM2 = (rex & 2) != 0;
			state->rexReg = (rex & 4) != 0;
			if (rex & 8)
				state->opSize = 8;
		}
	}


	static void __fastcall ClearOperand(InstructionOperand* oper)
	{
		oper->operand = NONE;
		oper->components[0] = NONE;
		oper->components[1] = NONE;
		oper->scale = 1;
		oper->immediate = 0;
	}


	static void __fastcall InitDisassemble(DecodeState* restrict state)
	{
		ClearOperand(&state->result->operands[0]);
		ClearOperand(&state->result->operands[1]);
		ClearOperand(&state->result->operands[2]);
		state->result->operation = INVALID;
		state->result->flags = 0;
		state->result->segment = SEG_DEFAULT;
		state->invalid = false;
		state->opPrefix = false;
		state->rep = REP_PREFIX_NONE;
		state->ripRelFixup = NULL;
		state->rex = false;
		state->rexReg = false;
		state->rexRM1 = false;
		state->rexRM2 = false;
	}


	static void __fastcall FinishDisassemble(DecodeState* restrict state)
	{
		state->result->length = state->opcode - state->opcodeStart;
		if (state->ripRelFixup)
			*state->ripRelFixup += state->addr + state->result->length;
	}


	bool Disassemble16(const uint8* opcode, uint64 addr, size_t maxLen, Instruction* result)
	{
		DecodeState state;
		state.result = result;
		state.opcodeStart = opcode;
		state.opcode = opcode;
		state.addr = addr;
		state.len = (maxLen > 15) ? 15 : maxLen;
		state.addrSize = 2;
		state.opSize = 2;
		state.using64 = false;
		InitDisassemble(&state);

		ProcessPrefixes(&state);
		ProcessOpcode(&state, mainOpcodeMap, Read8(&state));
		FinishDisassemble(&state);
		return !state.invalid;
	}


	bool Disassemble32(const uint8* opcode, uint64 addr, size_t maxLen, Instruction* result)
	{
		DecodeState state;
		state.result = result;
		state.opcodeStart = opcode;
		state.opcode = opcode;
		state.addr = addr;
		state.len = (maxLen > 15) ? 15 : maxLen;
		state.addrSize = 4;
		state.opSize = 4;
		state.using64 = false;
		InitDisassemble(&state);

		ProcessPrefixes(&state);
		ProcessOpcode(&state, mainOpcodeMap, Read8(&state));
		FinishDisassemble(&state);
		return !state.invalid;
	}


	bool Disassemble64(const uint8* opcode, uint64 addr, size_t maxLen, Instruction* result)
	{
		DecodeState state;
		state.result = result;
		state.opcodeStart = opcode;
		state.opcode = opcode;
		state.addr = addr;
		state.len = (maxLen > 15) ? 15 : maxLen;
		state.addrSize = 8;
		state.opSize = 4;
		state.using64 = true;
		InitDisassemble(&state);

		ProcessPrefixes(&state);
		ProcessOpcode(&state, mainOpcodeMap, Read8(&state));
		FinishDisassemble(&state);
		return !state.invalid;
	}


	static void __fastcall WriteChar(char** out, size_t* outMaxLen, char ch)
	{
		if (*outMaxLen > 1)
		{
			*((*out)++) = ch;
			(*outMaxLen)--;
		}
	}


	static void __fastcall WriteString(char** out, size_t* outMaxLen, const char* str)
	{
		for (; *str; str++)
			WriteChar(out, outMaxLen, *str);
	}


	static void __fastcall WriteHex(char** out, size_t* outMaxLen, uint64 val, uint32 width, bool prefix)
	{
		char temp[17];
		int32 i;
		if (prefix)
			WriteString(out, outMaxLen, "0x");
		if (width > 16)
			width = 16;
		for (i = (width - 1); i >= 0; i--, val >>= 4)
		{
			char digit = (char)(val & 0xf);
			if (digit < 10)
				temp[i] = digit + '0';
			else
				temp[i] = digit + 'a' - 10;
		}
		temp[width] = 0;
		WriteString(out, outMaxLen, temp);
	}


	static const char* __fastcall GetSizeString(uint8 size)
	{
		switch (size)
		{
		case 1:
			return "byte ";
		case 2:
			return "word ";
		case 4:
			return "dword ";
		case 6:
			return "fword ";
		case 8:
			return "qword ";
		case 10:
			return "tword ";
		case 16:
			return "oword ";
		default:
			return "";
		}
	}


	static void __fastcall WriteOperand(char** out, size_t* outMaxLen, OperandType type, uint8 scale, bool plus)
	{
		if (plus)
			WriteString(out, outMaxLen, "+");
		WriteString(out, outMaxLen, &operandString[operandOffsets[type]]);
		if (scale != 1)
		{
			WriteChar(out, outMaxLen, '*');
			WriteChar(out, outMaxLen, scale + '0');
		}
	}


	size_t FormatInstructionString(char* out, size_t outMaxLen, const char* fmt, const uint8* opcode,
		uint64 addr, const Instruction* instr)
	{
		char* start = out;
		size_t len;
		for (; *fmt; fmt++)
		{
			if (*fmt == '%')
			{
				uint32 width = 0;
				for (fmt++; *fmt; fmt++)
				{
					if (*fmt == 'a')
					{
						if (width == 0)
							width = sizeof(void*) * 2;
						WriteHex(&out, &outMaxLen, addr, width, false);
						break;
					}
					else if (*fmt == 'b')
					{
						size_t i;
						for (i = 0; i < instr->length; i++)
							WriteHex(&out, &outMaxLen, opcode[i], 2, false);
						for (; i < width; i++)
							WriteString(&out, &outMaxLen, "  ");
						break;
					}
					else if (*fmt == 'i')
					{
						char* operationStart = out;
						if (instr->flags & X86_FLAG_ANY_REP)
						{
							WriteString(&out, &outMaxLen, "rep");
							if (instr->flags & X86_FLAG_REPNE)
								WriteChar(&out, &outMaxLen, 'n');
							if (instr->flags & (X86_FLAG_REPNE | X86_FLAG_REPE))
								WriteChar(&out, &outMaxLen, 'e');
							WriteChar(&out, &outMaxLen, ' ');
						}
						WriteString(&out, &outMaxLen, &operationString[operationOffsets[instr->operation]]);
						for (; ((size_t)(out - operationStart) < (size_t)width) && (outMaxLen > 1); )
							WriteChar(&out, &outMaxLen, ' ');
						break;
					}
					else if (*fmt == 'o')
					{
						uint32 i;
						for (i = 0; i < 3; i++)
						{
							if (instr->operands[i].operand == NONE)
								break;
							if (i != 0)
								WriteString(&out, &outMaxLen, ", ");
							if (instr->operands[i].operand == IMM)
								WriteHex(&out, &outMaxLen, instr->operands[i].immediate, instr->operands[i].size * 2, true);
							else if (instr->operands[i].operand == MEM)
							{
								bool plus = false;
								WriteString(&out, &outMaxLen, GetSizeString(instr->operands[i].size));
								if ((instr->segment != SEG_DEFAULT) || (instr->operands[i].segment == SEG_ES))
								{
									WriteOperand(&out, &outMaxLen, (OperandType)(instr->operands[i].segment + REG_ES), 1, false);
									WriteChar(&out, &outMaxLen, ':');
								}
								WriteChar(&out, &outMaxLen, '[');
								if (instr->operands[i].components[0] != NONE)
								{
									WriteOperand(&out, &outMaxLen, instr->operands[i].components[0], 1, false);
									plus = true;
								}
								if (instr->operands[i].components[1] != NONE)
								{
									WriteOperand(&out, &outMaxLen, instr->operands[i].components[1], instr->operands[i].scale, plus);
									plus = true;
								}
								if (instr->operands[i].immediate != 0)
								{
									if (plus && ((int64)instr->operands[i].immediate >= -0x80) &&
										((int64)instr->operands[i].immediate < 0))
									{
										WriteChar(&out, &outMaxLen, '-');
										WriteHex(&out, &outMaxLen, -(int64)instr->operands[i].immediate, 2, true);
									}
									else if (plus && ((int64)instr->operands[i].immediate > 0) &&
										((int64)instr->operands[i].immediate <= 0x7f))
									{
										WriteChar(&out, &outMaxLen, '+');
										WriteHex(&out, &outMaxLen, instr->operands[i].immediate, 2, true);
									}
									else
									{
										if (plus)
											WriteChar(&out, &outMaxLen, '+');
										WriteHex(&out, &outMaxLen, instr->operands[i].immediate, 8, true);
									}
								}
								WriteChar(&out, &outMaxLen, ']');
							}
							else
								WriteOperand(&out, &outMaxLen, instr->operands[i].operand, 1, false);
						}
						break;
					}
					else if ((*fmt >= '0') && (*fmt <= '9'))
						width = (width * 10) + (*fmt - '0');
					else
					{
						WriteChar(&out, &outMaxLen, *fmt);
						break;
					}
				}
			}
			else
				WriteChar(&out, &outMaxLen, *fmt);
		}
		len = out - start;
		if (outMaxLen > 0)
			*(out++) = 0;
		return len;
	}


	size_t DisassembleToString16(char* out, size_t outMaxLen, const char* fmt, const uint8* opcode,
		uint64 addr, size_t maxLen, Instruction* instr)
	{
		if (!Disassemble16(opcode, addr, maxLen, instr))
			return 0;
		return FormatInstructionString(out, outMaxLen, fmt, opcode, addr, instr);
	}


	size_t DisassembleToString32(char* out, size_t outMaxLen, const char* fmt, const uint8* opcode,
		uint64 addr, size_t maxLen, Instruction* instr)
	{
		if (!Disassemble32(opcode, addr, maxLen, instr))
			return 0;
		return FormatInstructionString(out, outMaxLen, fmt, opcode, addr, instr);
	}


	size_t DisassembleToString64(char* out, size_t outMaxLen, const char* fmt, const uint8* opcode,
		uint64 addr, size_t maxLen, Instruction* instr)
	{
		if (!Disassemble64(opcode, addr, maxLen, instr))
			return 0;
		return FormatInstructionString(out, outMaxLen, fmt, opcode, addr, instr);
	}
#ifdef __cplusplus
}
#endif

