/*
 * instruction.h
 *
 *  Created on: Feb 16, 2017
 *      Author: Brandon Chambers
 *      Author: Thomas Xu
 *
 *  REFERENCES: see projmain.c header comment.
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "pipeline.h"
/******************************************************************************
 * Constants/Definitions
 */
// Alternative version of opcodes.
// Defining opcodes
//#define ADD 0b100000		//ADD   => 32
//#define SUB 0b100010		//SUB   => 34
//#define AND 0b100100		//AND   => 36
//#define SLT 0b101010		//SLT   => 42
//#define OR 0b100101		    //OR    => 37
//#define LI 0b100001		    //LI    => 33
//#define SYSCALL 0b001100	//SYSCALL=>12
//#define LW 0b100011		    //LW    => 35
//#define SW 0b101011		    //SW    => 43
//#define J 0b000010			//J     => 2
//#define BEQ 0b000100		//BEQ   => 4
//#define MOVE 0b000110		//MOVE  => 6
/******************************************************************************
 * Global Vars and Structs
 */
typedef enum _bool_tag {
	false, true
} bool;

typedef enum opcode_tag {
	ADD,ADDI,ADDIU,ADDU,AND,ANDI,BEQ,BNE,J,JAL,JR,LBU,LHU,LL,LUI,LW,NOR,OR,ORI,
	SLT,SLTI,SLTIU,SLTU,SLL,SRL,SB,SC,SH,SW,MUL,MULU,SUB,SUBU,DIV,DIVU,BUBBLE,HALT
} opcode;

/*
 * Future version of opcodes
 */
//struct {
//	const char *name;
//	uint8_t addr : 6;
//} opcode[] = {
//		{"ADD", 0b100000},{"SUB", 0b100010},{"AND", 0b100100},
//		{"ADDI",0x8},{},{}
//};


//Bubble type B
typedef enum _instruction_type {
	R, I, B, JType
} instr_type;


typedef struct instruction_tag {
	instr_type type;
	opcode op;
	int8_t rs;
	int8_t rt;
	int8_t rd;
	int32_t i;
	bool isHalt;
} instr;

struct {
	const char *name;
	char *address;
} regMap[] = { { "zero", "00000" }, { "at", "00001" }, { "v0", "00010" }, {
		"v1", "00011" }, { "a0", "00100" }, { "a1", "00101" },
		{ "a2", "00110" }, { "a3", "00111" }, { "t0", "01000" },
		{ "t1", "01001" }, { "t2", "01010" }, { "t3", "01011" },
		{ "t4", "01100" }, { "t5", "01101" }, { "t6", "01110" },
		{ "t7", "01111" }, { "s0", "10000" }, { "s1", "10001" },
		{ "s2", "10010" }, { "s3", "10011" }, { "s4", "10100" },
		{ "s5", "10101" }, { "s6", "10110" }, { "s7", "10111" },
		{ "t8", "11000" }, { "t9", "11001" }, { "k0", "11010" },
		{ "k1", "11011" }, { "gp", "11100" }, { "sp", "11101" },
		{ "fp", "11110" }, { "ra", "11111" }, { "\0", 0 } };
		//last tuple represents null terminator to mark the end

instr instructions[512];
int32_t pc = 0;
int32_t haltIndex = 0;
/******************************************************************************
 * Function Prototypes
 */

void parseInstruction(char*, FILE*);
void trimInstruction(char*);
bool isAValidCharacter(char);
bool isAValidReg(char);
char* extractOpcode(char*);
int extractRegister(char*, int);
int extractImmediate(char*, int);
int extractBase(char*);
opcode stringToOpcode(char*);
bool isRType(char* opcode);
bool isIType(char* opcode);
int regValue(char*);

/******************************************************************************
 * Functions
 */

/**
 * After parsing the .asm file in the fileparser.h function, each line
 * instruction is sent here for parsing into MIPS instructions.
 */
