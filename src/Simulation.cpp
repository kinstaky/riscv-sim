#include "Simulation.h"


REG mulh(REG a, REG b);

//指令运行数
long long inst_num=0;

//系统调用退出指示
int endPC;

//加载代码段
//初始化PC
int main(int argc, char **argv) {
	parse(argc, argv);

	//解析elf文件
	ELFIO::elfio reader;
	if (!reader.load(elfFile)) {
		fprintf(stderr, "Error loading ELF file %s\n", elfFile);
		return -1;
	}
	//加载内存
	loadMemory(&reader);

	//设置入口地址
	int pc = 0;
	ELFIO::Elf64_Addr mainEnd = 0;
	{
		ELFIO::Elf_Half sec_num = reader.sections.size();
		std::string name;
		ELFIO::Elf64_Addr value;
		ELFIO::Elf_Xword size;
		unsigned char bind;
		unsigned char type;
		ELFIO::Elf_Half section_index;
		unsigned char other;
		for (int i = 0; i != sec_num; ++i) {
			ELFIO::section* psec = reader.sections[i];
			if (psec->get_type() == SHT_SYMTAB) {
				const ELFIO::symbol_section_accessor symbols(reader, psec);
				for (unsigned int j = 0; j != symbols.get_symbols_num(); ++j) {
					symbols.get_symbol(j, name, value, size, bind, type, section_index, other);
					if (name == "main") {
						pc = value;
					}
					if (value > mainEnd) mainEnd = value;
				}
			}
		}

		for (int i = 0; i != sec_num; ++i) {
			ELFIO::section* psec = reader.sections[i];
			if (psec->get_type() == SHT_SYMTAB) {
				const ELFIO::symbol_section_accessor symbols(reader, psec);
				for (unsigned int j = 0; j != symbols.get_symbols_num(); ++j) {
					symbols.get_symbol(j, name, value, size, bind, type, section_index, other);
					if (value > pc && value < mainEnd) mainEnd = value;
				}
			}
		}
	}
	WB_IF.pc = pc;
	endPC = mainEnd-4;


	//设置全局数据段地址寄存器
	//reg[3] = gp;

	reg[2] = MAXADDR/2;//栈基址 （sp寄存器）

	for (int i = 0; i != 5; ++i) excepTable[i].valid = 0;

	simulate();

	return 0;
}

void parse(int argc, char **argv) {
	if (argc > 3 || argc < 2) {
		printUsage();
	}
	if (argc == 2) {
		if (!strcmp(argv[1], "-h")) printUsage();
		else strcpy(elfFile, argv[1]);
	}
	if (argc == 3) {
		if (strcmp(argv[1], "-s")) printUsage();
		else {
			singleStep = 1;
			strcpy(elfFile, argv[2]);
		}
	}
}

void printUsage() {
	printf("usage: ./sim [options] <filename>\n");
	printf("       -s 		run in single step\n");
	printf("       -h       print this information\n");
	exit(-1);
}

void simulate()
{
	//结束PC的设置
	//int end = (int)endPC-1;
	IF_ID.bubble = 1;
	ID_EX.bubble = 1;
	EX_MEM.bubble = 1;
	MEM_WB.bubble = 1;

	if (singleStep) {
		fprintf(stderr, ">>Simulation start. From 0x%x to 0x%x\n", WB_IF.pc, endPC);
	}

	while(true) {
		reg[0] = 0;
		//运行
		IF();
		ID();
		EX();
		MEM();
		WB();


		exceptionHandler();


		//更新中间寄存器
		if (checkException()) {
			IF_ID = IF_ID_old;
			ID_EX = ID_EX_old;
			EX_MEM = EX_MEM_old;
			MEM_WB = MEM_WB_old;
			WB_IF = WB_IF_old;
		}

        if (WB_IF.pc == endPC) WB_IF.bubble = 1;
        if (WB_IF.bubble && IF_ID.bubble && ID_EX.bubble && EX_MEM.bubble && MEM_WB.bubble && WB_IF.pc == endPC) {
        	break;
        }

        if (singleStep) {
        	fprintf(stderr, ">");
        	char ch;
        	while ((ch = getchar()) != '\n') {
        		if (ch == 'r') printREG();
       			else if (ch == 'x') {
					int num, address;
					char format;
					scanf(" %d %c %x", &num, &format, &address);
					switch (format) {
						case 'b':
							for (int i = 0; i != num; ++i) {
								char val;
								readMem(address+i, (void*)&val, 1);
								fprintf(stderr, "%02hhx\t", val);
								if (i % 8 == 7) fprintf(stderr, "\n");
							}
							break;
						case 'h':
							for (int i = 0; i != num; ++i) {
								short val;
								readMem(address+i*2, (void*)&val, 2);
								fprintf(stderr, "%04hx\t", val);
								if (i % 4 == 3) fprintf(stderr, "\n");
							}
							break;
						case 'w':
							for (int i = 0; i != num; ++i) {
								int val;
								readMem(address+i*4, (void*)&val, 4);
								fprintf(stderr, "%08x\t", val);
								if (i % 2 == 1) fprintf(stderr, "\n");
							}
							break;
						case 'd':
							for (int i = 0; i != num; ++i) {
								long long val;
								readMem(address+i*8, (void*)&val, 8);
								fprintf(stderr, "%016lx\n", val);
							}
							break;
						default:
							fprintf(stderr, "input \'r\' or \'x\' <number><format(b, h, w, d)> <address> or Enter\n");
					}
					fprintf(stderr, "\n>");
				}
				else fprintf(stderr, "input \'r\' or \'x\' <number><format(b, h, w, d)> <address> or Enter\n");
       			while ((ch = getchar()) != '\n');
        	}
        }

	}
	printf("\n");
	if (singleStep) {
		fprintf(stderr, ">>Simulation over.\n");
	}

	PrintStatic();
}


