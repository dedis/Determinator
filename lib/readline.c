#if LAB >= 6

#include <inc/lib.h>

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

#endif	// LAB >= 6
