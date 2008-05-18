#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "config_parser.h"

#define MAX_OPT_LEN	50
#define MAX_PARAM_LEN	1000

char* shorten(char *str) {
		char *tmp;
		int tmp_pos;
		char c;
		int line_pos = 0, i;
		int len = strlen(str);
		const char *delim = ";;";

		tmp = calloc(MAX_PARAM_LEN + 1, 1);

		if(!tmp)
			return NULL;

		while (isspace(str[line_pos])) {
			++line_pos;
		}
		tmp_pos = 0;
		while(line_pos < len) {
			/* case 1: quoted strings */
			if(tmp_pos != 0) {
				for(i = 0; i < strlen(delim); ++i)
					tmp[tmp_pos++] = delim[i];
			}
			if (str[line_pos] == '"' || str[line_pos] == '\'') {
				c = str[line_pos];
				++line_pos;  /* skip quote */
				for (; str[line_pos] != c; /* NOTHING */) {
						tmp[tmp_pos++] = str[line_pos++];
				}
				line_pos++;	/* skip the closing " or ' */
			} else {
				for(; isprint(str[line_pos]) && !isspace(str[line_pos]); /* NOTHING */) {
					tmp[tmp_pos++] = str[line_pos++];
				}
			}
			while (isspace(str[line_pos])) {
				++line_pos;
			}
		}
		return tmp;
}


int parse_config_file(const char *filename) {
	FILE *fp;
	char *line;
	char opt[MAX_OPT_LEN + 1];
	char param[MAX_PARAM_LEN + 1];
	char erbuf[100];
	char c;		/* for the "" and '' check */
	int line_num = 0;
	int line_pos;	/* line pos */
	int opt_pos;	/* opt pos */
	int param_pos;	/* param pos */
	int parse_error = 0;
	int opt_good = 0;
	int param_good = 0;
	struct stat fs;
	int ret = 0;

	if(stat(filename, &fs) == -1)  {
		printf("Error: Unable to determine file size of %s", filename);
		return -1;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		perror("fopen");
		return -1;
	}

	if ((line = malloc(fs.st_size + 1)) == NULL) {
		printf("Can't get memory for 'line': %s\n", strerror(errno));
		return -1;
	}

	if(fread(line, fs.st_size, 1, fp) != 1) {
		perror("fread");
		free(line);
		return -1;
	}

	line_pos = 0;

	while(line_pos != fs.st_size) {
		/* skip whitespaces */
		while (isspace(line[line_pos])) {
			if(line[line_pos] == '\n')
				line_num++;
			++line_pos;
		}

		/* comment */
		if (line[line_pos] == '#') {
			while (line[line_pos] != '\n') {
				++line_pos;
			}
			++line_num;
			++line_pos;  /* skip the newline as well */
		}

		/* read option */
		for (opt_pos = 0; isprint(line[line_pos]) && line[line_pos] != ' ' &&
				line[line_pos] != '#' && line[line_pos] != '='; /* NOTHING */) {
/* 				printf("%c\n", line[line_pos]); */
			if(line[line_pos] == '\n')
				++line_num;
			opt[opt_pos++] = line[line_pos++];
			if (opt_pos >= MAX_OPT_LEN) {
				snprintf(erbuf, sizeof(erbuf), "too long option at line %d\n", line_num);
				parse_error = 1;
				opt_good = 0;
			}
		}
		if (opt_pos == 0 || parse_error == 1) {
			snprintf(erbuf, sizeof(erbuf), "parse error at line %d (line_pos: %d)\n", line_num, line[line_pos]);
			break;
		} else {
			opt[opt_pos] = '\0';
			opt_good = 1;
		}
		/* skip whitespaces */
		while (isspace(line[line_pos])) {
			if(line[line_pos] == '\n')
				line_num++;
			++line_pos;
		}

		/* check '=' */
		if (line[line_pos++] != '=') {
			snprintf(erbuf, sizeof(erbuf), "Option '%s' needs a parameter (line %d)\n", opt, line_num);
			parse_error = 1;
			break;
		}

		/* skip whitespaces */
		while (isspace(line[line_pos])) {
			if(line[line_pos] == '\n')
				line_num++;
			++line_pos;
		}

		/* read the parameter */

		/* case 1: single string, no linebreaks allowed */
		if (line[line_pos] == '"' || line[line_pos] == '\'') {
			c = line[line_pos];
			++line_pos;  /* skip quote */
			param_good = 1;
			for (param_pos = 0; line[line_pos] != c; /* NOTHING */) {
				if(line_pos < fs.st_size && param_pos < MAX_PARAM_LEN) {
					param[param_pos++] = line[line_pos++];
				} else {
					snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n",opt, line_num);
					parse_error = 1;
					param_good = 0;
					break;
				}
			}
			if(param_good) {
				line_pos++;	/* skip the closing " or ' */
			} else {
				break;
			}
		/* case 2: multiple items, linebreaks allowed */
		} else if (line[line_pos] == '{') {
			c = '}';
			++line_pos;
			param_good = 1;
			for (param_pos = 0; line[line_pos] != c; /* NOTHING */) {
				if(line_pos < fs.st_size && param_pos < MAX_PARAM_LEN) {
					param[param_pos++] = line[line_pos++];
					if(line[line_pos] == '\n')
						line_num++;
				} else {
					snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n", opt, line_num);
					parse_error = 1;
					param_good = 0;
					break;
				}
			}
			if(param_good) {
				line_pos++;	/* skip the closing ')' */
			} else {
				break;
			}
		/* Case 3: integers */
		} else {
			param_good = 1;
			for (param_pos = 0; isprint(line[line_pos]) && !isspace(line[line_pos])
					&& line[line_pos] != '#'; /* NOTHING */) {
				param[param_pos++] = line[line_pos++];
				if (param_pos >= MAX_PARAM_LEN) {
					snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n", opt, line_num);
					parse_error = 1;
					param_good = 0;
					break;
				}
			}
			if(param_good == 0) {
				break;
			}
		}
		param[param_pos] = '\0';
#ifdef DEBUG
		printf("option: %s\n", opt);
		printf("param: %s\n", param);
		printf("shortened: %s\n", shorten(param));
		printf("-----------------\n");
#endif
		/* skip whitespaces */
		while (isspace(line[line_pos])) {
			if(line[line_pos] == '\n')
				line_num++;
			++line_pos;
		}
	}

	if(parse_error == 1) {
		printf("Parse error: %s", erbuf);
		ret = -1;
	}
	if(line)
		free(line);
	return ret;
}

int main(int argc, char **argv) {

	if(argc != 2) {
		printf("Usage: %s <conf-file>\n", argv[0]);
		exit(0);
	}
	if(parse_config_file(argv[1]) == -1)
		printf("Error parsing configuration file\n");
	return 0;
}