//取指令
void IF() {
	int bubble = WB_IF.bubble;
	if (bubble) {
		if (singleStep) {
			fprintf(stderr, "bubble %d\n", bubble);
		}
		IF_ID_old.bubble = bubble;
		WB_IF_old.bubble = bubble - 1;
		return;
	}

	// calculate the new pc
	int pc = WB_IF.pc;
	int npc = pc + 4;		// next pc
	int instr;
	int excep = readMem(pc, (void*)&instr, 4);
	if (excep) {
		raiseException(PAGE_FAULT, pc, pc);
		return;
	}


	// debug
	int instType;

	// temp var, use in this function
	int OP = instr & 0x7f;
	int opType = -1;
	int fuc3 = (instr >> 12) & 0x7;
	int fuc7 = (instr >> 25) & 0x7f;
	int rd = (instr >> 7) & 0x1f;
	int rs1 = (instr >> 15) & 0x1f;
	int rs2 = 0;
	REG val2;
	REG val1;
	int readREG1 = 1;
	int readREG2 = 0;
	// REGs
	int extTop = 0;			// top of sign extension, don't extend if 0
	int readSize = 0;		// size of memory read top, don't read if 0
	int writeSize = 0;		// size of memory write top, don't write if 0
	int branch = 0;			// branch type
	int writeREG = 1;		// write to rd
	REG writeVal;
	excep = NO_EXCEPTION;
	int excepStatus;


	if (OP == OP_R) {
		rs2 = (instr >> 20) & 0x1f;
		readREG2 = 1;
		switch (fuc3) {
			case F3_ADD:	// add, mul, sub
				if (fuc7 == F7_ADD) {
					opType = OPADD;
					instType = ADD;
				}
				else if (fuc7 == F7_MUL) {
					opType = OPMUL;
					instType = MUL;
				}
				else if (fuc7 == F7_SUB) {
					opType = OPSUB;
					instType = SUB;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_SLL:		// sll, mulh
				if (fuc7 == F7_SLL) {
					opType = OPSL;
					instType = SLL;
				}
				else if (fuc7 == F7_MULH) {
					opType = OPMULH;
					instType = MULH;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_SLT:
				if (fuc7 == F7_SLT) {
					opType = OPSLT;
					instType = SLT;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_XOR:		// xor, div
				if (fuc7 == F7_XOR) {
					opType = OPXOR;
					instType = XOR;
				}
				else if (fuc7 == F7_DIV) {
					opType = OPDIV;
					instType = DIV;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_SRL:		// srl, sra
				if (fuc7 == F7_SRL) {
					opType = OPSR;
					instType = SRL;
				}
				else if (fuc7 == F7_SRA) {
					opType = OPSR;
					instType = SRA;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_OR:			// or, rem
				if (fuc7 == F7_OR) {
					opType = OPOR;
					instType = OR;
				}
				else if (fuc7 == F7_REM) {
					opType = OPREM;
					instType = REM;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			case F3_AND:		// and
				if (fuc7 == F7_AND) {
					opType = OPAND;
					instType = AND;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			default:
				excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
		}
		if (excep == NO_EXCEPTION && singleStep) {
			fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, x%d\n", pc, instr, INSTNAME[instType], rd, rs1, rs2);
		}
	}
	else if (OP == OP_M) {
		rs2 = (instr >> 20) & 0x1f;
		readREG2 = 1;
		switch (fuc3) {
			case F3_MULW:
				if (fuc7 == F7_MULW) {
					opType = OPMUL;
					instType = MULW;
				}
				else if (fuc7 == F7_ADDW) {
					opType = OPADD;
					instType = ADDW;
				}
				else {
					excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
				}
				break;
			default:
				excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
		}
	}
	else if(OP == OP_LW) {
		val2 = (instr >> 20) & 0xfff;			// immediate
		val2 = signExt(val2, 12);
		opType = OPADD;
		switch (fuc3) {
			case F3_LB:
				instType = LB;
				extTop = 8;
				readSize = 1;
				break;
			case F3_LH:
				instType = LH;
				extTop = 16;
				readSize = 2;
				break;
			case F3_LW:
				instType = LW;
				extTop = 32;
				readSize = 4;
				break;
			case F3_LD:
				instType = LD;
				readSize = 8;
				break;
			default:
				excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
		}
		if (excep == NO_EXCEPTION && singleStep) {
			fprintf(stderr, "pc 0x%x    %08x    %s x%d 0x%lx(x%d)\n", pc, instr, INSTNAME[instType], rd, val2, rs1);
		}
    }
    else if (OP == OP_I) {
    	val2 = (instr >> 20) & 0xfff;
    	val2 = signExt(val2, 12);
    	switch (fuc3) {
    		case F3_ADDI:
    			opType = OPADD;
    			instType = ADDI;
    			break;
    		case F3_SLLI:
    			val2 &= 0x3f;
    			val2 = signExt(val2, 6);
    			opType = OPSL;
    			instType = SLLI;
    			break;
    		case F3_SLTI:
    			opType = OPSLT;
    			instType = SLTI;
    			break;
    		case F3_XORI:
    			opType = OPXOR;
    			instType = XORI;
    			break;
    		case F3_SRLI:		// srli, srai
    			val2 &= 0x3f;
    			val2 = signExt(val2, 6);
    			opType = OPSR;
    			fuc7 = (instr >> 26) & 0x3f;
    			if (fuc7== F7_SRLI) {
	    			instType = SRLI;
	    		} else if (fuc7 == F7_SRAI) {
	    			instType = SRAI;
	    		}
	    		else {
	    			excep = UNDEF_INST;
					excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
	    		}
    			break;
    		case F3_ORI:
    			opType = OPOR;
    			instType = ORI;
    			break;
    		case F3_ANDI:
    			opType = OPAND;
    			instType = ANDI;
    			break;
    		default:
    			excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    	}
    	if (excep == NO_EXCEPTION && singleStep) {
    		fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, rs1, val2);
    	}
    }
    else if (OP == OP_IW) {
    	if (fuc3 == F3_ADDIW) {
    		val2 = signExt((instr>>20)&0xfff, 12);
    		extTop = 32;
    		opType = OPADD;
    		instType = ADDIW;
    		if (singleStep) {
    			fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, rs1, val2);
    		}
    	}
    	else if (fuc3 == F3_SLLIW) {
    		val2 = signExt((instr>>20)&0xfff, 12);
    		extTop = 32;
    		opType = OPSL;
    		instType = SLLIW;
    		if (singleStep) {
    			fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, rs1, val2);
    		}
    	}
    	else if (fuc3 == F3_SRLIW) {
    		val2 = signExt((instr>>20)&0xfff, 12);
    		extTop = 32;
    		opType = OPSR;
    		instType = SRLIW;
    		if (singleStep) {
    			fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, rs1, val2);
    		}
    	}
    	else {
    		excep = UNDEF_INST;
			excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    	}

    }
    else if (OP == OP_JALR) {
    	if (fuc3 == 0) {
    		opType = OPJMP;
    		instType = JALR;
    		val2 = (instr >> 20) & 0xfff;
    		val2 = signExt(val2, 12);
    		branch = B_NO;
    		WB_IF_old.bubble = 2;
    		if (singleStep) {
    			fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, rs1, val2);
    		}
    	}
    	else {
    		excep = UNDEF_INST;
			excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    	}
    }
    else if (OP == OP_SCALL) {
    	opType = OPNONE;
    	instType = ECALL;
    	writeREG = 0;
   		readREG1 = 0;

   		excep = SYSCALL;
		excepStatus = 0;
		if (singleStep) {
			fprintf(stderr, "pc 0x%x    %08x    %s\n", pc, instr, INSTNAME[instType]);
		}
    }
    else if (OP == OP_S) {
    	readREG2 = 1;
    	rs2 = (instr >> 20) & 0x1f;
    	val2 = (((instr>>25)&0x7f) << 5) | ((instr>>7)&0x1f);
    	val2 = signExt(val2, 12);
    	opType = OPADD;
    	writeREG = 0;
    	switch (fuc3) {
    		case F3_SB:
    			instType = SB;
    			writeSize = 1;
    			break;
    		case F3_SH:
    			instType = SH;
    			writeSize = 2;
    			break;
    		case F3_SW:
    			instType = SW;
    			writeSize = 4;
    			break;
    		case F3_SD:
    			instType = SD;
    			writeSize = 8;
    			break;
    		default:
    			excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    	}
    	if (excep == NO_EXCEPTION && singleStep) {
    		fprintf(stderr, "pc 0x%x    %08x    %s x%d, 0x%lx(x%d)\n", pc, instr, INSTNAME[instType], rs2, val2, rs1);
    	}
    }
    else if (OP == OP_SB) {
    	readREG2 = 1;
    	rs2 = (instr >> 20) & 0x1f;
    	opType = OPNONE;
    	writeREG = 0;
    	int offset = (((instr>>31)&0x1)<<12) | (((instr>>7)&0x1)<<11);
    	offset |= (((instr>>25)&0x3f)<<5) | (((instr>>8)&0xf)<<1);
    	offset = signExt(offset, 12);
    	npc = branchPC(npc, pc+offset);			// get next PC with branch prediction
    	switch (fuc3) {
    		case F3_BEQ:
    			instType = BEQ;
    			branch = B_BEQ;
    			break;
    		case F3_BNE:
    			instType = BNE;
    			branch = B_BNE;
    			break;
    		case F3_BLT:
    			instType = BLT;
    			branch = B_BLT;
    			break;
    		case F3_BGE:
    			instType = BGE;
    			branch = B_BGE;
    			break;
    		default:
    			excep = UNDEF_INST;
				excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    	}
    	if (excep == NO_EXCEPTION && singleStep) {
    		fprintf(stderr, "pc 0x%x    %08x    %s x%d, x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rs1, rs2, val2);
    	}
    }
    else if (OP == OP_AUIPC) {
    	readREG1 = 0;
    	val1 = pc;
    	val2 = instr & 0xfffff000;
    	val2 = signExt(val2, 32);
    	opType = OPADD;
   		instType = AUIPC;
   		if (singleStep) {
   			fprintf(stderr, "pc 0x%x    %08x    %s x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, val2);
   		}
    }
    else if (OP == OP_LUI) {
    	readREG1 = 0;
    	val1 = instr & 0xfffff000;
    	val1 = signExt(val1, 32);
    	opType = OPMOV;
    	instType = LUI;
    	if (singleStep) {
    		fprintf(stderr, "pc 0x%x    %08x    %s x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, val1);
    	}
    }
    else if (OP == OP_JAL) {
    	opType = OPJMP;
    	instType = JAL;
    	branch = B_NO;
    	readREG1 = 0;
    	val1 = pc;
    	val2 = (((instr>>31)&0x1)<<20) | (((instr>>12)&0xff)<<12);
    	val2 |= (((instr>>20)&0x1)<<11) | (((instr>>21)&0x3ff)<<1);
    	val2 = signExt(val2, 21);
    	WB_IF_old.bubble = 2;
    	if (singleStep) {
    		fprintf(stderr, "pc 0x%x    %08x    %s x%d, 0x%lx\n", pc, instr, INSTNAME[instType], rd, val2);
    	}
    }
    else {
    	excep = UNDEF_INST;
		excepStatus = (OP<<10) + (fuc3<<7) + fuc7;
    }

    if (excep == UNDEF_INST) {
    	opType = OPNONE;
    	writeREG = 0;
    	readREG1 = 0;
    	readREG2 = 0;
    	extTop = 0;
    	branch = 0;
    }


    // regs hazard
    if (readREG1) {
    	if (rs1 == IF_ID.rd && IF_ID.writeREG && !IF_ID.bubble) {		// conflict of previous instruction
    		if (singleStep) {
    			fprintf(stderr, "rs1 hazard with previous instruction\n");
    		}
    		WB_IF_old.bubble = 1;
    		++staRegHazard;
    		return;
    	}
    	else if (rs1 == ID_EX.rd && ID_EX.writeREG && !ID_EX.bubble) {
    		if (singleStep) {
    			fprintf(stderr, "rs1 hazard with previous 2 instruction\n");
    		}
    		WB_IF_old.bubble = 1;
    		++staRegHazard;
    		return;
    	}
    	else if (rs1 == EX_MEM.rd && EX_MEM.writeREG && !EX_MEM.bubble) {
    		if (EX_MEM.readSize) {
    			if (singleStep) {
    				fprintf(stderr, "rs1 hazard with previous 3 instruction\n");
    			}
    			WB_IF_old.bubble = 1;
    			++staRegHazard;
    			return;
    		}
    		if (rs1 == 0) val1 = 0;
    		else val1 = EX_MEM.res;
    		readREG1 = 0;
    	}
    }

    if (readREG2) {
    	if (rs2 == IF_ID.rd && IF_ID.writeREG && !IF_ID.bubble) {		// conflict of previous instruction
    		if (singleStep) {
    			fprintf(stderr, "rs2 hazard with previous instruction\n");
    		}
    		WB_IF_old.bubble = 1;
    		++staRegHazard;
    		return;
    	}
    	else if (rs2 == ID_EX.rd && ID_EX.writeREG && !ID_EX.bubble) {
    		if (singleStep) {
    			fprintf(stderr, "rs2 hazard with previous 2 instruction\n");
    		}
    		WB_IF_old.bubble = 1;
    		++staRegHazard;
    		return;
    	}
    	else if (rs2 == EX_MEM.rd && EX_MEM.writeREG && !EX_MEM.bubble) {
    		if (EX_MEM.readSize) {
    			if (singleStep) {
    				fprintf(stderr, "rs2 hazard with previous 3 instruction\n");
    			}
    			WB_IF_old.bubble = 1;
    			++staRegHazard;
    			return;
    		}
    		if (writeSize) {
    			if (rs2 == 0) writeVal = 0;
    			else writeVal = EX_MEM.res;
    		}
    		else {
    			if (rs2 == 0) val2 = 0;
    			else val2 = EX_MEM.res;
    		}
    		readREG2 = 0;
    	}
    }


    // write new pc
    WB_IF_old.pc = npc;
	//write IF_ID_old
	IF_ID_old.instType = instType;
	IF_ID_old.opType = opType;
	IF_ID_old.npc = npc;
	IF_ID_old.pc = pc;
	IF_ID_old.rd = rd;
	IF_ID_old.rs1 = rs1;
	IF_ID_old.readREG1 = readREG1;
	IF_ID_old.val1 = val1;
	IF_ID_old.rs2 = rs2;
	IF_ID_old.readREG2 = readREG2;
	IF_ID_old.val2 = val2;
	IF_ID_old.branch = branch;
	IF_ID_old.writeSize = writeSize;
	IF_ID_old.writeVal = writeVal;
	IF_ID_old.writeREG = writeREG;
	IF_ID_old.readSize = readSize;
	IF_ID_old.extTop = extTop;
	IF_ID_old.bubble = bubble;
	IF_ID_old.excep = excep;
	IF_ID_old.excepStatus = excepStatus;
}

//译码
void ID() {
	//Read IF_ID
	int bubble = IF_ID.bubble;
	if (bubble) {
		if (singleStep) {
			fprintf(stderr, "\t\t\t\t\t\tID bubble %d\n", bubble);
		}
		ID_EX_old.bubble = bubble;
		return;
	}

	int instType = IF_ID.instType;
	int opType = IF_ID.opType;
	int npc = IF_ID.npc;
	int pc = IF_ID.pc;
	int rd = IF_ID.rd;
	int rs1 = IF_ID.rs1;
	int readREG1 = IF_ID.readREG1;
	REG val1 = IF_ID.val1;
	int rs2 = IF_ID.rs2;
	int readREG2 = IF_ID.readREG2;
	REG val2 = IF_ID.val2;
	int branch = IF_ID.branch;
	int writeREG = IF_ID.writeREG;
	int writeSize = IF_ID.writeSize;
	int readSize = IF_ID.readSize;
	int extTop = IF_ID.extTop;
	int excep = IF_ID.excep;
	int excepStatus = IF_ID.excepStatus;
	REG writeVal = IF_ID.writeVal;

	if (readREG1) {
		if (rs1 == 0) val1 = 0;
		else val1 = reg[rs1];
	}
	if (readREG2) {
		if (writeSize) {
			if (rs2 == 0) writeVal = 0;
			else writeVal = reg[rs2];
		}
		else {
			if (rs2 == 0) val2 = 0;
			else val2 = reg[rs2];
		}
	}


	//write ID_EX_old
	ID_EX_old.rd = rd;
	ID_EX_old.opType = opType;
	ID_EX_old.val1 = val1;
	ID_EX_old.val2 = val2;
	ID_EX_old.writeVal = writeVal;
	ID_EX_old.extTop = extTop;
	ID_EX_old.readSize = readSize;
	ID_EX_old.writeSize = writeSize;
	ID_EX_old.branch = branch;
	ID_EX_old.npc = npc;
	ID_EX_old.pc = pc;
	ID_EX_old.writeREG = writeREG;
	ID_EX_old.excep = excep;
	ID_EX_old.excepStatus = excepStatus;
	ID_EX_old.bubble = bubble;
}

//执行
void EX() {
	int bubble = ID_EX.bubble;
	if (bubble) {
		if (singleStep) {
			fprintf(stderr, "\t\t\t\t\t\tEX bubble %d\n", bubble);
		}
		EX_MEM_old.bubble = bubble;
		++staCycle;
		return;
	}
	//read ID_EX
	int npc = ID_EX.npc;
	int pc = ID_EX.pc;				// for debug
	int opType = ID_EX.opType;
	REG val1 = ID_EX.val1;
	REG val2 = ID_EX.val2;
	int branch = ID_EX.branch;
	// not use, but transfrom
	int rd = ID_EX.rd;
	int extTop = ID_EX.extTop;
	int readSize = ID_EX.readSize;
	int writeSize = ID_EX.writeSize;
	REG writeVal = ID_EX.writeVal;
	int writeREG = ID_EX.writeREG;
	int excep = ID_EX.excep;
	int excepStatus = ID_EX.excepStatus;

	REG res;			// result of ALU
	int errorPredict = 0;		// error predicttion of branch
	// ALU calculate
	switch (opType) {
		case OPNONE:
			break;
		case OPADD:
			res = val1 + val2;
			break;
		case OPSUB:
			res = val1 - val2;
			break;
		case OPMUL:
			res = val1 * val2;
			staCycle += CYCLE_MUL - CYCLE_NORMAL;
			break;
		case OPMULH:
			res = mulh(val1, val2);
			staCycle += CYCLE_MULH - CYCLE_NORMAL;
			break;
		case OPDIV:
			if (val2 == 0) {
				excep = DIVIDE_ZERO;
				excepStatus = pc;
				writeREG = 0;
				return;
			}
			else res = val1 / val2;
			staCycle += CYCLE_DIV - CYCLE_NORMAL;
			break;
		case OPREM:
			if (val2 == 0) {
				excep = DIVIDE_ZERO;
				excepStatus = pc;
				writeREG = 0;
				return;
			}
			else res = val1 % val2;
			staCycle += CYCLE_DIV - CYCLE_NORMAL;
			break;
		case OPSL:
			res = val1 << val2;
			break;
		case OPSR:
			res = val1 >> val2;
			break;
		case OPSLT:
			res = (long long)val1 < (long long)val2 ? 1 : 0;
			break;
		case OPXOR:
			res = val1 ^ val2;
			break;
		case OPOR:
			res = val1 | val2;
			break;
		case OPAND:
			res = val1 & val2;
			break;
		case OPJMP:
			res = npc;
			WB_IF_old.pc = (val1 + val2) & -2;
			break;
		case OPMOV:
			res = val1;
			break;
		default:
			fprintf(stderr, "Undefined opType\n");
			ASSERT(false);
	}
	++staCycle;

	// Branch PC calulate
	switch (branch) {
		case B_NO:
			break;
		case B_BEQ:
			if (branchCorrect(val1 == val2)) branch = B_NO;
			else errorPredict = 1;
			break;
		case B_BNE:
			if (branchCorrect(val1 != val2)) branch = B_NO;
			else errorPredict = 1;
			break;
		case B_BLT:
			if (branchCorrect((long long)val1 < (long long)val2)) branch = B_NO;
			else errorPredict = 1;
			break;
		case B_BGE:
			if (branchCorrect((long long)val1 >= (long long)val2)) branch = B_NO;
			else errorPredict = 1;
			break;
		default:
			fprintf(stderr, "Undefined branch type\n");
			ASSERT(false);
	}

	// branch hazard
	if (errorPredict) {
		staCtrlHazard += 2;
		IF_ID_old.bubble = 1;
		ID_EX_old.bubble = 1;
		WB_IF_old.pc = branchCorrectPC();
		if (singleStep) {
			fprintf(stderr, "branch predict error at 0x%x and change to 0x%x\n", pc, WB_IF_old.pc);
		}
	}


	//write EX_MEM_old
	EX_MEM_old.res = res;
	EX_MEM_old.pc = pc;
	EX_MEM_old.rd = rd;
	EX_MEM_old.extTop = extTop;
	EX_MEM_old.readSize = readSize;
	EX_MEM_old.writeSize = writeSize;
	EX_MEM_old.writeVal = writeVal;
	EX_MEM_old.writeREG = writeREG;
	EX_MEM_old.bubble = bubble;
	EX_MEM_old.excep = excep;
	EX_MEM_old.excepStatus = excepStatus;
}

//访问存储器
void MEM() {
	//read EX_MEM
	int bubble = EX_MEM.bubble;
	if (bubble) {
		if (singleStep) {
			fprintf(stderr, "\t\t\t\t\t\tMEM bubble %d\n", bubble);
		}
		MEM_WB_old.bubble = bubble;
		return;
	}
	REG res = EX_MEM.res;
	int pc = EX_MEM.pc;
	int rd = EX_MEM.rd;
	int extTop = EX_MEM.extTop;
	int readSize = EX_MEM.readSize;
	int writeSize = EX_MEM.writeSize;
	REG writeVal = EX_MEM.writeVal;
	int writeREG = EX_MEM.writeREG;
	int excep = EX_MEM.excep;
	int excepStatus = EX_MEM.excepStatus;


	// memory hazard
	if (writeSize && memConflict(EX_MEM_old.pc, 4, res, writeSize)) {		// conflict between MEM and EX
		if (singleStep) {
			fprintf(stderr, "memory hazard between 0x%x and 0x%x\n", pc, EX_MEM_old.pc);
		}
		EX_MEM_old.bubble = 1;
		ID_EX_old.bubble = 1;
		IF_ID_old.bubble = 1;
		WB_IF_old.pc = ID_EX_old.pc;
		staMemHazard += 3;
	}
	else if (writeSize && !ID_EX_old.bubble && memConflict(ID_EX_old.pc, 4, res, writeSize)) {		// conflict between MEM and ID
		if (singleStep) {
			fprintf(stderr, "memory hazard between 0x%x and  0x%x\n", pc, ID_EX_old.pc);
		}
		ID_EX_old.bubble = 1;
		IF_ID_old.bubble = 1;
		WB_IF_old.pc = IF_ID_old.pc;
		staMemHazard += 2;
	}
	else if (writeSize && !IF_ID_old.bubble && memConflict(IF_ID_old.pc, 4, res, writeSize)) {
		if (singleStep) {
			fprintf(stderr, "memory hazard between 0x%x and 0x%x\n", pc, IF_ID_old.pc);
		}
		IF_ID_old.bubble = 1;
		WB_IF_old.pc = WB_IF_old.pc;
		staMemHazard += 1;
	}


	//read / write memory
	if (readSize) {
		int addr = res;
		int excep = readMem(addr, (void*)&res, readSize);
		if (excep) raiseException(PAGE_FAULT, addr, pc);
		readSize = 0;
	}

	if (extTop) {
		res = signExt(res, extTop);
		extTop = 0;
	}

	if (writeSize) {
		int excep = writeMem(res, (void*)&writeVal, writeSize);
		if (excep) raiseException(PAGE_FAULT, res, pc);
		writeSize = 0;
	}


	// write MEM_WB_old
	MEM_WB_old.pc = pc;
	MEM_WB_old.res = res;
	MEM_WB_old.rd = rd;
	MEM_WB_old.writeREG = writeREG;
	MEM_WB_old.bubble = bubble;
	MEM_WB_old.excep = excep;
	MEM_WB_old.excepStatus = excepStatus;
}


//写回
void WB() {
	int bubble = MEM_WB.bubble;
	if (bubble) {
		if (singleStep) {
			fprintf(stderr, "\t\t\t\t\t\tWB bubble %d\n", bubble);
		}
		return;
	}
	//read MEM_WB
	int pc = MEM_WB.pc;
	int rd = MEM_WB.rd;
	REG res = MEM_WB.res;
	int writeREG = MEM_WB.writeREG;
	int excep = MEM_WB.excep;
	int excepStatus = MEM_WB.excepStatus;
	// exception
	if (excep != NO_EXCEPTION) {
		raiseException(excep, excepStatus, pc);
	}
	//write reg
	if (writeREG) {
		reg[rd] = res;
//printf("\t\tpc 0x%x, write at x%d, value %x\n", pc, rd, res);
	}

	++staInstr;
}


REG signExt(unsigned int src, int bit) {
	REG sign = (src >> (bit-1)) & 0x1;
	return (~((sign << bit) - 1) | (src & (((REG)1<<bit)-1)));
}

REG mulh(REG a, REG b) {
	REG ah = (a & 0xffffffff00000000) >> 32;
	REG al = a & 0xffffffff;
	REG bh = (b & 0xffffffff00000000) >> 32;
	REG bl = b & 0xffffffff;
	REG res = ah * bh;
	REG t1 = ah * bl;
	REG t2 = al * bh + t1;
	res += t2 < t1 ? ((REG)1<<32) : 0;
	res += t2 >> 32;
	t2 <<= 32;
	t1 = t2 + al * bl;
	res += t1 < t2 ? 1 : 0;
	return res;
}

void printREG() {
	for (int i = 0; i != 4; ++i) {
		for (int j = 0; j != 7; ++j) {
			fprintf(stderr, "x%d %lx\t", i*8+j, reg[i*8+j]);
		}
		fprintf(stderr, "x%d %lx\n", i*8+7, reg[i*8+7]);
	}
	fprintf(stderr, "\n>");
}


//-----------------------------------------------------------------------------------------------------------
// memory part
//-----------------------------------------------------------------------------------------------------------

// read from memory
int readMem(int addr, void *val, int size) {
	int phyAddr;
	int excep = translate(addr, &phyAddr, size, 0);
	if (excep) return excep;
	switch (size) {
		case 1:			// byte
			*(char*)val = memory[phyAddr];
			break;
		case 2:
			*(short*)val = *(short*)(memory+phyAddr);
			break;
		case 4:
			*(int*)val = *(int*)(memory+phyAddr);
			break;
		case 8:
			*(REG*)val = *(REG*)(memory+phyAddr);
			break;
		default:
			fprintf(stderr, "Undefined read size\n");
			ASSERT(false);
	}
	return NO_EXCEPTION;
}



int writeMem(int addr, void *val, int size) {
	int phyAddr;
	int excep = translate(addr, &phyAddr, size, 1);
	if (excep) return excep;
	switch (size) {
		case 1:
			memory[addr] = *(char*)val;
			break;
		case 2:
			*(short*)(memory+addr) = *(short*)val;
			break;
		case 4:
			*(int*)(memory+addr) = *(int*)val;
			break;
		case 8:
			*(REG*)(memory+addr) = *(REG*)val;
			break;
		default:
			fprintf(stderr, "Undefined write size\n");
			ASSERT(false);
	}
	return NO_EXCEPTION;
}


int translate(int virAddr, int *phyAddr, int size, int writing) {
	// linear translation
	int excep = NO_EXCEPTION;
	if (virAddr + size > MAXADDR || virAddr < 0) {
		excep = PAGE_FAULT;
		return excep;
	}
	*phyAddr = virAddr;
	return excep;
}

void loadMemory(ELFIO::elfio *reader) {
	ELFIO::Elf_Half seg_num = reader->segments.size();
	for (int i = 0; i != seg_num; ++i) {
		const ELFIO::segment *pseg = reader->segments[i];

		uint64_t fullmemsz = pseg->get_memory_size();
		uint64_t fulladdr = pseg->get_virtual_address();

		if (fulladdr + fullmemsz > MAXADDR) {
			fprintf(stderr, "ELF file too big\n");
			exit(-1);
		}

		uint32_t filesz = pseg->get_file_size();
		uint32_t memsz = pseg->get_memory_size();
		uint32_t addr = (uint32_t)pseg->get_virtual_address();

		for (uint32_t p = addr; p < addr + memsz; ++p) {
			if (p < addr + filesz)
				memory[p] = pseg->get_data()[p-addr];
		}
	}
}

bool memConflict(int readAddr, int readSize, int writeAddr, int writeSize) {
	int readEnd = readAddr + readSize;
	int writeEnd = writeAddr + writeSize;
	if (readAddr >= writeEnd) return false;
	if (writeAddr >= readEnd) return false;
	return true;
}






//-----------------------------------------------------------------------------------------------------------
// branch part
//-----------------------------------------------------------------------------------------------------------

int branchPC(int npc, int bpc) {
	branchNPC = npc;
	branchBPC = bpc;
	switch (branchPolicy) {
		case POL_ANT:
			return npc;
		case POL_AT:
			return bpc;
		case POL_BTFN:
			if (bpc > npc) return npc;
			return bpc;
		default:
			fprintf(stderr, "Undefined branch policy %d\n", branchPolicy);
			ASSERT(false);
	}
}

bool branchCorrect(bool condition) {
	switch (branchPolicy) {
		case POL_ANT:
			return !condition;
		case POL_AT:
			return condition;
		case POL_BTFN:
			if (branchBPC > branchNPC) return !condition;
			return condition;
		default:
			fprintf(stderr, "Undefined branch policy %d\n", branchPolicy);
			ASSERT(false);
	}
}

int branchCorrectPC() {
	switch (branchPolicy) {
		case POL_ANT:
			return branchBPC;
		case POL_AT:
			return branchNPC;
		case POL_BTFN:
			if (branchBPC > branchNPC) return branchBPC;
			return branchNPC;
		default:
			fprintf(stderr, "Undefined branch policy %d\n", branchPolicy);
			ASSERT(false);
	}
}




//-----------------------------------------------------------------------------------------------------------
// exception part
//-----------------------------------------------------------------------------------------------------------

void raiseException(int type, int status, int pc) {
	if (singleStep) fprintf(stderr, "raise exception type %d, pc %x\n", type, pc);
	excepTable[type].valid = 1;
	excepTable[type].status = status;
	excepTable[type].pc = pc;
}


void exceptionHandler() {
	if (excepTable[PAGE_FAULT].valid) {
		// page fault exception will not happen
		fprintf(stderr, "Exception: not ready for page fault at 0x%x\n", excepTable[PAGE_FAULT].pc);
		ASSERT(false);
	}
	if (excepTable[UNDEF_INST].valid) {
		int inst = excepTable[UNDEF_INST].status;
		fprintf(stderr, "Exception: undefined instruction at 0x%x, opcode = 0x%x, funct3 = 0x%x, funct7 = 0x%x\n", 
			excepTable[UNDEF_INST].pc, (inst>>10)&0x7f, (inst>>7)&0x7, inst&0x7f);
		ASSERT(false);
	}
	if (excepTable[SYSCALL].valid) {
		syscallHandler();
	}
	if (excepTable[DIVIDE_ZERO].valid) {
		fprintf(stderr, "Exception: divide zero at 0x%x\n", excepTable[DIVIDE_ZERO].pc);
		ASSERT(false);
	}
}

bool checkException() {
	if (excepTable[PAGE_FAULT].valid) return false;
	return true;
}


void syscallHandler() {
	excepTable[SYSCALL].valid = 0;
	REG val = reg[10];
	REG type = reg[17];

	switch (type) {
		case 1:
			printf("%c", val);
			break;
		case 2:
			printf("%d", val);
			break;
		case 10:
			printf("\n");
			exit(0);
		default:
			fprintf(stderr, "undefined syscall type %d\n", type);
			ASSERT(false);
	}
}


//-----------------------------------------------------------------------------------------------------------
// static part
//-----------------------------------------------------------------------------------------------------------
void PrintStatic() {
	fprintf(stderr, "\n");
	fprintf(stderr, "Cycles %d, instructions %d, CPI %lf\n", staCycle, staInstr, (double)staCycle / (double)staInstr);
	fprintf(stderr, "Memory hazard %d, register hazard %d, branch hazard %d\n", staMemHazard, staRegHazard, staCtrlHazard);
}