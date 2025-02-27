#include <ctype.h>
#include <err.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

/*
#define ANIMATE
*/

#define MAX_ANSI_SEQ_LEN 256
#define ESC '\033'

const char * const PROGNAME = "lolcatc";
const char * const VERSION = "0.1.0";
const char * const OPTIONS = "afitvhdF:p:S:"
#ifdef ANIMATE
	"ad:s:"
#endif
	;

typedef struct {
	bool reading_escape;
	unsigned char prev_char;
	double counter;
	double offset;
	int prev_r, prev_g, prev_b;
} context_t;

typedef struct {
	int duration;
	int seed;
	double frequency;
	double speed;
	double spread;
	bool animate;
	bool force;
	bool invert;
	bool truecolor;
} options_t;

bool
valid_double(double d) {
	return d != 0.0 && d != HUGE_VAL && d != -HUGE_VAL;
}

void
print_version(FILE *fp) {
	fprintf(fp, "%s version %s\n", PROGNAME, VERSION);
}

void
print_help(FILE *fp, const options_t *options, const char *msg) {
	if (msg) fprintf(fp, "error: %s\n\n", msg);
	fprintf(fp, "%s [options] [filename]\n", PROGNAME);
	fprintf(fp, " -v  Version\n");
	fprintf(fp, " -h  This help\n");
	fprintf(fp, " -p  Spread (default=%0.2f)\n", options->spread);
	fprintf(fp, " -F  Frequency (default=%0.2f)\n", options->frequency);
	fprintf(fp, " -S  Seed (default=%d)\n", options->seed);
	fprintf(fp, " -i  Invert foreground/background\n");
	fprintf(fp, " -t  24-bit color output\n");
	fprintf(fp, " -f  Force color even when stdout is not a tty\n");
#ifdef ANIMATE
	fprintf(fp, " -a  Animate\n");
	fprintf(fp, " -d  Animation duration (default=%d)\n", options->duration);
	fprintf(fp, " -s  Animation speed (default=%0.2f)\n", options->speed);
#endif
}

int
parse_args(options_t *options, int *argc, char **argv[]) {
	int ch;
	while ((ch = getopt(*argc, *argv, OPTIONS)) != -1) {
		switch (ch) {
		/* no args */
		case 'a': options->animate = true; break;
		case 'f': options->force = true; break;
		case 'i': options->invert = true; break;
		case 't': options->truecolor = true; break;
		case 'v': print_version(stdout);
			/* FALLTHROUGH */
		case 'h': /* help */
			print_help(stdout, options, NULL);
			return EX_USAGE;
		/* things that have args */
		case 'd':
			options->duration = atoi(optarg);
			if (options->duration <= 1) {
				print_help(stderr, options, "bad duration");
				return EX_USAGE;
			}
			break;
		case 'p':
			options->spread = atof(optarg);
			if (!valid_double(options->spread)) {
				print_help(stderr, options, "bad spread");
				return EX_USAGE;
			}
			break;
		case 's':
			options->speed = atof(optarg);
			if (!valid_double(options->speed) || options->speed < 0) {
				print_help(stderr, options, "bad speed");
				return EX_USAGE;
			}
			break;
		case 'F':
			options->frequency = atof(optarg);
			if (!valid_double(options->frequency) || options->frequency < 0) {
				print_help(stderr, options, "bad frequency");
				return EX_USAGE;
			}
			break;
		case 'S':
			options->seed = atoi(optarg);
			if (options->seed <= 0) {
				print_help(stderr, options, "bad seed");
				return EX_USAGE;
			}
			break;
	   }
	}
	*argc -= optind;
	*argv += optind;
	return EX_OK;
}

void
rainbow(
		options_t *options,
		context_t *context,
		unsigned char c
		) {
	int r, g, b;
	double f = options->frequency, a;
	if (c == ESC) {
		context->reading_escape = true;
	} else {
		if (context->reading_escape) {
			if (isalpha(c) || c == 0x07) {
				context->reading_escape = false;
			}
		} else {
			a = context->counter + context->offset;
			r = (int)(sin(f*a + 0       ) * 127 + 128);
			g = (int)(sin(f*a + 2*M_PI/3) * 127 + 128);
			b = (int)(sin(f*a + 4*M_PI/3) * 127 + 128);
			context->counter += options->spread;
			if (!options->truecolor) {
				r = (r & 0xc0) >> 6;
				b = (b & 0xc0) >> 6;
				g = (g & 0xc0) >> 6;
			}
			if (context->prev_r != r
					|| context->prev_b != b
					|| context->prev_g != g
					) {
				if (options->truecolor) {
						printf("\e[%s;2;%d;%d;%dm",
							options->invert ? "48" : "38",
							r, g, b
							);
				} else {
					printf("\e[%s%s3%im",
						options->invert ? "7;" : "",
						(r & 0x02 || b & 0x02 || c & 0x02) ? "1;" : "21;",
						((r & 0x03) ? 0x01 : 0)
						|
						((g & 0x03) ? 0x02 : 0)
						|
						((b & 0x03) ? 0x04 : 0)
						);
				}
				context->prev_r = r;
				context->prev_b = b;
				context->prev_g = g;
			}
			putchar(c);
			if (c == '\n') {
				++context->offset;
				context->counter = 0;
			}
		}
	}
}

int
run_file(
		options_t *options,
		context_t *context,
		int fnum,
		bool colorize,
		const char *name
		) {
	unsigned char buffer[BUFSIZ];
	unsigned char *cp;
	size_t byte_count;

	while ((byte_count = read(fnum, buffer, BUFSIZ))) {
		for (cp = buffer; byte_count; cp++, byte_count--) {
			if (colorize) {
				rainbow(options, context, *cp);
			} else {
				putchar(*cp);
			}
		}
	}
	return EX_OK;
}

int
run(options_t *options, int argc, char *argv[]) {
	FILE *fp;
	const char *name;
	bool tty = isatty(fileno(stdout));
	int i, result, fnum;
	context_t context = {
		.counter=0,
		.offset=0,
		.reading_escape=false,
		.prev_char=0,
		.prev_r=0,
		.prev_g=0,
		.prev_b=0,
		};

	if (argc) {
		for (i=0; i<argc; i++) {
			name = argv[i];
			if (strcmp(name, "-")) {
				fp = fopen(name, "r");
				if (!fp) {
					err(EX_OSERR, "could not open %s", name);
				}
			} else {
				fp = stdin;
			}
			fnum = fileno(fp);
			if ((result = run_file(
					options,
					&context,
					fnum,
					tty || options->force,
					name
					)) != EX_OK) {
				if (fp != stdin) fclose(fp);
				return result;
			}
			if (fp != stdin) fclose(fp);
		}
	} else {
		fnum = fileno(stdin);
		result = run_file(
			options,
			&context,
			fnum,
			tty || options->force,
			"(stdin)"
			);
	}
	/* reset when done */
	if (tty || options->force) printf("\e[m\e[?25h\e[?1;5;2004l");
	return result;
}

int
main(int argc, char *argv[]) {
	options_t options = {
		.seed = 0,
		.spread = 3.0,
		.frequency = 0.1,
		.invert = false,
		.truecolor = false,
		.force = false,
		.animate = false,
		.duration = 12,
		.speed = 20.0,
		};
	int status;
#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif

	if ((status = parse_args(&options, &argc, &argv)) != EX_OK)
		return status;
	return run(&options, argc, argv);
}
