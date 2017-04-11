/*
 * pipeline.h
 *
 *  Created on: Feb 16, 2017
 *      Author: Brandon Chambers
 *      Author: Thomas Xu
 *
 *  In the MIPS architecture there are 5 pipeline stages:
 *    IF: Instruction Fetch, which represents instruction memory/register
 *    ID: Instruction Decode & register file read
 *    EX: Execution, representing the ALU (arithmetic and logic unit)
 *    MEM: Memory (RAM) access stage
 *    WB: Write Back to the registers
 *  The data path follows in that order, and in between each of the
 *  five pipeline stages are other blocks in the hardware.
 *
 *  REFERENCES: see projmain.c header comment.
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "instruction.h"

/******************************************************************************
 * Constants/Definitions
 */
#define LW_CLOCK_WAIT 100 //simulation of load word time to process
#define MAX_LINE_LENGTH 256
#define MAX_LENGTH 32

/******************************************************************************
 * Global Vars and Structs
 */

/*Latch is a way to move ops/instrs through each stage of pipeline;
 using this, we can stop the current operations mid-propagation and
 start new instructions by flushing the latches of their fields
 enable data-forwarding for concurrency/threading*/
typedef struct latch_tag {
	bool valid;
	bool readyToWork;
	instr inst;
} latch;

//same as latch, but with a data field for propagating data through pipeline
typedef struct _data_latch_tag {
	bool valid;
	bool readyToWork;
	int32_t data;
	instr inst;
} d_latch;


int32_t offsetSW = 0; //to save the calculated offset for the 'sw' instr
int32_t offsetLW = 0; //to save the calculated offset for the 'lw' instr
//counter for how many clock cycles the program uses
int32_t clocks = 0;
//counters to calculate the utilization ratio of each pipeline stage
int32_t usageIF = 0;
int32_t usageID = 0;
int32_t usageEX = 0;
int32_t usageMEM = 0;
int32_t usageWB = 0;
//the main RAM memory, aka 'data memory'
int32_t RAM[512];
//the register file representing each MIPS register and holding their contents
int32_t regs[32];

bool branchWaiting = false;
bool allWorkCompleted = false; //when halt goes through pipeline

instr bubble = { B, BUBBLE, 0, 0, 0, 0, false };
//go-between latches for pipeline STAGE-TO-STAGE - 'connections'
latch IF_ID = { .readyToWork = false };
latch ID_EX = { .readyToWork = false };
//data latches simply add 'data' fields to their structs to move data
d_latch EX_MEM = { .readyToWork = false };
d_latch MEM_WB = { .readyToWork = false };

/******************************************************************************
 * Function Prototypes
 */
//Pipeline Stage declarations
void IF();
void ID();
void EX();
void MEM();
void WB();

int isHazard();

/******************************************************************************
 * Functions
 */

/**
 * Instruction Fetch, represents the instruction register (ir) or instruction
 * memory (im).  The first pipeline stage, that retrieves the instruction then
 * passes it on to the second stage: ID
 */
void IF() {
	if (!branchWaiting) {
		if (!IF_ID.valid) {
			IF_ID.valid = true;
			IF_ID.inst = instructions[pc];
			if (pc < haltIndex)
				pc++;
			usageIF++;
			if (!IF_ID.readyToWork)
				IF_ID.readyToWork = true;
		}
	} else { //branchWaiting
		/*
		 * Waiting on the completion of a branch
		 * Hand off to ID? Premature return?
		 * Implement data forwarding or other optimizations?
		 */
	}
} //end function IF()

/**
 * Instruction Decode, represents parsing the instruction/assembling the instr
 * or interpreting into machine code bits for the cpu/alu to work with.  The
 * architecture also reads the register files in this stage. Second pipeline
 * stage that feeds into EX.
 */
void ID() {
	if (IF_ID.valid && IF_ID.readyToWork && !ID_EX.valid) {
		if (isHazard() == -1) { //if no hazard
			//If it's a branch, send it along to ex, IF will wait
			if (IF_ID.inst.op == BEQ)
				branchWaiting = true;
			IF_ID.valid = false;
			ID_EX.valid = true;
			ID_EX.inst = IF_ID.inst; //push instruction up the pipe
			if (ID_EX.inst.type != B)  //if not a bubble we did work here
				usageID++;
			if (!ID_EX.readyToWork)
				ID_EX.readyToWork = true;
		} else { //instruction is a bubble
			ID_EX.valid = true;
			ID_EX.inst = bubble;
		} //end inner else
	} //end big if
} //end function ID()

/**
 * Execution, representing the ALU (arithmetic and logic unit) which takes the
 * machine code constructed from assembly by the ID, and does the indicated
 * operations on the data/registers.  Third stage which passes results on to
 * the MEM stage.
 */
