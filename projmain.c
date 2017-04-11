/*
 * projmain.c
 *
 *  Created on: Feb 16, 2017
 *      Author: Brandon Chambers
 *      Author: Thomas Xu
 *
 *  MIPS Assembler and Simulator w/ command line interface
 *
 * REFERENCES: (Many codes were used as inspiration and guidance, including
 * our text books for this course and previous LC3 simulator assignments from
 * 371 Machine Organization, following is a non-exhaustive list of online
 * resources we found helpful.)
 *  https://github.com/nkavanagh/mips-tutorials
 *  https://github.com/nayefc/mips-simulator
 *  https://github.com/nayefc/mips-assembler
 *  https://github.com/clord/MIPS-CPU-Simulator
 *  http://cs.nyu.edu/courses/spring13/CSCI-GA.1144-001/project.html
 *  xavier.perseguers.ch/fileadmin/download/mips-assembler/rm.pdf
 *  https://www.ncsu.edu/wcae/MICRO2009/p32-fujieda.pdf
 *  http://courses.missouristate.edu/KenVollmar/mars/
 *
 *  INSTRUCTOR/Grader PLEASE DISREGARD ALL TODOs
 *
 *	TODOs are in order of importance (left for future improvements)
 *
 *  TODO: Requirements for the project: xADD, xSUB, xAND, xOR, xLW, xSW
 *
 *  TODO: Add all instructions (like lui, sll, etc.)---
 *   go to the function @parseInstruction(0, 1) in the file instruction.h
 *   and add what's needed. Also go to instruction.h and alter the "isIType",
 *   "isRType", etc. functions to work with new operations.
 *   Then go to file pipeline.h and the EX() function to add the
 *    actual operations.
 *
 *  TODO: Finish the ouput file, which I've begun passing through to
 *  main-->parseASMFile-->parseInstruction
 *  add the binary machine codes for the instruction, likely to happen directly
 *  from the parseInstruction() function (hopefully that simple).
 *
 *  TODO: related to output file: make it an object.obj file just like a
 *  real assembler would do, then maybe have the program automatically
 *  pull the bits from that file to simulate the operations (rather than the
 *  direct/hardcoded way it's happening now) be mindful of the Pipeline!
 *
 *  TODO: also related to the output file: add a getBin() function to convert
 *  to binary, already have written a couple in other C files...
 *
 *  TODO: consolidate and minimize, while also seeing about modularizing a bit
 *  more via offloading instruction.h and pipeline.h functions and vars to
 *  a new header file or the existing fileparser.h--where appropriate ofc!
 *
 *  TODO: optimizing the pipeline with data forwarding (these are
 *  classically implemented with a pair of muxes in between the pipeline stages
 *  EX and MEM) and moving the branching execution to the ID as the text book
 *  on page 318-319 mentions.
 *
 *  TODO: investigate other pipeline optimizations that increase concurrency
 *  and threadedness.
 *
 *  TODO: optimize data structures and implement the hashfunction.h and
 *  hashmap.h files to hash our data for quick retrieval (already using binary
 *  search here, but it is temporarily disabled).
 *
 *  TODO: optimize ALL variables into the appropriate size and type, i.e. use
 *  uint8_t and int8_t or char where possible and mix in int16_t with int32_t.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pipeline.h"
#include "instruction.h"
#include "fileparser.h"

/******************************************************************************
 * Function Prototypes
 */
void displayBits();
void printMemory();
void printStatistics();
void printRegisters();

/******************************************************************************
 * Run from command line like so:
 *
 * > gcc projmain.c -o app
 * > app tester.asm output.txt
 *
 * Where 'output.txt' is any named txt file you want - created on demand.
 */
