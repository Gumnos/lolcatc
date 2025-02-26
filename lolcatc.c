#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>

/*
#define ANIMATE
*/

const char * const PROGNAME = "lolcatc";
const char * const VERSION = "0.1.0";
const char * const OPTIONS = "F:hip:S:v"
#ifdef ANIMATE
	"ad:s:"
#endif
	;

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

inline
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
	if (msg) fprintf(fp, "error: %s\n", msg);
	fprintf(fp, "\n%s [options] [filename]\n", PROGNAME);
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
		case 'a':
			options->animate = true;
			break;
		case 'd':
			options->duration = atoi(optarg);
			if (options->duration <= 1) {
				print_help(stderr, options, "bad duration");
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
		case 'p':
			options->spread = atof(optarg);
			if (!valid_double(options->spread)) {
				print_help(stderr, options, "bad spread");
				return EX_USAGE;
			}
			break;
		case 'S': /* seed */
			options->seed = atoi(optarg);
			if (options->seed <= 0) {
				print_help(stderr, options, "bad seed");
				return EX_USAGE;
			}
			break;
		case 'v': /* version */
			print_version(stdout);
			/* FALLTHROUGH */
		case 'h': /* help */
			print_help(stdout, options, NULL);
			return EX_OK;
	   }
	}
	*argc -= optind;
	*argv += optind;
	return EX_OK;
}

int
main(int argc, char *argv[]) {
	options_t options = {
		.seed = 0,
		.spread = 3.0,
		.frequency = 0.1,
		.invert = false,
		.force = false,
		.animate = false,
		.duration = 12,
		.speed = 20.0,
		};
	int status;
	if ((status = parse_args(&options, &argc, &argv)) != EX_OK)
		return status;
	return EX_OK;
}