void parseInstruction(char *instr, FILE *outFile) {

	char *ignore = "#"; //check for comments
	char *c = instr; //store copy of the instruction string
	while (*c) { //iterate through string checking each char
		if (strchr(ignore, *c))
			return; //if comment found, do not parse this line!
		c++;
	}

	trimInstruction(instr);

	printf("\t%s\n", instr);
	char* opcode = extractOpcode(instr);

	if (isRType(opcode)) {
		int rs = extractRegister(instr, 1);
		int rt = extractRegister(instr, 2);
		int rd = extractRegister(instr, 0);
		instructions[pc].type = R;
		instructions[pc].op = stringToOpcode(opcode);
		instructions[pc].rs = rs;
		instructions[pc].rt = rt;
		instructions[pc].rd = rd;
		instructions[pc].i = -1;
		instructions[pc].isHalt = false;
	} else if (isIType(opcode)) {
		int rs, imm;
		int rt = extractRegister(instr, 0);
		if (strcmp(opcode, "lw") != 0 && strcmp(opcode, "sw") != 0) {
			rs = extractRegister(instr, 1);
			imm = extractImmediate(instr, 2);
		} else { //opcode is lw or sw
			rs = extractBase(instr);
			imm = extractImmediate(instr, 1);
		}
		instructions[pc].type = I;
		instructions[pc].op = stringToOpcode(opcode);
		instructions[pc].rs = rs;
		instructions[pc].rt = rt;
		instructions[pc].rd = rt;
		instructions[pc].i = imm;
		instructions[pc].isHalt = false;
	} else if (strcmp(opcode, "halt") == 0) {
		instructions[pc].type = B;
		instructions[pc].op = HALT;
		instructions[pc].rs = -1;
		instructions[pc].rt = -1;
		instructions[pc].rd = -1;
		instructions[pc].i = -1;
		instructions[pc].isHalt = true;
		haltIndex = pc;
	} else {
		printf("\n>>>ERROR!\n******Illegal or unimplemented"
				" opcode: * %s *\n\tFrom: instruction.h @ line 168\n", opcode);
		exit(1);
	}

	pc++;
}

/**
 * remove spaces or unnecessary characters from each instruction line
 */
void trimInstruction(char* instruction) {
	int stage = 0; //How many times we've gone from not a space to a space
	int newIndex = 0;
	char temp[100];
	int i;
	bool lastCharWasSpace = true;
	for (i = 0; instruction[i] != '\0'; i++) {
		if (isAValidCharacter(instruction[i])) { //Is alphanumeric or a dollar sign or a comma or a dash or a paren
			temp[newIndex++] = instruction[i];
			lastCharWasSpace = false;
		} else { //Basically is a space
			if (!lastCharWasSpace) {
				stage++;
				if (stage == 1) {
					temp[newIndex++] = ' ';
				}
			}
			lastCharWasSpace = true;
		}
	}
	temp[newIndex++] = '\0';
	strcpy(instruction, temp);
}

/**
 * Check if the opcode is an R, I, B, or J Type instruction
 */
bool isRType(char* opcode) {
	return strcmp(opcode, "add") == 0 || strcmp(opcode, "sub") == 0
			|| strcmp(opcode, "and") == 0 || strcmp(opcode, "or") == 0
			|| strcmp(opcode, "mul") == 0;
}

/**
 * Check if the opcode is an R, I, B, or J Type instruction
 */
bool isIType(char* opcode) {
	return strcmp(opcode, "addi") == 0 || strcmp(opcode, "lw") == 0
			|| strcmp(opcode, "sw") == 0 || strcmp(opcode, "beq") == 0;
}

/**
 * built in function 'isalnum' (is alphanumeric)
 * or other various approved chars
 */
bool isAValidCharacter(char c) {
	return isalnum(c) || c == '$' || c == ',' || c == '-' || c == '('
			|| c == ')';
}

/**
 * is alphanumeric?
 */
bool isAValidReg(char c) {
	return isalnum(c) || c == '$';
}

/**
 * take the opcode from instr line and return as string
 */
char* extractOpcode(char* instr) {
	char* opcode = (char *) malloc(sizeof(char) * 15);
	int i;
	for (i = 0; instr[i] != ' '; i++) {
		opcode[i] = instr[i];
	}
	opcode[i] = '\0';
	return opcode;
}