int main(void) {

	char inFile[100];
	char outFile[100];
	char continuity = 'r';

	while (continuity == 'r') {

		printf("\n--------------------------------\n");
		printf("   Welcome to MIPS Assembler!");
		printf("\n--------------------------------\n\n");
		printf("Load File: ");
		scanf("%s", inFile);

		printf("Out File: ");
		scanf("%s", outFile);
		printf("\n");
		parseASMFile(inFile, outFile);

        //Once we've read everything in, reset the program_counter
		pc = 0;
        //Then start iterating over the pipelined stages in reverse
		while (!allWorkCompleted) {
			WB();MEM();EX();ID();IF();
			clocks++;
		}
		printStatistics();
		printMemory();
		printRegisters();

		printf("\nEnter r to repeat, q to quit: \n");
		scanf(" %c", &continuity);
	}
	return 0;
}

/*
 * Outputs the pipeline usage in percentage, per each stage.
 */
void printStatistics() {
	printf("\n\t~~~~~~~ Pipeline Stage Utilization Statistics ~~~~~~~\n");
	printf("\tIF: %19.2f%%\n", 1.0 * usageIF / clocks * 100);
	printf("\tID: %19.2f%%\n", 1.0 * usageID / clocks * 100);
	printf("\tEX: %19.2f%%\n", 1.0 * usageEX / clocks * 100);
	printf("\tMEM: %18.2f%%\n", 1.0 * usageMEM / clocks * 100);
	printf("\tWB: %19.2f%%\n", 1.0 * usageWB / clocks * 100);
	printf("\tExecutionTime: %9d clocks\n\n", clocks);
}

/*
 * Output the contents of our virtual machine's memory space.
 * Only addresses with contents other then NULL/0 will be shown.
 */
void printMemory() {
	printf("\n----------- Memory Contents ------------\n");
	printf(" address\tvalueHex\tvalueDec\n");
	printf("________________________________________\n");
	int i;
	for (i = 0; i < 512; i++) {
		if (RAM[i] != 0x0)
			printf(" 0x%04x->\t0x%08x\t%8d\n", i, RAM[i], RAM[i]);
	}
}

/*
 * Final Output of Register labels and there contents
 */
void printRegisters() {
	printf("\n----------- Register Contents ------------\n");
	printf("index   name     valueHex          valueDec\n");
	printf("___________________________________________\n");
	printf(" 0	zero\t0x%08x%16d\n", regs[0], regs[0]);	//this is aligned
	printf(" 1	$at	0x%08x%16d\n", regs[1], regs[1]);
	printf(" 2	$v0	0x%08x%16d\n", regs[2], regs[2]);
	printf(" 3	$v1	0x%08x%16d\n", regs[3], regs[3]);
	printf(" 4	$a0	0x%08x%16d\n", regs[4], regs[4]);
	printf(" 5	$a1	0x%08x%16d\n", regs[5], regs[5]);
	printf(" 6	$a2	0x%08x%16d\n", regs[6], regs[6]);
	printf(" 7	$a3	0x%08x%16d\n", regs[7], regs[7]);
	printf(" 8	$t0	0x%08x%16d\n", regs[8], regs[8]);
	printf(" 9	$t1	0x%08x%16d\n", regs[9], regs[9]);
	printf(" 10	$t2	0x%08x%16d\n", regs[10], regs[10]);
	printf(" 11	$t3	0x%08x%16d\n", regs[11], regs[11]);
	printf(" 12	$t4	0x%08x%16d\n", regs[12], regs[12]);
	printf(" 13	$t5	0x%08x%16d\n", regs[13], regs[13]);
	printf(" 14	$t6	0x%08x%16d\n", regs[14], regs[14]);
	printf(" 15	$t7	0x%08x%16d\n", regs[15], regs[15]);
	printf(" 16	$s0	0x%08x%16d\n", regs[16], regs[16]);
	printf(" 17	$s1	0x%08x%16d\n", regs[17], regs[17]);
	printf(" 18	$s2	0x%08x%16d\n", regs[18], regs[18]);
	printf(" 19	$s3	0x%08x%16d\n", regs[19], regs[19]);
	printf(" 20	$s4	0x%08x%16d\n", regs[20], regs[20]);
	printf(" 21	$s5	0x%08x%16d\n", regs[21], regs[21]);
	printf(" 22	$s6	0x%08x%16d\n", regs[22], regs[22]);
	printf(" 23	$s7	0x%08x%16d\n", regs[23], regs[23]);
	printf(" 24	$t8	0x%08x%16d\n", regs[24], regs[24]);
	printf(" 25	$t9	0x%08x%16d\n", regs[25], regs[25]);
	printf(" 26	$k0	0x%08x%16d\n", regs[26], regs[26]);
	printf(" 27	$k1	0x%08x%16d\n", regs[27], regs[27]);
	printf(" 28	$gp	0x%08x%16d\n", regs[28], regs[28]);
	printf(" 29	$sp	0x%08x%16d\n", regs[29], regs[29]);
	printf(" 30   $s8/$fp\t0x%08x%16d\n", regs[30], regs[30]);//this is aligned
	printf(" 31	$ra	0x%08x%16d\n", regs[31], regs[31]);
	printf("___________________________________________\n");
	printf(" PC%8d\n", pc);
}

