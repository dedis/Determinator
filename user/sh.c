#if LAB >= 6
#include "lib.h"

//
// get the next token from string s
// set *p1 to the beginning of the token and
// *p2 just past the token.
// return:
//	0 for end-of-string
//	> for >
//	| for |
//	w for a word
//
// eventually (once we parse the space where the nul will go),
// words get nul-terminated.
int
_gettoken(char *s, char **p1, char **p2)
{
	while(*s == ' ' || *s == '\t' || *s == '\n')
		*s++ = 0;
	if(*s == 0)
		return 0;
	if(*s == '>'){
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return '>';
	}
	if(*s == '|'){
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return '|';
	}
	*p1 = s;
	while(*s && !strchr(" \t\n>|", *s))
		s++;
	*p2 = s;
	return 'w';
}

int
gettoken(char *s, char **p1, char **p2)
{
	static int c, nc;
	static char *np1, *np2;
	static int first = 1;

	if(first){
		nc = _gettoken(s, &np1, &np2);
		first = 0;
	}
	c = nc;
	*p1 = np1;
	*p2 = np2;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, r, p[2], fd;

again:
	argc = 0;
	for(;;){
		c = gettoken(s, &t, &s);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				printf("too many arguments\n");
				return;
			}
			argv[argc++] = t;
			break;
		case '>':
			if(gettoken(s, &t, &s) != 'w'){
				printf("syntax error: > not followed by word\n");
				return;
			}
			if ((fd = open(t, O_WRONLY)) < 0) {
				printf("open %s for write: %e", t, fd);
				return;
			}
			if(fd != 1){
				dup(fd, 1);
				close(fd);
			}
			break;
		case '|':
			if((r=pipe(p)) < 0){
				printf("pipe: %e", r);
				return;
			}
			if((r=fork()) < 0){
				printf("fork: %e", r);
				return;
			}
			if(r == 0){
				if(p[0] != 0){
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			}else{
				if(p[1] != 1){
					dup(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
			break;
		}
	}

runit:
	if(argc == 0)
		return;
	if((r=spawn(argv[0], argv)) < 0){
		printf("spawn %s: %e", argv[0], r);
		return;
	}
	wait(r);
	exit();
}

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
			buf[0] = 0;
			return;
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
usage(void)
{
	printf("usage: sh [-i] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int r, interactive;

	interactive = '?';
	ARGBEGIN{
	case 'i':
		interactive = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1){
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			panic("open %s: %e", r);
		assert(r==0);
	}
	if(interactive == '?')
		interactive = isconsole(0);
	for(;;){
		readline(buf, sizeof buf);
		runcmd(buf);
	}
}

#endif
