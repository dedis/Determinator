#if LAB >= 6
#include "lib.h"

void
readline(char *buf, u_int n)
{
	int i, r;

	r = 0;
	for(i=0; i<n; i++){
		if((r = read(0, buf+i, 1)) != 1){
			if(r == 0)
				printf("unexpected eof\n");
			else
				printf("read error: %e", r);
			exit();
		}
		if(buf[i] == '\b'){
			if(i >= 2)
				i -= 2;
			else
				i = 0;
		}
		if(buf[i] == '\n'){
			buf[i] = 0;
			return;
		}
	}
	printf("line too long\n");
	while((r = read(0, buf, 1)) == 1 && buf[0] != '\n')
		;
	buf[0] = 0;
}	

char buf[1024];

void
umain(void)
{
	int r;

	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);

	for(;;){
		printf("Type a line: ");
		readline(buf, sizeof buf);
		fprintf(1, "%s\n", buf);
	}
}
#endif
