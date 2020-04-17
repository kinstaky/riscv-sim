#ifndef  REG_DEF_H
#define  REG_DEF_H

typedef unsigned long long REG;

struct IFID{
	int instType, opType;
	int npc, pc;
	int rd, rs1, rs2;
	int readREG1, readREG2, writeREG;
	REG val1, val2, writeVal;
	int branch;
	int writeSize, readSize, extTop;
	int bubble;
	int excep, excepStatus;

}IF_ID,IF_ID_old;


struct IDEX{
	int pc, npc;
	int opType;
	int rd;
	REG val1, val2, writeVal;
	int readSize, writeSize, extTop;
	int branch, writeREG;
	int bubble;
	int excep, excepStatus;

}ID_EX,ID_EX_old;

struct EXMEM{
	int pc;
	REG res, writeVal;
	int writeREG;
	int rd;
	int extTop, readSize, writeSize;
	int bubble;
	int excep, excepStatus;

}EX_MEM,EX_MEM_old;

struct MEMWB{
	int pc;
	REG res;
	int rd;
	int writeREG;
	int bubble;
	int excep, excepStatus;

}MEM_WB,MEM_WB_old;

struct WBIF {
	int pc;
	int bubble;
}	WB_IF, WB_IF_old;

#endif