void EX() {
	if (ID_EX.readyToWork && ID_EX.valid) {
		//artificial cycles to represent how long it takes
		static int exCycles = 0;
		if (ID_EX.inst.type == B) { //it's a bubble
			if (!EX_MEM.valid) {
				ID_EX.valid = false;
				EX_MEM.valid = true;
				EX_MEM.inst = ID_EX.inst; //push bubble up the pipe
			}
		} else {
			if (!EX_MEM.valid
					&& ((exCycles == 15 && ID_EX.inst.op == MUL)
							|| (exCycles == 10 && ID_EX.inst.op != MUL))) {
				if (ID_EX.inst.rs > 31 || ID_EX.inst.rt > 31) {
					printf("\n>>>ERROR!\n******Invalid register location,"
							" rs: * %d * and rt: * %d *\n\tFrom: pipeline.h"
							" @ line 168\n", ID_EX.inst.rs, ID_EX.inst.rt);
					exit(1);
				}
				if (ID_EX.inst.op == ADD)
					EX_MEM.data = regs[ID_EX.inst.rs] + regs[ID_EX.inst.rt];
				else if (ID_EX.inst.op == ADDI)
					EX_MEM.data = regs[ID_EX.inst.rs] + ID_EX.inst.i;
				else if (ID_EX.inst.op == SUB)
					EX_MEM.data = regs[ID_EX.inst.rs] - regs[ID_EX.inst.rt];
				else if (ID_EX.inst.op == AND)
					EX_MEM.data = regs[ID_EX.inst.rs] & regs[ID_EX.inst.rt];
				else if (ID_EX.inst.op == OR)
					EX_MEM.data = regs[ID_EX.inst.rs] | regs[ID_EX.inst.rt];
				else if (ID_EX.inst.op == MUL)
					EX_MEM.data = regs[ID_EX.inst.rs] * regs[ID_EX.inst.rt];
				else if (ID_EX.inst.op == BEQ) {
					if (regs[ID_EX.inst.rs] == regs[ID_EX.inst.rt]) {
						pc = pc + ID_EX.inst.i;
						if (pc > haltIndex) {
							printf("\n>>>ERROR!\n******Branched beyond "
									"program boundaries, pc: * %d * and "
									"haltIndex: * %d *\n\tFrom: pipeline.h"
									" @ line 189\n", pc, haltIndex);
							exit(1);
						} //inner inner inner if
					}
					branchWaiting = false;
					/*
					 * LW and SW code here!
					 */
				} else if (ID_EX.inst.op == LW || ID_EX.inst.op == SW) {
					if (ID_EX.inst.i % 4 == 0) {
						/* TODO: NOTE!
						 Storing 'rt' into mem.data is CORRECT!
						 the first reg in a 'sw' instr is the data to be
						 stored in memory, while the reg's value in parenthesis
						 is used as an address + specified offset with which
						 to store 'rt' data in the memory
						 */
						EX_MEM.data = regs[ID_EX.inst.rt];
//						EX_MEM.data = regs[ID_EX.inst.rs]; NO, not this way

						if (ID_EX.inst.op == SW) {
							EX_MEM.inst.rd = regs[ID_EX.inst.rs]
									+ (ID_EX.inst.i / 4);
							offsetSW = EX_MEM.inst.rd; //save offset
						}
						if (ID_EX.inst.op == LW) {
							EX_MEM.inst.rs = regs[ID_EX.inst.rs]
									+ (ID_EX.inst.i / 4);
							offsetLW = EX_MEM.inst.rs;
						}
					} else {
						printf("\n>>>ERROR!\n******Memory Misaligned/Access,"
								"\n\tFrom: pipeline.h @ line 223\n");
						exit(1);
					}
				} else {
					printf("\n>>>ERROR!\n******Unrecognized Operation,"
							"\n\tFrom: pipeline.h @ line 228\n");
					exit(1);
				}
				exCycles = 0;
				ID_EX.valid = false;
				EX_MEM.valid = true;
				EX_MEM.inst = ID_EX.inst; //push instr up pipe to MEM
				if (!EX_MEM.readyToWork)
					EX_MEM.readyToWork = true;
			} else if ((exCycles < 15 && ID_EX.inst.op == MUL)
					|| (exCycles < 10 && ID_EX.inst.op != MUL)) {
				exCycles++;
			}
			usageEX++;

		} //end big outer else
	} //end big outer if
} //end function EX()

/**
 * Memory, the fourth stage represents the data memory (not registers or cache)
 * and is system RAM.  Any data that needs to be stored or loaded, as indicated
 *  by the instruction, will be written or read here.
 */
