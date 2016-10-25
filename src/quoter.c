/* vim:set noet cinoptions= sw=4 ts=4:
This file is part of the quoter project and distributed under the MIT license.

Copyright (c) Martin Väth <martin@mvath.de>

Purpose: Quote arguments or standard input for usage in POSIX shell by eval

Installation: Just compile this single file and copy _quoter to zsh's $fpath
If the compiler does not understand the specials __builtin_expect or
__attribute__ ((noreturn)) it should help to pass the respective compiler
options -DAVOID_BUILTIN_EXPECT or -DAVOID_ATTRIBUTE_NORETURN
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AVOID_BUILTIN_EXPECT
#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#ifndef AVOID_ATTRIBUTE_NORETURN
#define ATTRIBUTE_NORETURN __attribute__ ((noreturn))
#else
#define ATTRIBUTE_NORETURN
#endif

#ifndef BOOL
typedef char BOOL;
#endif
typedef signed char TINY;

void version_and_exit() ATTRIBUTE_NORETURN;
void help_and_exit() ATTRIBUTE_NORETURN;
void stdin_eval();
void quoter_eval(char *source);
void delimiter();
void parse_opt(int argc, char **argv);
void init_static();
void safe_fputc(int c);
void safe_fputs(const char *s);
void safe_fwrite(const char *s, const char *e);
void write_error() ATTRIBUTE_NORETURN;
void close_file();
void *safe_malloc(size_t s);
void *safe_realloc(void *p, size_t s);
void out_of_memory() ATTRIBUTE_NORETURN;
void free_and_exit(int i) ATTRIBUTE_NORETURN;

/* This is a simple script: Most variables are just global */

static TINY *badtype = NULL;
static char *buffer = NULL;
static FILE *outputfh;
static FILE *openfile = NULL;
static BOOL had_output = 0;

/* Option related variables */

static char **args = NULL;
static char *outputname = NULL;
static char *openmode;
static TINY opt_short = 0;
static BOOL opt_stdin = 0;
static BOOL opt_newline = 0;
static BOOL opt_cut = 0;

/* Save space by storing strings only once */

static char *o_para = "w";
static char *a_para = "a";
static char *empty = "";

void version_and_exit() {
	puts("quoter v1.0");
	free_and_exit(EXIT_SUCCESS);
}

void help_and_exit() {
	puts("Usage: quoter [options] [--] [args]\n"
"Output args quoted appropriate for POSIX shell by eval\n"
"Options:\n"
"-o FILE or --output=FILE send output to FILE instead of stdout\n"
"-a FILE or --append=FILE as -o but open FILE in append mode\n"
"-i or --stdin            append standard input (split at \\0 symbols) to args\n"
"-n or --newline          output newline instead of a space as arg separator\n"
"-s or --short            output the shortest string as possible\n"
"-S or --unshort          output readable and compatible length (default)\n"
"-l or --long             output paranoically long/compatible\n"
"-c or --cut              omit trailing newline in output\n"
"-V or --version          print version\n"
"-h or --help             show this help text\n");
	free_and_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	args = safe_malloc(argc * sizeof(char **));
	parse_opt(argc, argv);
	if(unlikely(unlikely(outputname != NULL) && likely(*outputname != '\0'))) {
		if(!(openfile = fopen(outputname, openmode))) {
			fprintf(stderr, "quoter: cannot open file for writing: %s\n", outputname);
			free_and_exit(EXIT_FAILURE);
		}
		outputfh = openfile;
	} else {
		outputname = NULL;
		outputfh = stdout;
	}
	init_static();
	char **curr;
	for(curr = args; likely(*curr != NULL); ++curr) {
		quoter_eval(*curr);
	}
	if(opt_stdin) {
		stdin_eval();
	}
	if(likely(!opt_cut)) {
		safe_fputc('\n');
	}
	free_and_exit(EXIT_SUCCESS);
}

