#if LAB >= 6
#include "lib.h"

#define debug 0

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
	int t;

	if (s == 0) {
		if (debug > 1) printf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1) printf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while(*s == ' ' || *s == '\t' || *s == '\n')
		*s++ = 0;
	if(*s == 0) {
		if (debug > 1) printf("EOL\n");
		return 0;
	}
	if(*s == '>' || *s == '|'){
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1) printf("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while(*s && !strchr(" \t\n>|", *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;
		**p2 = 0;
		printf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 16
void
runcmd(char *s)
{
	char *argv[MAXARGS], *t;
	int argc, c, i, r, p[2], fd, rightpipe;

	rightpipe = 0;
	gettoken(s, 0);
again:
	argc = 0;
	for(;;){
		c = gettoken(0, &t);
		switch(c){
		case 0:
			goto runit;
		case 'w':
			if(argc == MAXARGS){
				printf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '>':
			if(gettoken(0, &t) != 'w'){
				printf("syntax error: > not followed by word\n");
				exit();
			}
			if ((fd = open(t, O_WRONLY)) < 0) {
				printf("open %s for write: %e", t, fd);
				exit();
			}
			if(fd != 1){
				dup(fd, 1);
				close(fd);
			}
			break;
		case '|':
			if((r=pipe(p)) < 0){
				printf("pipe: %e", r);
				exit();
			}
			if (debug) printf("PIPE: %d %d\n", p[0], p[1]);
			if((r=fork()) < 0){
				printf("fork: %e", r);
				exit();
			}
			if(r == 0){
				if(p[0] != 0){
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			}else{
				rightpipe = r;
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
	if(argc == 0) {
		if (debug) printf("EMPTY COMMAND\n");
		return;
	}
	argv[argc] = 0;
	if (debug) {
		printf("[%08x] SPAWN:", env->env_id);
		for (i=0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("\n");
	}
	if ((r = spawn(argv[0], argv)) < 0)
		printf("spawn %s: %e\n", argv[0], r);
	close_all();
	if (r >= 0) {
		if (debug) printf("[%08x] WAIT %s %08x\n", env->env_id, argv[0], r);
		wait(r);
		if (debug) printf("[%08x] wait finished\n", env->env_id);
	}
	if (rightpipe) {
		if (debug) printf("[%08x] WAIT right-pipe %08x\n", env->env_id, rightpipe);
		wait(rightpipe);
		if (debug) printf("[%08x] wait finished\n", env->env_id);
	}
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
		interactive = iscons(0);
	for(;;){
		if (interactive)
			fprintf(1, "$ ");
		readline(buf, sizeof buf);
		if (debug) printf("LINE: %s\n", buf);
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (r == 0) {
			runcmd(buf);
			exit();
		} else
			wait(r);
	}
}

#endif