/**
 * TODO: for later
 */
void displayBits(){
	/*
	 * noop
	 * 0000 0000 0000 0000 0000 0000 0000 0000
	 *
	 * add $d, $s, $t
	 * 0000 00ss ssst tttt dddd d000 0010 0000
	 * addi $t, $s, imm
	 * 0010 00ss ssst tttt iiii iiii iiii iiii
	 * addiu $t, $s, imm
	 * 0010 01ss ssst tttt iiii iiii iiii iiii
	 *  addu $d, $s, $t
	 *  0000 00ss ssst tttt dddd d000 0010 0001
	 *
	 *  and $d, $s, $t
	 *  0000 00ss ssst tttt dddd d000 0010 0100
	 *  andi $t, $s, imm
	 *  0011 00ss ssst tttt iiii iiii iiii iiii
	 *
	 *  beq $s, $t, offset
	 *  0001 00ss ssst tttt iiii iiii iiii iiii
	 *  bgez $s, offset
	 *  0000 01ss sss0 0001 iiii iiii iiii iiii
	 *  bne $s, $t, offset
	 *  0001 01ss ssst tttt iiii iiii iiii iiii
	 *
	 *  div $s, $t
	 *  0000 00ss ssst tttt 0000 0000 0001 1010
	 *
	 *  j target
	 *  0000 10ii iiii iiii iiii iiii iiii iiii
	 *  jal target
	 *  0000 11ii iiii iiii iiii iiii iiii iiii
	 *  jr $s
	 *  0000 00ss sss0 0000 0000 0000 0000 1000
	 *
	 *  lb $t, offset($s)
	 *  1000 00ss ssst tttt iiii iiii iiii iiii
	 *  lw $t, offset($s)
	 *  1000 11ss ssst tttt iiii iiii iiii iiii
	 *
	 *  mult $s, $t
	 *  0000 00ss ssst tttt 0000 0000 0001 1000
	 *
	 *
	 *  or $d, $s, $t
	 *  0000 00ss ssst tttt dddd d000 0010 0101
	 *  ori $t, $s, imm
	 *  0011 01ss ssst tttt iiii iiii iiii iiii
	 *
	 *  sb $t, offset($s)
	 *  1010 00ss ssst tttt iiii iiii iiii iiii
	 *  sll $d, $t, h
	 *  0000 00ss ssst tttt dddd dhhh hh00 0000
	 *  srl $d, $t, h
	 *  0000 00-- ---t tttt dddd dhhh hh00 0010
	 *
	 *  sub $d, $s, $t
	 *  0000 00ss ssst tttt dddd d000 0010 0010
	 *
	 *  sw $t, offset($s)
	 *  1010 11ss ssst tttt iiii iiii iiii iiii
	 *
	 *  xor $d, $s, $t
	 *  0000 00ss ssst tttt dddd d--- --10 0110
	 *  xori $t, $s, imm
	 *  0011 10ss ssst tttt iiii iiii iiii iiii
	 *
	 */
}