/**
 * remove any unecessary chars, then return the register name as string
 */
int extractRegister(char* instr, int index) {
	int i;
	int regIdx = 0;
	int charIdx = 0;
	bool gotOp = false; //did we find the opcode yet?
	char reg[6]; //to hold $zero+'\0'

	for (i = 0; instr[i] != '\0'; i++) {
		if (gotOp && instr[i] == ',')
			regIdx++;
		if (gotOp && isAValidReg(instr[i]) && index == regIdx)
			reg[charIdx++] = instr[i];
		if (instr[i] == ' ')
			gotOp = true;
	}
	reg[charIdx++] = '\0'; //null terminate

	if (reg[0] != '$') {
		printf("\n>>>ERROR!\n******Invalid Register Name: * %s  *,"
				"\n\tFrom: instruction.h @ line 270\n", (char*) reg);
		exit(1);
	}
	//trim dollar sign
	int regVal = regValue(reg + 1);
	//Register is invalid: only one char
	if (regVal == -1) {
		printf("\n>>>ERROR!\n******Invalid Register Name: * %c  *,"
				"\n\tFrom: instruction.h @ line 278\n", (char) regVal);
		exit(1);
	}
	return regVal;
}

/*
 * Takes in a register without the dollar sign; s3, v0, 20, 5, zero
 * Checks for validity and returns normalized form
 */
int regValue(char* c) {

	int retVal = -1; //return -1 if not found in register map
	if (isalpha(c[0])) {
		int i = 0;
		while (strcmp(c, regMap[i].name) != 0) { // && regMap[i].name != NULL) {
			if (strcmp("\0", regMap[i].name) != 0)
				i++;
		    else
				return retVal;
		}

		if (strcmp(c, regMap[i].name) == 0)
			retVal = i;

	} else {
		/*If no letter in first index, check if it is $# format.
		 * Then check if all chars after '$' are numbers...
		 * If numbers, then convert to int to check if it's between 0 and 31
		 *  Otherwise return -1
		 */
		int i = 0;
		char currC = c[i];
		while (currC != '\0') {
			if (!isdigit(currC))
				return -1;
			currC = c[++i];
		}
		//If we've made it here, the string is a number
		int regIndex = atoi(c);
		if (regIndex >= 0 && regIndex <= 31)
			return regIndex;
		else {
			printf("\n>>>ERROR!\n******Register Index:* %d *out of bounds,"
					"\n\tFrom: projmain.h @ line 322\n", regIndex);
			exit(1);
		}
	}	//end else

	return retVal;
}	//end regValue function


/**
 * For instrs with Base registers...
 */
int extractBase(char* instr) {
	int i;
	int charIdx = 0;
	bool paren1 = false; //checking for opening parenthesis
	bool paren2 = false; //checking for closing parenthesis
	char reg[6]; //To hold $zero+'\0'
	for (i = 0; instr[i] != '\0'; i++) {

		if (instr[i] == ')')
			paren2 = true;
		if (paren1 && !paren2) {
			reg[charIdx] = instr[i];
			charIdx++;
		}
		if (instr[i] == '(')
			paren1 = true;

	}
	if (!paren2 || !paren1) {
		printf("\n>>>ERROR!\n******Invalid Parentheses,"
				"\n\tFrom: instruction.h @ line 354\n");
		exit(1);
	}
	if (reg[0] != '$') {
		printf("\n>>>ERROR!\n******Invalid Offset/Base Register (no $),"
				"\n\tFrom: instruction.h @ line 359\n");
		exit(1);
	}
	reg[charIdx++] = '\0'; //null terminate
	return regValue(reg + 1);
}

/**
 * For IType instrs, get the constant number
 */
