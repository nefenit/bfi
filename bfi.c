#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {DEFAULT_TAPE_SIZE = 30000};

char* readcode(char *filename, size_t *code_size) {
	FILE *fp;
	char *code;
	int c, i;
	
	if(!(fp = fopen(filename, "rb"))){
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	*code_size = 0;

	while((c = getc(fp)) != EOF) {
		switch(c) {
		case '>':
		case '<':
		case '+':
		case '-':
		case '.':
		case ',':
		case '[':
		case ']':
			++*code_size;
			break;
		}	
	}

	code = (char*)malloc(sizeof(char) * *code_size);

	fseek(fp, 0l, SEEK_SET);

	i = 0;

	while((c = getc(fp)) != EOF) {
		switch(c) {
		case '>':
		case '<':
		case '+':
		case '-':
		case '.':
		case ',':
		case '[':
		case ']':
			code[i++] = (char)c;
		}	
	}

	fclose(fp);

	return code;
}

int interpret(char *code, size_t code_size, size_t tape_size) {
	uint8_t *tape;
	int p, ip;

	if(!(tape = (uint8_t*)calloc(tape_size, sizeof(uint8_t)))){
		perror("calloc");
		exit(EXIT_FAILURE);	
	}

	for(ip = p = 0; ip < code_size; ++ip) {
		switch(code[ip]) {
		case '>':
			++p;
			if(p == tape_size)
				p = 0;
			break;
		case '<':
			--p;
			if(p == -1)
				p = tape_size - 1;
			break;
		case '+':
			if(tape[p] == 255)
				tape[p] = 0;
			else
				++tape[p];
			break;
		case '-':
			if(tape[p] == 0)
				tape[p] = 255;
			else
				--tape[p];
			break;
		case '.':
			putchar((int)tape[p]);
			break;
		case ',':
			tape[p] = (uint8_t)getchar();
			break;
		case '[':
			{
				int openbrackets;
				
				if(tape[p] == 0) {
					openbrackets = 0;
					++ip;
					for(;ip < code_size; ++ip) {
						if(code[ip] == ']' && openbrackets == 0)
							break;
						else if(code[ip] == '[')
							++openbrackets;
						else if(code[ip] == ']')
							--openbrackets;
					}
				}
			}
			break;
		case ']':
			{
				int closedbrackets;
				
				if(tape[p] != 0) {
					closedbrackets = 0;
					--ip;
					for(;ip >= 0; --ip) {
						if(code[ip] == '[' && closedbrackets == 0)
							break;
						else if(code[ip] == ']')
							++closedbrackets;
						else if(code[ip] == '[')
							--closedbrackets;
					}
				}
			}
			break;
		}
	}

	free(tape);

	return 0;
}

int main(int argc, char *argv[]) {
	char   *code;
	size_t  code_size;

	if(!(code = readcode(argv[1], &code_size)))
		return EXIT_FAILURE;

	interpret(code, code_size, DEFAULT_TAPE_SIZE);

	free(code);

	return EXIT_SUCCESS;
}

