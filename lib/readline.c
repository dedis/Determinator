#if LAB >= 1

#include <inc/stdio.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i = 0;

	if (prompt != NULL)
		printf("%s", prompt);

	while (1) {
		int c = getchar();
			; // spin
		if (c < 0) {
			printf("read error: %e", c);
			return NULL;
		}
		else if (c >= ' ' && i < BUFLEN-1) {
			putchar(c);
			buf[i++] = c;
		}
		else if (c == '\b' && i > 0) {
			putchar(c);
			i--;
		}
		else if (c == '\n') {
			putchar(c);
			buf[i] = 0;
			return buf;
		}
	}
}

#endif	// LAB >= 1
