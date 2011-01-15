#if LAB >= 9
#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/string.h>

char * optarg;
int optind = 1;
int opterr = 1;
int optopt = 0;
static int next_index = 1;

int getopt(int argc, char * argv[], const char * optstring) {

	char * argv_tmp[argc];
	char * opt, * match;
	char ** s_cur, ** s_end, ** d_cur, ** d_end;
	int cur;
	bool two_strings = false;
	enum status_codes {good, invalid, missing};
	enum status_codes status = good;

	// Remove any previous optarg.
	optarg = NULL;

	// Find the current option.
	cur = optind;
	while (cur < argc && argv[cur][0] != '-')
		cur++;

	// No more options?
	if (cur == argc)
		return -1;

	opt = argv[cur];

	// Empty option?
	if (opt[1] == '\0')
		status = invalid;

	else {
		match = strchr(optstring, opt[1]);

		// Invalid option?
		if (match == NULL)
			status = invalid;
		
		// Argument?
		else if (match[1] == ':') {
			if (opt[2] != '\0')
				optarg = &opt[2];
			else if (match[2] != ':' && cur + 1 < argc) {
				optarg = argv[cur + 1];
				two_strings = true;
			}
			else
				status = missing;
		}
	}

	s_cur = argv;
	d_cur = argv_tmp;
	d_end = argv_tmp + next_index;
	while (d_cur < d_end)
		*d_cur++ = *s_cur++;
	*d_cur++ = argv[cur];
	if (two_strings)
		*d_cur++ = argv[cur + 1];
	s_end = argv + cur;
	while (s_cur < s_end)
		*d_cur++ = *s_cur++;
	s_cur++;
	if (two_strings) s_cur++;
	s_end = argv + argc;
	while (s_cur < s_end)
		*d_cur++ = *s_cur++;

	memmove(argv, argv_tmp, argc * sizeof(argv[0]));

	next_index += (two_strings && next_index < argc) ? 2 : 1;

	// Update optind.
	optind = next_index;
	while (optind < argc && argv[optind][0] != '-')
		optind++;
	if (optind == argc)
		optind = next_index;

	switch (status) {
	case invalid:
		optopt = (int) opt[1];
		if (opterr)
			fprintf(stderr, "Invalid option: %c\n", optopt);
		return '?';
	case missing:
		optopt = (int) opt[1];
		if (opterr)
			fprintf(stderr, "Missing required argument to option %c\n", optopt);
		return ':';
	case good:
	default:
		optopt = 0;
		return (int) opt[1];
	}
}

#endif	// LAB >= 9
