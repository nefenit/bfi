/* bfi.c - Extended Brainfuck interpreter
 * (c) Copyright 2019 Bartosz Mierzynski
 * Written in ANSI C (C89)
 */

#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
  #if __STDC_VERSION__ >= 199901L
    #include <stdbool.h>
  #else
    #ifndef __bool_true_false_are_defined
      #define bool                           int
      #define true                           1 /* non-zero */
      #define false                          0
      #define __bool_true_false_are_defined  1 
    #endif /*__bool_true_false_are_defined */
  #endif /* __STDC_VERSION__ >= 199901L */
#endif /* __cplusplus */

#define PROGRAM_NAME         "bfi"
#define PROGRAM_VERSION      "1.0"
#define COPYRIGHT_YEAR       "2019"
#define COPYRIGHT_HOLDER     "Bartosz Mierzynski"
#define LICENSE_ABBREVIATION ""
#define LICENSE_LINE         ""

enum {DEFAULT_TAPE_SIZE = 30000};

typedef enum {
	EXTENDED_TYPE_0 = 0,
	EXTENDED_TYPE_1 = 1<<1,
	EXTENDED_TYPE_2 = 1<<2,
	EXTENDED_TYPE_3 = 1<<3
} extended_t;

void usage(int status) {
	printf(
	"usage: %s [-x|-X] [-m SIZE] [-i CODE]... FILE...\n"
	"       %s --help\n"
	"       %s --version\n"
	"Extended Brainfuck interpreter\n%s%s%s",
	PROGRAM_NAME,
	PROGRAM_NAME,
	PROGRAM_NAME,
	"Options:\n"
	"  -m, --memory=SIZE\n"
	"  -i, --interpret=CODE\n"
	"  -x, --extended\n"
	"  -X, --extended-2\n"
	"  -h, --help\n"
	"  -v, --version\n"
	,
	"Basic Commands:\n"
	"  >  increments data pointer\n"
	"     (to point to the next cell to he right)\n"
	"  <  decrement the pointer\n"
	"     (to point to the next cell to the left)\n"
	"  +  increment the byte at the data pointer\n"
	"  -  decrement the byte at the data pointer\n"
	,
	"  .  output byte at the data pointer\n"
	"  ,  accept one byte of input, storing its value at the data pointer\n"
	"  [  if the byte at the pointer is zero\n"
	"     jump forward to the command after the corresponding ]\n"
	"  ]  if the byte at the pointer is non-zero\n"
	"     jump back to the command after the corresponding [ \n"
	);
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

bool isinstruction(extended_t extended, char c) {
	char   opcodes[] = "><+-.,[]@$!}{~^&|?)(*/=_%XxMmLl:0123456789ABCDEF#";
	size_t opcodes_n = 8;
	int    i;

	if(extended & EXTENDED_TYPE_1)
		opcodes_n += 9;
	if(extended & EXTENDED_TYPE_2)
		opcodes_n += 8;
	if(extended & EXTENDED_TYPE_3)
		opcodes_n += 24;

	for(i = 0; i < opcodes_n; ++i)
		if(c == opcodes[i])
			return true;

	return false;
}

char* readcode(bool extended, char *filename, size_t *code_size) {
	FILE *fp;
	char *code;
	int   c, i;

	if((fp = fopen(filename, "rb")) == NULL){
		perror(filename);
		return NULL;
	}

	if(extended & EXTENDED_TYPE_2){
		fseek(fp, 0l, SEEK_END);
		*code_size = ftell(fp);
	} else {
		*code_size = 0;
		while((c = getc(fp)) != EOF)
			if(isinstruction(extended, c))
				++*code_size;
	}
	
	code = (char*)malloc(sizeof(char) * *code_size);

	rewind(fp);

	i = 0;

	if(extended & EXTENDED_TYPE_2)
		while((c = getc(fp)) != EOF)
				code[i++] = (char)c;
	else
		while((c = getc(fp)) != EOF)
			if(isinstruction(extended, c))
				code[i++] = (char)c;

	fclose(fp);

	return code;
}

int interpret(extended_t extended, char *code, size_t code_size, size_t tape_size) {
	uint8_t *tape, storage;
	uint16_t tmp;
	int      dp, ip;

	if(!(tape = (uint8_t*)calloc(tape_size, sizeof(uint8_t)))){
		perror("calloc");
		exit(EXIT_FAILURE);	
	}

	/* Memory Layout */
	if(extended & EXTENDED_TYPE_2) {
		/* Extended Type 2 onwards
		 *
		 * tape:
		 * 0         ip      dp
		 * +-----------------------+
		 * | storage | code | data |
		 * +-----------------------+
		 * */
		memcpy   (tape + 1, code, code_size);
		*tape   = 0;
		ip      = 1;
		{
			int i;
			
			for(dp = 0, i = code_size; i > 0; --i) {
				if(tape[i] == '@')
					dp = i+1;
			}
			if(dp == 0)
				dp = code_size+1;
		}
	} else {
		/* Default
		 *
		 * storage:
		 * +---------+
		 * | storage |
		 * +---------+
		 *
		 * code:
		 * ip
		 * +------+
		 * | code |
		 * +------+
		 * 
		 * tape:
		 * dp
		 * +------+
		 * | tape |
		 * +------+
		 * */
		ip = dp = storage = 0;
	}

	for(; ip < (extended & EXTENDED_TYPE_2 ? tape_size : code_size); ++ip) {
		switch((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip])) {
		case '\0':
			goto exit_interpret;
			break;
		case '>':
			++dp;
			if(dp == tape_size)
				dp = 0;
			break;
		case '<':
			--dp;
			if(dp == -1)
				dp = tape_size - 1;
			break;
		case '+':
			if(tape[dp] == 255)
				tape[dp] = 0;
			else
				++tape[dp];
			break;
		case '-':
			if(tape[dp] == 0)
				tape[dp] = 255;
			else
				--tape[dp];
			break;
		case '.':
			putchar((int)tape[dp]);
			break;
		case ',':
			{
				int c = getchar();
				
				tape[dp] = (uint8_t)(c == EOF ? 0 : c);
			}
			break;
		case '[':
			{
				int openbrackets;
				
				if(tape[dp] == 0) {
					openbrackets = 0;
					for(++ip; ip < (extended & EXTENDED_TYPE_2 ? tape_size : code_size); ++ip) {
						if(code[ip] == ']' && openbrackets == 0)
							break;
						else if((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip]) == '[')
							++openbrackets;
						else if((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip]) == ']')
							--openbrackets;
					}
				}
			}
			break;
		case ']':
			{
				int closedbrackets;
				
				if(tape[dp]) {
					closedbrackets = 0;
					for(--ip; ip >= 0; --ip) {
						if((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip]) == '[' && closedbrackets == 0)
							break;
						else if((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip]) == ']')
							++closedbrackets;
						else if((extended & EXTENDED_TYPE_2 ? tape[ip] : code[ip]) == '[')
							--closedbrackets;
					}
				}
			}
			break;
		case '@':
			if(extended & EXTENDED_TYPE_1)
				goto exit_interpret;
			break;
		case '$':
			if(extended & EXTENDED_TYPE_1) {
				if(extended & EXTENDED_TYPE_2)
					*tape   = tape[dp];
				else
					storage = tape[dp];
			}
			break;
		case '!':
			if(extended & EXTENDED_TYPE_1) {
				if(extended & EXTENDED_TYPE_2)
					tape[dp] = *tape;
				else
					tape[dp] = storage;
			}
			break;
		case '}':
			if(extended & EXTENDED_TYPE_1)
				tape[dp] >>= 1;
			break;
		case '{':
			if(extended & EXTENDED_TYPE_1)
				tape[dp] <<= 1;
			break;
		case '~':
			if(extended & EXTENDED_TYPE_1)
				tape[dp] = ~tape[dp];
			break;
		case '^':
			if(extended & EXTENDED_TYPE_1) {
				if(extended & EXTENDED_TYPE_2)
					tape[dp] ^= *tape;
				else
					tape[dp] ^= storage;
			}
			break;
		case '&':
			if(extended & EXTENDED_TYPE_1) {
				if(extended & EXTENDED_TYPE_2)
					tape[dp] &= *tape;
				else
					tape[dp] &= storage;
			}
			break;
		case '|':
			if(extended & EXTENDED_TYPE_1) {
				if(extended & EXTENDED_TYPE_2)
					tape[dp] |= *tape;
				else 
					tape[dp] |= storage;
			}
			break;
		case '?':
			if(extended & EXTENDED_TYPE_2)
				ip = dp;
			break;
		case ')':
			{
				int i;

				if(extended & EXTENDED_TYPE_2) {
					if(!(tape = (uint8_t*)realloc(tape, sizeof(uint8_t) * (tape_size+1)))){
						perror("realloc");
						exit(EXIT_FAILURE);
					}
					++tape_size;
					for(i = tape_size; i > 0; --i){
						if(i == dp) {
							tape[i] = tape[i-1];
							tape[i] = (uint8_t)0;	
						}
						if(i > dp)
							tape[i] = tape[i-1];
					}
				}
			}
			break;
		case '(':
			{
				int i;

				if(extended & EXTENDED_TYPE_2) {
					for(i = dp; i < tape_size-2; ++i){
							tape[i] = tape[i+1];
					}
					if(!(tape = (uint8_t*)realloc(tape, sizeof(uint8_t) * (tape_size-1)))){
						perror("realloc");
						exit(EXIT_FAILURE);
					}
					--tape_size;
				}
			}
			break;
		case '*':
			if(extended & EXTENDED_TYPE_2) {
				tmp = tape[dp] * *tape;
				tape[dp] = tmp & 0xFF;
			}
			break;
		case '/':
			if(extended & EXTENDED_TYPE_2) {
				if(*tape) {
					tmp = tape[dp] / *tape;
					tape[dp] = tmp & 0xFF;
				} else {
					fprintf(stderr, "%s: Division by 0\n", PROGRAM_NAME);
					exit(EXIT_FAILURE);
				}
			}
			break;
		case '=':
			if(extended & EXTENDED_TYPE_2) {
				tmp = tape[dp] + *tape;
				tape[dp] = tmp & 0xFF;
			}
			break;
		case '_':
			if(extended & EXTENDED_TYPE_2) {
				tmp = tape[dp] - *tape;
				tape[dp] = tmp & 0xFF;
			}
			break;
		case '%':
			if(extended & EXTENDED_TYPE_2) {
				tmp = tape[dp] % *tape;
				tape[dp] = tmp & 0xFF;
			}
			break;
		}
	}

