#if LAB >= 6

#include <inc/lib.h>

int 
memcmp(char *a, char *b, int n)
{
	int i;

	for(i=0; i<n; i++)
		if(a[i] != b[i])
			return 1;
	return 0;
}

void
umain(int argc, char **argv)
{
	char *buf;
	int n;
	void *v;

	for(;;){
		buf = readline("> ");
		if(buf == 0)
			exit();
		if(memcmp(buf, "free ", 5) == 0){
			v = (void*)strtol(buf+5, 0, 0);
			free(v);
		}
		else if(memcmp(buf, "malloc ", 7) == 0){
			n = strtol(buf+7, 0, 0);
			v = malloc(n);
			printf("\t0x%x\n", (u_int)v);
		}
		else
			printf("?unknown command\n");
	}
}
#endif