void MEM() {
	if (EX_MEM.readyToWork && EX_MEM.valid) {
		if (EX_MEM.inst.type == B) { //pushing the bubble up
			if (!MEM_WB.valid) {
				EX_MEM.valid = false;
				MEM_WB.valid = true;
				MEM_WB.inst = EX_MEM.inst;
			}
		} else {
			//artificial cycles to represent how long it takes
			static int memCycles = 0;
			bool is_lw = EX_MEM.inst.op == LW;
			bool is_sw = EX_MEM.inst.op == SW;
			/*
			 * WE MADE IT HERE FOR DEBUGGING LW!!!!!
			 */
			if (is_lw || is_sw) {
				if (memCycles == LW_CLOCK_WAIT && !MEM_WB.valid) {
					memCycles = 0;
					EX_MEM.valid = false;
					MEM_WB.valid = true;
					MEM_WB.inst = EX_MEM.inst;
					if (!MEM_WB.readyToWork) {
						MEM_WB.readyToWork = true;
					}
					/*
					 * Load Word from Memory/RAM into Register
					 */
					if (is_lw) {
						if (&RAM[EX_MEM.data] == NULL) {
							printf("\n>>>ERROR!\n******Invalid Memory Read,"
									"\n\tFrom: pipeline.h @ line 283\n");
							exit(1);
						}
						/*
						 * TODO: NOTE! Works now. For real.
						 */
//						MEM_WB.data = RAM[regs[EX_MEM.inst.rs]];
						MEM_WB.data = RAM[offsetLW];
					}
					/**
					 * Store Word into Memory/RAM
					 */
					if (is_sw) {
						if (&RAM[EX_MEM.data] == NULL) {
							printf("\n>>>ERROR!\n******Invalid Memory Write,"
									"\n\tFrom: pipeline.h @ line 298\n");
							exit(1);
						}
						/**
						 * TODO:  NOTE!  Works now. For real.
						 */
						RAM[offsetSW] = EX_MEM.data;
					}
				} else if (memCycles < LW_CLOCK_WAIT)
					memCycles++;
			} else { //not lw && not sw
				EX_MEM.valid = false;
				MEM_WB.valid = true;
				MEM_WB.inst = EX_MEM.inst;
				MEM_WB.data = EX_MEM.data;
				if (!MEM_WB.readyToWork)
					MEM_WB.readyToWork = true;
			}
			if (EX_MEM.inst.type != B)
				usageMEM++;
		} //end big else
	} // end big if
} //end function MEM()

/**
 * Write Back, the fifth and final pipeline stage is where whatever data values
 * that need to be stored inside registers/cache will be written to. This data
 * could have been passed from another register, loaded from MEM/RAM, or
 * calculated by EX in the ALU then passed into the register/cache.
 */
void WB() {
	if (MEM_WB.valid && MEM_WB.readyToWork) {
		if (MEM_WB.inst.op != SW && MEM_WB.inst.op != BEQ
				&& MEM_WB.inst.op != HALT && MEM_WB.inst.rd != 0
				&& MEM_WB.inst.type != B) {
			regs[MEM_WB.inst.rd] = MEM_WB.data; //data latch

			usageWB++;
		}
		if (MEM_WB.inst.type == B && MEM_WB.inst.isHalt) {
			allWorkCompleted = true; //halt execution, end program
		}
		MEM_WB.valid = false;
	}
}

/**
 * Check for hazards:
 * pipeline data hazards or structure hazards, etc.
 *
 * No hazards possible on register 0.
 */
int isHazard() {
	instr inst = IF_ID.inst;
	//branch/control hazard
	if (IF_ID.inst.type != B) {
		if (ID_EX.readyToWork && inst.rs == ID_EX.inst.rd
				&& ID_EX.inst.op != SW)
			if (inst.rs != 0)
				return inst.rs;

		if (EX_MEM.readyToWork && inst.rs == EX_MEM.inst.rd
				&& EX_MEM.inst.op != SW)
			if (inst.rs != 0)
				return inst.rs;

		if (MEM_WB.readyToWork && inst.rs == MEM_WB.inst.rd
				&& MEM_WB.inst.op != SW)
			if (inst.rs != 0)
				return inst.rs;

		if (inst.type == R || inst.op == BEQ) {
			//Need to check that rs and rt aren't targets of future ops
			if (ID_EX.readyToWork && inst.rt == ID_EX.inst.rd
					&& ID_EX.inst.op != SW)
				if (inst.rt != 0)
					return inst.rt;

			if (EX_MEM.readyToWork && inst.rt == EX_MEM.inst.rd
					&& EX_MEM.inst.op != SW)
				if (inst.rt != 0)
					return inst.rt;

			if (MEM_WB.readyToWork && inst.rt == MEM_WB.inst.rd
					&& MEM_WB.inst.op != SW)
				if (inst.rt != 0)
					return inst.rt;
		} //end medium inner if
	} //end big if

	return -1;
} //end function hazard()

#endif /* PIPELINE_H_ */