void stdin_eval() {
	size_t currsize = 0x1000;
	buffer = safe_malloc(currsize);
	size_t read_offset = 0;
	BOOL force_empty = 0;
	for(;;) {
		size_t len = read_offset + fread(buffer + read_offset, 1, currsize - read_offset, stdin);
		if(unlikely(len < currsize)) {
			currsize = len;
			break;
		}
		force_empty = 0;
		size_t curr = 0;
		size_t zero = read_offset + strnlen(buffer + read_offset, currsize - read_offset);
		while(likely(zero != currsize)) {
			quoter_eval(buffer + curr);
			curr = zero + 1;
			if(unlikely(curr == currsize)) {  /* buffer ends with \0 */
				force_empty = 1;
				break;
			}
			zero = curr + strnlen(buffer + curr, currsize - curr);
		}
		if(unlikely(force_empty)) {
			read_offset = 0;
		} else if(unlikely(curr > 0)) {  /* at least one \0 found */
			read_offset = currsize - curr;
			memmove(buffer, buffer + curr, read_offset);
		} else {  /* buffer was not large enough */
			read_offset = currsize;
			currsize += 0x1000;
			buffer = safe_realloc(buffer, currsize);
		}
	}
	if(unlikely(currsize == 0)) {
		if(unlikely(force_empty)) {
			quoter_eval(empty);
		}
		return;
	}
	buffer[currsize] = 0;
	quoter_eval(buffer);
	size_t curr = read_offset + strlen(buffer + read_offset) + 1;
	while(likely(curr != currsize + 1)) {
		quoter_eval(buffer + curr);
		curr += strlen(buffer + curr) + 1;
	}
}

void quoter_eval(char *source) {
	char *start;
	for(start = source; ; ) {
		char *escape = NULL;
		BOOL mustframe = 0;
		char *curr;
		for(curr = start; ; ++curr) {
			TINY bad = badtype[(unsigned char)(*curr)];
			if(likely(bad == 0)) {  /* most frequent case first */
				continue;
			}
			if(opt_short < 0) {
				if(likely((bad >= 0) && (bad != 4))) {
					mustframe = 1;
					continue;
				}
			} else {
				if(likely(bad == 1) ||
					(likely(curr != source) && (bad == 2))) {
					continue;
				}
				if(likely(bad == 2) || likely(bad == 3)) {
					if(likely(opt_short == 0) || likely(escape != NULL)) {
						mustframe = 1;
					} else {
						escape = curr;
					}
					continue;
				}
				if(likely(bad == 5)) {
					mustframe = 1;
					continue;
				}
			}
			/* end or single quote: output previous part */
			if(unlikely(start == source)) {
				delimiter();
			}
			if(unlikely(mustframe)) {
				safe_fputc('\'');
				safe_fwrite(start, curr);
				safe_fputc('\'');
			} else if(unlikely(escape != NULL)) {
				safe_fwrite(start, escape);
				safe_fputc('\\');
				safe_fwrite(escape, curr);
			} else {
				safe_fwrite(start, curr);
			}
			if(bad < 0) {
				if(unlikely(curr == source)) {
					safe_fputs("''");
				}
				return;
			}
			safe_fputs("\\\'");
			start = curr + 1;
			break;
		}
	}
}

void delimiter() {
	if(likely(had_output)) {
		if(unlikely(opt_newline)) {
			safe_fputs(" \\\n");
		} else {
			safe_fputc(' ');
		}
	} else {
		had_output = 1;
	}
}

#define LONGOPT(name, size, action) if(!strncmp(curr, name, size)) { \
	action; \
	continue; \
}

#define LONGOPT_ARG(name, size, arg, action) \
if(!strncmp(curr, name, size)) { \
	action; \
	if(likely(curr[size] == '=')) { \
		arg = curr + (size + 1); \
	} else if(i == argc) { \
		arg = NULL; \
	} else { \
		arg = argv[++i]; \
	} \
	continue; \
}

#define SHORTOPT_ARG(arg) \
	arg = ++curr; \
	if(likely(*curr == '\0') && likely(i < argc)) { \
		arg = argv[++i]; \
	} \
	curr = NULL