exit_interpret:

	free(tape);
	free(code);

	return 0;
}

int main(int argc, char *argv[]) {
	int c;
	extended_t extended            = EXTENDED_TYPE_0;
	char *code                     = (char*)NULL;
	size_t code_size, tape_size    = 0;
	const char optstring[]         = "i:m:xXhv";
	const struct option longopts[] = {
	{"interpret",  required_argument, NULL, 'i'},
	{"memory",     required_argument, NULL, 'm'},
	{"extended",   no_argument,       NULL, 'x'},
	{"extended-2", no_argument,       NULL, 'X'},
	{"help",       no_argument,       NULL, 'h'},
	{"version",    no_argument,       NULL, 'v'},
	{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
		switch (c) {
		case 'i':
			interpret(extended, optarg, strlen(optarg), tape_size ? tape_size : DEFAULT_TAPE_SIZE);
			break;
		case 'm':
			tape_size = strtoul(optarg, NULL, 0);
			break;
		case 'x':
			extended = EXTENDED_TYPE_1;
			break;
		case 'X':
			extended = EXTENDED_TYPE_1|EXTENDED_TYPE_2;
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
		if(!(code = readcode(extended, *argv, &code_size)))
			return EXIT_FAILURE;
		interpret(extended, code, code_size, tape_size ? tape_size : DEFAULT_TAPE_SIZE);
	}
	return EXIT_SUCCESS;
}