int extractImmediate(char* instruction, int index) {
	int i;
	int regIdx = 0;
	int charIdx = 0;
	bool readOpcode = false;
	char reg[6]; //To be able to hold $zero, which is 5 characters and the null character
//	printf("extractImmediate\n");
	for (i = 0; instruction[i] != '\0' && instruction[i] != '('; i++) {
		if (readOpcode && instruction[i] == ',')
			regIdx++;
		if (readOpcode && instruction[i] != ',' && regIdx == index)
			reg[charIdx++] = instruction[i];
		if (instruction[i] == ' ')
			readOpcode = true;
	}
	reg[charIdx++] = '\0';
	for (i = 0; reg[i] != '\0'; i++) {
		if (!isdigit(reg[i])) {
			if (!(i == 0 && reg[i] == '-')) {
				printf("\n>>>ERROR!\n******Invalid Immediate Field,"
						"\n\tFrom: instruction.h @ line 389\n");
				exit(1);
			}
		}
	}
	int imm = atoi(reg);
	if (imm > 32767 || imm < -32768) {
		printf("\n>>>ERROR!\n******Invalid Immediate Field: Too Large at size="
				"%d\n\tFrom: instruction.h @ line 397\n", imm);
		exit(1);
	}
	return imm;
}

/**
 * If valid op, assign the Enum value...
 * TODO: Switch/Case instead
 */
opcode stringToOpcode(char* opcode) {
	/*add types*/
	if (strcmp(opcode, "add") == 0)
		return ADD;
	else if (strcmp(opcode, "addi") == 0)
		return ADDI;
	else if (strcmp(opcode, "addiu") == 0)
		return ADDIU;
	else if (strcmp(opcode, "addu") == 0)
		return ADDU;
	/*and types*/
	else if (strcmp(opcode, "and") == 0)
		return AND;
	else if (strcmp(opcode, "andi") == 0)
		return ANDI;
	/*branching and jumping types*/
	else if (strcmp(opcode, "beq") == 0)
		return BEQ;
	else if (strcmp(opcode, "bne") == 0)
		return BNE;
	else if (strcmp(opcode, "j") == 0)
		return J;
	else if (strcmp(opcode, "jal") == 0)
		return JAL;
	else if (strcmp(opcode, "jr") == 0)
		return JR;
	/*load types*/
	else if (strcmp(opcode, "lbu") == 0)
		return LBU;
	else if (strcmp(opcode, "lhu") == 0)
		return LHU;
	else if (strcmp(opcode, "ll") == 0)
		return LL;
	else if (strcmp(opcode, "lui") == 0)
		return LUI;
	else if (strcmp(opcode, "lw") == 0)
		return LW;
	/*or type*/
	else if (strcmp(opcode, "nor") == 0)
		return NOR;
	else if (strcmp(opcode, "or") == 0)
		return OR;
	else if (strcmp(opcode, "ori") == 0)
		return ORI;
	/*set less than types*/
	else if (strcmp(opcode, "slt") == 0)
		return SLT;
	else if (strcmp(opcode, "slti") == 0)
		return SLTI;
	else if (strcmp(opcode, "sltiu") == 0)
		return SLTIU;
	else if (strcmp(opcode, "sltu") == 0)
		return SLTU;
	/*shift types*/
	else if (strcmp(opcode, "sll") == 0)
		return SLL;
	else if (strcmp(opcode, "srl") == 0)
		return SRL;
	/*store types*/
	else if (strcmp(opcode, "sb") == 0)
		return SB;
	else if (strcmp(opcode, "sc") == 0)
		return SC;
	else if (strcmp(opcode, "sh") == 0)
		return SH;
	else if (strcmp(opcode, "sw") == 0)
		return SW;
	/*arithmetic types*/
	else if (strcmp(opcode, "mul") == 0)
		return MUL;
	else if (strcmp(opcode, "mulu") == 0)
		return MULU;
	else if (strcmp(opcode, "sub") == 0)
		return SUB;
	else if (strcmp(opcode, "subu") == 0)
		return SUBU;
	else if (strcmp(opcode, "div") == 0)
		return DIV;
	else if (strcmp(opcode, "divu") == 0)
		return DIVU;
	/*custom types*/
	else if (strcmp(opcode, "bubble") == 0)
		return BUBBLE;
	else if (strcmp(opcode, "halt") == 0)
		return HALT;
	else
		return HALT;
} //end function stringToOpcode()

#endif /* INSTRUCTION_H_ */
