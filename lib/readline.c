#if LAB >= 1

#include <inc/stdio.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i, c, echoing;

	if (prompt != NULL)
		printf("%s", prompt);

	i = 0;
	echoing = iscons(0);
	for(;;) {
		c = getchar();
		if (c < 0) {
			printf("read error: %e\n", c);
			return NULL;
		}
		else if (c >= ' ' && i < BUFLEN-1) {
			if(echoing)
				putchar(c);
			buf[i++] = c;
		}
		else if (c == '\b' && i > 0) {
			if(echoing)
				putchar(c);
			i--;
		}
		else if (c == '\n') {
			if(echoing)
				putchar(c);
			buf[i] = 0;
			return buf;
		}
	}
}

#endif	// LAB >= 1
