/*
 * fileparser.h
 *
 *  Created on: Feb 16, 2017
 *      Author: Brandon Chambers
 *      Author: Thomas Xu
 *
 *  REFERENCES: see projmain.c header comment.
 */

#ifndef FILEPARSER_H_
#define FILEPARSER_H_

/******************************************************************************
 * Constants/Definitions
 */

/******************************************************************************
 * Global Vars and Structs
 */

/******************************************************************************
 * Function Prototypes
 */
void parseASMFile(char*, char*);

/******************************************************************************
 * Functions
 */

/*
 * Take in an assembly file (.asm) with MIPS instructions, parse each line
 * of text into an array and send it to the 'parseInstruction' function found
 * in 'instruction.h' header file.
 */
void parseASMFile(char *inFile, char *outFile) {

	char instrStr[100];//instruction string/lines

	FILE *fptr = fopen(inFile, "r");
	if (fptr == NULL) {
		printf("Input file '%s' could not be opened.", inFile);
		exit(1);
	}
	FILE *fptrOUT = fopen(outFile, "w");
	if (fptrOUT == NULL) {
			printf("Output file '%s' could not be opened.", outFile);
			exit(1);
	}

	printf("Instructions found:\n");
	while (fgets(instrStr, 100, fptr))
		parseInstruction(instrStr, fptrOUT);

	fclose(fptr);
}

#endif /* FILEPARSER_H_ */
