#if LAB >= 4
#include <inc/stdio.h>
#include <inc/unistd.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i, c, echoing;

	if (prompt != NULL)
		fprintf(stdout, "%s", prompt);

	i = 0;
	echoing = isatty(0);
	while (1) {
		c = getchar();
		if (c < 0) {
			if (c != EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			if (echoing)
				putchar('\b');
			i--;
		} else if (c >= ' ' && i < BUFLEN-1) {
			if (echoing)
				putchar(c);
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			if (echoing) {
				putchar('\n');
				fflush(stdout);
			}
			buf[i] = 0;
			return buf;
		}
	}
}

#endif	// LAB >= 4
