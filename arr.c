// arr - (re)arrange and select fields on each line

/*
##% gcc -Os -Wall -g -o $STEM $FILE -Wextra -Wwrite-strings
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char lastsplit;
static char *s;

//  "%{" FIELD (CHOP FIELD)* (("|" | "*") FIELD ((, | :) FIELD)*)? "}"

void
fmt_range(char **args, int bytewise, size_t argsnum)
{
	long n = 0;
	char *end;

	int printed = 0;

	while (*s) {
		switch (*s) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '-':
			errno = 0;
			n = strtol(s, &end, 10);
			if (errno != 0) {
				fprintf(stderr, "can't parse number at '%s'.\n", s);
				exit(1);
			}
			s = end;
			if (bytewise) {
				if (n < 0)
					n += argsnum + 1;
				if (n < 0)
					n = 0;
				if (n >= 1 && n <= argsnum)
					printf("%c", args[0][n-1]);
			} else {
				if (n < 0)
					n += argsnum;
				if (printed++)
					printf("%c", lastsplit);
				if (n > 0 && n < argsnum)
					printf("%s", args[n]);
			}
			break;
		case ',':
			n = 0;
			s++;
			break;
		case ':': {
			s++;
			long l = n;
			errno = 0;
			n = strtol(s, &end, 10);
			if (errno != 0) {
				fprintf(stderr, "can't parse number at '%s'.\n", s);
				exit(1);
			}
			if (s == end)  // default to -1
				n = -1;
			s = end;
			if (bytewise) {
				if (n < 0)
					n += argsnum + 1;
				if (n < 0)
					n = 0;
				if (n > argsnum)
					n = argsnum;
				if (l <= n)
					for (l++; l <= n; l++)
						printf("%c", args[0][l-1]);
				else
					for (l--; l >= n; l--)
						printf("%c", args[0][l-1]);
			} else {
				if (n < 0)
					n += argsnum;
				if (n >= argsnum)
					n = argsnum-1;
				if (l <= n)
					for (l++; l <= n; l++) {
						if (printed++)
							printf("%c", lastsplit);
						printf("%s", args[l]);
					}
				else
					for (l--; l >= n; l--) {
						if (printed++)
							printf("%c", lastsplit);
						printf("%s", args[l]);
					}
			}
			break;
		}
		case '}':
			s++;
			return;
		default:
			fprintf(stderr, "invalid range at '%s'\n", s);
			exit(1);
		}
	}
}

void
fmt_inner(char **args, size_t argsnum)
{
	long field, newfield;
	char *end;
	int i;

	field = 0;

	while (*s) {
		switch (*s) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '-':
			errno = 0;
			newfield = strtol(s, &end, 10);
			if (errno != 0) {
				fprintf(stderr, "can't parse number at '%s'.\n", s);
				exit(1);
			}
			if (s == end && *s == '-')
				goto split;
			s = end;
			if (newfield < 0)
				newfield += argsnum;
			field = newfield;
			break;
		case '|':
			s++;
			fmt_range(args, 0, argsnum);
			return;
		case '*':
			s++;
			if (field >= 0 && field < argsnum)
				fmt_range(args+field, 1, strlen(args[field]));
			else
				fmt_range(args, 1, 0);
			return;
		case '}':
			s++;
			if (field >= 1 && field < argsnum)
				printf("%s", args[field]);
			return;
		case '"':
			s++;
			lastsplit = *s;
			s++;
			if (*s != '"') {
				fprintf(stderr, "invalid syntax '\"%c%s'\n",
				    lastsplit, s);
				exit(1);
			}
			s++;
			goto split2;
		default: { /* split at char, recurse */
			split:
			lastsplit = *s;
			s++;
			split2:;
			char *t;
			if (field >= 1 && field < argsnum)
				t = strdup(args[field]);
			else
				t = strdup("");
			char **newargs = calloc(32 /*XXX*/, sizeof (char *));
			if (lastsplit == ' ') {  // split on any ws
				while (isspace((unsigned char)*t))
					t++;
				newargs[1] = t;
				for (i = 2; *t; t++) {
					if (isspace((unsigned char)*t)) {
						while (isspace((unsigned char)*(t+1)))
							*t++ = 0;
						newargs[i++] = t + 1;
					}
				}
			} else {
				newargs[1] = t;
				for (i = 2; *t; t++) {
					if (*t == lastsplit) {
						*t = 0;
						newargs[i++] = t + 1;
					}
				}
			}
			args = newargs;
			argsnum = i;
			field = 0;
		}
		}
	}
}

void
fmt(const char *pattern, char **args, size_t argsnum)
{
	s = pattern;
	lastsplit = '\t';

	while (*s) {
		switch (*s) {
		case '%':
			s++;
			switch (*s) {
			case '%':
				s++;
				putchar('%');
				break;
			case '{':
				s++;
				fmt_inner(args, argsnum);
				break;
			default:
				putchar('%');
			}
			break;
		default:
			putchar(*s);
			s++;
		}
	}
}

int
main(int argc, char *argv[]) {
        // number of implicit stdin arguments
        int stdins = 0;

	char c;
	char *padding = "";
	int delim = '\n';

	while ((c = getopt(argc, argv, "0Pp:")) != -1)
		switch(c) {
		case '0': delim = '\0'; break;
		case 'p': padding = optarg; break;
		case 'P': padding = 0; break;
                default: goto usage;
		}

	if (optind >= argc) {
usage:
		fprintf(stderr, "Usage: %s FMT FILES...\n", argv[0]);
		exit(1);
	}

	int argnum = argc - optind;
	char **arglist = argv + optind;  // starts counting at 1

	// default to stdin when no file arguments are given
	if (argnum <= 1)
		stdins++;

	FILE **files = calloc(argnum+stdins, sizeof (FILE *));
	char **lines = calloc(argnum+stdins, sizeof (char *));
	int i;
	for (i = 1; i < argnum+stdins; i++) {
		if (i >= argnum || strcmp(arglist[i], "-") == 0) {
			files[i] = stdin;
		} else {
			files[i] = fopen(arglist[i], "r");
			if (!files[i]) {
				fprintf(stderr, "%s: %s: %s\n",
				    argv[0], arglist[i], strerror(errno));
				exit(1);
			}
		}
	}

	int eof;
	size_t len;
	while (1) {
		eof = 0;
		for (i = 1; i < argnum+stdins; i++) {
			int read = getdelim(lines+i, &len, delim, files[i]);
			if (read == -1) {
				if (feof(files[i])) {
					if (!padding)
						exit(0);
					lines[i] = strdup(padding);
					eof++;
				} else {
					exit(1);
				}
			} else {
				// strip delimiter
				if (lines[i][read-1] == delim)
					lines[i][read-1] = 0;
			}
		}
		if (eof >= argnum+stdins-1)
			break;
		fmt(arglist[0], lines, argnum+stdins);
		printf("%c", delim);
	}

	return 0;
}
