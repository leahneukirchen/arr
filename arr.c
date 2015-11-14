// arr - (re)arrange and select fields on each line

/*
##% gcc -Os -Wall -g -o $STEM $FILE -Wextra -Wwrite-strings
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

char lastsplit;
static char *s;

// TODO: negative numbers count from end
// TODO: C-style literals
// %FIELD shortcut?
// default to as many - as there are references to fields?
// option to don't stop at first EOF, fill with string (default empty)

//  "%{" FIELD (CHOP FIELD)* (("|" | "*") FIELD ((, | :) FIELD)*)? "}"

void
fmt_range(char **args, int bytewise)
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
				printf("%c", args[0][n-1]);
			} else {
				if (printed++)
					printf("%c", lastsplit);
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
			s = end;
			if (bytewise) {
				if (l <= n)
					for (l++; l <= n; l++)
						printf("%c", args[0][l-1]);
				else
					for (l--; l >= n; l--)
						printf("%c", args[0][l-1]);
			} else {
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
fmt_inner(char **args)
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
			field = newfield;
			break;
		case '|':
			s++;
			fmt_range(args, 0);
			return;
		case '*':
			s++;
			fmt_range(args+field, 1);
			return;
		case '}':
			s++;
			if (field == 0)
				abort();
			printf("%s", args[field]);
			return;
		case '"':
			s++;
			lastsplit = *s;
			// XXX check for second "
			s++;
			s++;
			goto split2;
		default: { /* split at char, recurse */
			split:
			/* TODO "|" */
			if (field == 0)
				abort();
			lastsplit = *s;
			s++;
			split2:;
			char *t = strdup(args[field]);
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
			field = 0;
		}
		}
	}
}

void
fmt(const char *pattern, char **args)
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
				fmt_inner(args);
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
	char s[] = "foo,bar,baz,quux";
	char t[] = "bo/ing;zi/ng;zo/ng";
	char v[] = "  words  wordss\t wordsss";
	char *a[] = { 0, s, t, v };
	/* fmt("hello\n", a); */
	/* fmt("%{1}\n", a); */
	/* fmt("%{2}\n", a); */
	/* fmt("%{1,2}\n", a); */
	/* fmt(">>%{2}<<\n", a); */
	/* fmt(">>%{2;|1:3}<<\n", a); */
	/* fmt(">>%{2;|3:1}<<\n", a); */

	/* fmt(">>%{2;|1:3} %{2;|3:1} %{2;|1,3}<<\n", a); */
	/* fmt(">>%{2;|1,2,3} %{2;|3,2,1} %{2;|1,3}<<\n", a); */
	/* fmt(">>%{2;2}<<\n", a); */
	/* fmt(">>%{2;2/1}<<\n", a); */
	/* fmt(">>%{3 |1:3}<<\n", a); */
	/* fmt(">>%{3 1*1}<<\n", a); */
	/* fmt(">>%{3 1*1:4}<<\n", a); */
	/* fmt("%zz%%zz\n", a); */

        // number of implicit stdin arguments
        int stdins = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s FMT FILES...\n", argv[0]);
		exit(1);
	}

	// default to stdin when no file arguments are given
	if (argc == 2)
		stdins++;

	FILE **files = calloc(argc+stdins, sizeof (FILE *));
	char **lines = calloc(argc+stdins, sizeof (char *));
	int i;
	for (i = 1; i < argc-1+stdins; i++)
		if (i >= argc-1 || strcmp(argv[i+1], "-") == 0) {
			files[i] = stdin;
		} else {
			files[i] = fopen(argv[i+1], "r");
			if (!files[i]) {
				fprintf(stderr, "%s: %s: %s\n",
				    argv[0], argv[i+1], strerror(errno));
				exit(1);
			}
		}

	int eof;
	size_t len;
	int delim = '\n';
	while (1) {
		eof = 0;
		for (i = 1; i < argc-1+stdins; i++) {
			int read = getdelim(lines+i, &len, delim, files[i]);
			if (read == -1) {
				if (feof(files[i])) {
					lines[i] = strdup("(EOF)");
					eof++;
				} else {
					exit(1);
				}
			}
			if (lines[i][read-1] == delim)  // strip delimiter
				lines[i][read-1] = 0;
		}
		if (eof)
			break;
		fmt(argv[1], lines);
		printf("%c", delim);
	}

	return 0;
}
