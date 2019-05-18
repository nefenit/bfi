/* bfi.c - Brainfuck interpreter
 * (c) Copyright 2019 Bartosz Mierzynski
 * Written in ANSI C (C89)
 */

#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME         "bfi"
#define PROGRAM_VERSION      "1.0"
#define COPYRIGHT_YEAR       "2019"
#define COPYRIGHT_HOLDER     "Bartosz Mierzynski"
#define LICENSE_ABBREVIATION ""
#define LICENSE_LINE         ""

enum {DEFAULT_TAPE_SIZE = 30000};

void usage(int status) {
	printf(
	"usage: %s [-m SIZE] [-i CODE]... FILE...\n"
	"       %s --help\n"
	"       %s --version\n"
	"Brainfuck interpreter\n"
	"Options:\n"
	"  -m, --memory=SIZE\n"
	"  -i, --interpret=CODE\n"
	"  -h, --help\n"
	"  -v, --version\n",
	PROGRAM_NAME,
	PROGRAM_NAME,
	PROGRAM_NAME);
	exit(status);
}

void version(const char* program_name, const char* program_version) {
	printf("%s %s\n"
	"Copyright (C) %s %s\n"
	"License %s: %s\n"
	"This is free software: you are free to change and redistribute it.\n"
	"There is NO WARRANTY, to the extent permitted by law.\n",
	program_name,
	program_version,
	COPYRIGHT_YEAR,
	COPYRIGHT_HOLDER,
	LICENSE_ABBREVIATION,
	LICENSE_LINE);
	exit(EXIT_SUCCESS);
}

char* readcode(char *filename, size_t *code_size) {
	FILE *fp;
	char *code;
	int c, i;

	if((fp = fopen(filename, "rb")) == NULL){
		perror(filename);
		return NULL;
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
			{
				int c = getchar();
				
				tape[p] = (uint8_t)(c == EOF ? 0 : c);
			}
			break;
		case '[':
			{
				int openbrackets;
				
				if(tape[p] == 0) {
					openbrackets = 0;
					for(++ip; ip < code_size; ++ip) {
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
					for(--ip; ip >= 0; --ip) {
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
	int c;
	char *code = NULL;
	size_t code_size, tape_size = 0;
	const struct option longopts[] = {
	{"interpret",  required_argument, NULL, 'i'},
	{"memory",     required_argument, NULL, 'm'},
	{"help",       no_argument,       NULL, 'h'},
	{"version",    no_argument,       NULL, 'v'},
	{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "i:m:hv", longopts, NULL)) != -1) {
		switch (c) {
		case 'i':
			interpret(optarg, strlen(optarg), tape_size ? tape_size : DEFAULT_TAPE_SIZE);
			break;
		case 'm':
			tape_size = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case 'v':
			version(PROGRAM_NAME, PROGRAM_VERSION);
			break;
		case '?':
		default:
			usage(EXIT_FAILURE);
		}
	}
	
	argc -= optind;
	argv += optind;

	for(;argc; --argc, ++argv){	
		if(!(code = readcode(*argv, &code_size)))
			return EXIT_FAILURE;
		interpret(code, code_size, tape_size ? tape_size : DEFAULT_TAPE_SIZE);
	}

	free(code);

	return EXIT_SUCCESS;
}