void parse_opt(int argc, char **argv) {
	BOOL getopt = 1;
	char **coll = args;
	int i;
	for(i = 1; i < argc; ++i) {
		char *curr = argv[i];
		if(unlikely(getopt) && unlikely(*curr == '-')) {
			++curr;
			if(*curr == '-') {
				++curr;
				if(*curr == '\0') {
					getopt = 0;
					continue;
				}
				LONGOPT_ARG("output", 6, outputname, openmode = o_para)
				LONGOPT_ARG("append", 6, outputname, openmode = a_para)
				LONGOPT("short", 5, opt_short = 0)
				LONGOPT("unshort", 7, opt_short = 1)
				LONGOPT("long", 4, opt_short = -1)
				LONGOPT("stdin", 5, opt_stdin = 1)
				LONGOPT("newline", 5, opt_newline = 1)
				LONGOPT("cut", 3, opt_cut = 1)
				LONGOPT("version", 7, version_and_exit())
				LONGOPT("help", 4, help_and_exit())
				fprintf(stderr, "quoter: unknown option %s\n", argv[i]);
				free_and_exit(EXIT_FAILURE);
				continue;
			}
			for(; likely(*curr != '\0'); ++curr) {
				switch(*curr) {
					case 'o':
						openmode = o_para;
						SHORTOPT_ARG(outputname);
						break;
					case 'a':
						openmode = a_para;
						SHORTOPT_ARG(outputname);
						break;
					case 'i':
						opt_stdin = 1;
						break;
					case 'n':
						opt_newline = 1;
						break;
					case 'c':
						opt_cut = 1;
						break;
					case 'S':
						opt_short = 0;
						break;
					case 'l':
						opt_short = -1;
						break;
					case 's':
						opt_short = 1;
						break;
					case 'V':
						version_and_exit();
						break;
					case 'h':
					case 'H':
					case '?':
						help_and_exit();
						break;
					default:
						fprintf(stderr, "quoter: unknown option -%c\n", *curr);
						free_and_exit(EXIT_FAILURE);
				}
				if(curr == NULL) {
					break;
				}
			}
		} else {
			*(coll++) = curr;
		}
	}
	*coll = 0;
}

void init_static() {
	badtype = safe_malloc(256 * sizeof(TINY));
	int i;
	for(i = 0; i <= 255; ++i) {
		if(((i >= 'A') && (i <= 'Z')) ||
			((i >= 'a') && (i <= 'z')) ||
			((i >= '0') && (i <= '9'))) {
			badtype[i] = 0;
			continue;
		}
		switch(i) {
			case '_':
				badtype[i] = 0;  /* OK */
				break;
			case '-':
			case '+':
			case '/':
			case ':':
			case '.':
			case '%':
			case ',':
			case '@':
				badtype[i] = 1;  /* almost OK */
				break;
			case '~':
			case '=':
				badtype[i] = 2;  /* bad at start; escapable */
				break;
			case ' ':
			case '?':
			case '*':
			case '"':
			case '`':
			case '#':
			case ';':
			case '<':
			case '>':
			case '|':
			case '^':
			case '\\':
			case '&':
			case '$':
			case '{':
			case '}':
			case '(':
			case ')':
			case '[':
			case ']':
				badtype[i] = 3;  /* escapable */
				break;
			case '\'':
				badtype[i] = 4;  /* separator of '...' framing */
				break;
			case '\0':
				badtype[i] = -1;  /* end of string */
				break;
			default:
				badtype[i] = 5;  /* requires '...' framing */
		}
	}
}

void safe_fputc(int c) {
	if(likely(fputc(c, outputfh)) == c) {
		return;
	}
	write_error();
}

void safe_fputs(const char *s) {
	if(likely(fputs(s, outputfh)) >= 0) {
		return;
	}
	write_error();
}

void safe_fwrite(const char *s, const char *e) {
	if(unlikely(s == e)) {
		return;
	}
	size_t count = e - s;
	if(likely(fwrite(s, 1, count, outputfh)) == count) {
		return;
	}
	write_error();
}

void write_error() {
	if(outputname != NULL) {
		fprintf(stderr, "quoter: write error on %s\n", outputname);
	} else {
		fputs("quoter: write error to stdout\n", stderr);
	}
	free_and_exit(EXIT_FAILURE);
}

void close_file() {
	if(likely(openfile == NULL)) {
		return;
	}
	if(likely(fclose(openfile) == 0)) {
		openfile = NULL;
		return;
	}
	openfile = NULL;
	fprintf(stderr, "quoter: failure when closing %s\n", outputname);
	free_and_exit(EXIT_FAILURE);
}

void *safe_malloc(size_t s) {
	void *p;
	if(likely((p = malloc(s)) != NULL)) {
		return p;
	}
	out_of_memory();
}

void *safe_realloc(void *p, size_t s) {
	if(likely((p = realloc(p, s)) != NULL)) {
		return p;
	}
	out_of_memory();
}

void out_of_memory() {
	fputs("quoter: out of memory\n", stderr);
	free_and_exit(EXIT_FAILURE);
}

void free_and_exit(int i) {
	close_file();
	if(likely(args != NULL)) {
		free(args);
	}
	if(likely(badtype != NULL)) {
		free(badtype);
	}
	if(buffer != NULL) {
		free(buffer);
	}
	exit(i);
}