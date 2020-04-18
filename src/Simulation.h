#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdio.h>
#include <math.h>
#include <sys/io.h>
//#include <process.h>
#include <time.h>
#include <stdlib.h>
#include "Reg_def.h"
#include "Debug.h"

#include "elfio/elfio.hpp"
// opcode type
#define OP_R 		51
#define OP_M		59
#define OP_LW 		3
#define OP_I 		19
#define OP_IW 		27
#define OP_JALR		103
#define OP_SCALL 	115
#define OP_S		35
#define OP_SB		99
#define OP_AUIPC	23
#define OP_LUI		55
#define OP_JAL 		111


// funct3 type
#define F3_ADD 		0
#define F3_MUL 		0
#define F3_SUB 		0
#define F3_SLL 		1
#define F3_MULH 	1
#define F3_SLT		2
#define F3_XOR		4
#define F3_DIV		4
#define F3_SRL		5
#define F3_SRA		5
#define F3_OR 		6
#define F3_REM		6
#define F3_AND		7

#define F3_MULW		0
#define F3_ADDW 	0

#define F3_LB		0
#define F3_LH 		1
#define F3_LW 		2
#define F3_LD 		3

#define F3_ADDI 	0
#define F3_SLLI		1
#define F3_SLTI		2
#define F3_XORI		4
#define F3_SRLI		5
#define F3_SRAI		5
#define F3_ORI 		6
#define F3_ANDI		7
#define F3_ADDIW 	0
#define F3_SLLIW	1
#define F3_SRLIW	5
#define F3_JALR		0
#define F3_SCALL 	0

#define F3_SB 		0
#define F3_SH		1
#define F3_SW		2
#define F3_SD		3

#define F3_BEQ 		0
#define F3_BNE		1
#define F3_BLT		4
#define F3_BGE		5


// funct7 type
#define F7_ADD 		0
#define F7_MUL 		1
#define F7_SUB 		32
#define F7_SLL 		0
#define F7_MULH 	1
#define F7_SLT		2
#define F7_XOR		4
#define F7_DIV		1
#define F7_SRL		0
#define F7_SRA		32
#define F7_OR 		0
#define F7_REM		1
#define F7_AND		0

#define F7_SLLI		0
#define F7_SRLI		0
#define F7_SRAI		16

#define F7_ADDW		0
#define F7_MULW		1

#define F7_SCALL 0

// index of instruction, for debug
#define ADD 		0
#define MUL 		1
#define SUB 		2
#define SLL 		3
#define MULH 		4
#define SLT 		5
#define XOR 		6
#define DIV 		7
#define SRL 		8
#define SRA 		9
#define OR 			10
#define REM 		11
#define AND 		12
#define LB 			13
#define LH 			14
#define LW 			15
#define LD 			16
#define ADDI 		17
#define SLLI 		18
#define SLTI 		19
#define XORI 		20
#define SRLI 		21
#define SRAI 		22
#define ORI 		23
#define ANDI 		24
#define ADDIW 		25
#define JALR 		26
#define ECALL 		27
#define SB 			28
#define SH 			29
#define SW 			30
#define SD 			31
#define BEQ 		32
#define BNE 		33
#define BLT 		34
#define BGE 		35
#define AUIPC 		36
#define LUI 		37
#define JAL 		38

#define MULW 		39
#define SLLIW 		40
#define SRLIW 		41
#define ADDW 		42

// names of instruction, for debug
const char *INSTNAME[]{
    "add", "mul", "sub", "sll", "mulh", "slt", "xor", "div", "srl", "sra",
    "or", "rem", "and", "lb", "lh", "lw", "ld", "addi", "slli", "slti",
    "xori", "srli", "srai", "ori", "andi", "addiw", "jalr", "ecall", "sb", "sh",
    "sw", "sd", "beq", "bne", "blt", "bge", "auipc", "lui", "jal", "mulw", "slliw",
    "srliw", "addw"
};

// operation type
#define OPNONE		0
#define OPADD 		1
#define OPSUB 		2
#define OPMUL 		3
#define OPMULH		4
#define OPDIV		5
#define OPREM		6
#define OPSL 		7	// shift left
#define OPSR 		8	// shift right
#define OPSLT		9
#define OPXOR 		10
#define OPOR 		11
#define OPAND		12
#define OPJMP		13
#define OPMOV		14

// branch type
#define B_NO		0
#define B_BEQ		1
#define B_BNE		2
#define B_BLT		3
#define B_BGE		4
#define B_ALL		5


#define MAXADDR 100000000

//主存
char memory[MAXADDR]={0};
//寄存器堆
REG reg[32]={0};

char elfFile[32];
int singleStep = 0;

//加载内存
void loadMemory(ELFIO::elfio *reader);

void parse(int argc, char **argv);

void printUsage();

void simulate();

void IF();

void ID();

void EX();

void MEM();

void WB();


//符号扩展
REG signExt(unsigned int src,int bit);

void printREG();

//-----------------------------------------------------------------------------------------------------------
// memory part
//-----------------------------------------------------------------------------------------------------------

int readMem(int addr, void *val, int size);
int writeMem(int addr, void *val, int size);
int translate(int addr, int *phyAddr, int size, int writing);
bool memConflict(int readAddr, int readSize, int writeAddr, int writeSize);


//-----------------------------------------------------------------------------------------------------------
// branch part
//-----------------------------------------------------------------------------------------------------------

// for branch prediction
#define POL_ANT		0			// always not taken
#define POL_AT 		1			// always taken
#define POL_BTFN	2			// back taken forward not taken

int branchPolicy = POL_ANT;
int branchBPC;
int branchNPC;

int branchPC(int npc, int bpc);

bool branchCorrect(bool condition);

int branchCorrectPC();

struct exception {
	int status;
	int valid;
	int pc;
};


//-----------------------------------------------------------------------------------------------------------
// exception part
//-----------------------------------------------------------------------------------------------------------
// excetptions
#define NO_EXCEPTION 	0
#define UNDEF_INST		1
#define DIVIDE_ZERO		2
#define PAGE_FAULT		3
#define SYSCALL			4


void exceptionHandler();

void raiseException(int type, int status, int pc);

bool checkException();

struct exception excepTable[5];

void syscallHandler();

//-----------------------------------------------------------------------------------------------------------
// static part
//-----------------------------------------------------------------------------------------------------------
#define CYCLE_NORMAL 	1
#define CYCLE_MUL	 	10
#define CYCLE_MULH		20
#define CYCLE_DIV		40

int staCycle = 0;
int staInstr = 0;
int staMemHazard = 0;
int staRegHazard = 0;
int staCtrlHazard = 0;

void PrintStatic();


#endif