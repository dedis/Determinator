#if LAB >= 4
/*
 * Simple Unix-style command shell usable in interactive or script mode.
 *
 * Copyright (C) 1997 Massachusetts Institute of Technology
 * See section "MIT License" in the file LICENSES for licensing terms.
 *
 * Derived from the MIT Exokernel and JOS.
 * Adapted for PIOS by Bryan Ford at Yale University.
 */

#if SOL >= 1
// NOTE: The "#if LAB >= 1" sections below used to be "#if SOL >= 7"
// sections.  Now we gave them the entire shell in the last lab so
// they would have more time to focus on the final project.
#endif
#include <inc/cdefs.h>
#include <inc/stdio.h>
#include <inc/stdlib.h>
#include <inc/unistd.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/errno.h>
#include <inc/args.h>

#define BUFSIZ 1024		/* Find the buffer overrun bug! */
int debug = 0;


// gettoken(s, 0) prepares gettoken for subsequent calls and returns 0.
// gettoken(0, token) parses a shell token from the previously set string,
// null-terminates that token, stores the token pointer in '*token',
// and returns a token ID (0, '<', '>', '|', or 'w').
// Subsequent calls to 'gettoken(0, token)' will return subsequent
// tokens from the string.
int gettoken(char *s, char **token);


// Parse a shell command from string 's' and execute it.
// Do not return until the shell command is finished.
// runcmd() is called in a forked child,
// so it's OK to manipulate file descriptor state.
#define MAXARGS 256
void gcc_noreturn
runcmd(char* s)
{
	char *argv[MAXARGS], *t, argv0buf[BUFSIZ];
	int argc, c, i, r, p[2], fd, pipe_child;

	pipe_child = 0;
	gettoken(s, 0);
	
again:
	argc = 0;
	while (1) {
		switch ((c = gettoken(0, &t))) {

		case 'w':	// Add an argument
			if (argc == MAXARGS) {
				cprintf("sh: too many arguments\n");
				exit(EXIT_FAILURE);
			}
			argv[argc++] = t;
			break;
			
		case '<':	// Input redirection
			// Grab the filename from the argument list
			if (gettoken(0, &t) != 'w') {
				cprintf("syntax error: < not followed by word\n");
				exit(EXIT_FAILURE);
			}
#if LAB >= 1
			if ((fd = open(t, O_RDONLY)) < 0) {
				cprintf("open %s for read: %e", t, fd);
				exit(EXIT_FAILURE);
			}
			if (fd != 0) {
				dup2(fd, 0);
				close(fd);
			}
#else
			// Open 't' for reading as file descriptor 0
			// (which environments use as standard input).
			// We can't open a file onto a particular descriptor,
			// so open the file as 'fd',
			// then check whether 'fd' is 0.
			// If not, dup 'fd' onto file descriptor 0,
			// then close the original 'fd'.
			
			// LAB 5: Your code here.
			panic("< redirection not implemented");
#endif
			break;
			
		case '>':	// Output redirection
			// Grab the filename from the argument list
			if (gettoken(0, &t) != 'w') {
				cprintf("syntax error: > not followed by word\n");
				exit(EXIT_FAILURE);
			}
#if LAB >= 1
			if ((fd = open(t, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
				cprintf("open %s for write: %s", t,
					strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (fd != 1) {
				dup2(fd, 1);
				close(fd);
			}
#else
			// Open 't' for writing as file descriptor 1
			// (which environments use as standard output).
			// We can't open a file onto a particular descriptor,
			// so open the file as 'fd',
			// then check whether 'fd' is 1.
			// If not, dup 'fd' onto file descriptor 1,
			// then close the original 'fd'.
			// Also, truncate fd.
			
			// LAB 5: Your code here.
			panic("> redirection not implemented");
#endif
			break;
			
#if LAB >= 99
		case '|':	// Pipe
#if LAB >= 1
			if ((r = pipe(p)) < 0) {
				cprintf("pipe: %e", r);
				exit(EXIT_FAILURE);
			}
			if (debug)
				cprintf("PIPE: %d %d\n", p[0], p[1]);
			if ((r = fork()) < 0) {
				cprintf("fork: %e", r);
				exit(EXIT_FAILURE);
			}
			if (r == 0) {
				if (p[0] != 0) {
					dup2(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			} else {
				pipe_child = r;
				if (p[1] != 1) {
					dup2(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
#else
			// Set up pipe redirection.
			
			// Allocate a pipe by calling 'pipe(p)'.
			// Like the Unix version of pipe() (man 2 pipe),
			// this function allocates two file descriptors;
			// data written onto 'p[1]' can be read from 'p[0]'.
			// Then fork.
			// The child runs the right side of the pipe:
			//	Use dup() to duplicate the read end of the pipe
			//	(p[0]) onto file descriptor 0 (standard input).
			//	Then close the pipe (both p[0] and p[1]).
			//	(The read end will still be open, as file
			//	descriptor 0.)
			//	Then 'goto again', to parse the rest of the
			//	command line as a new command.
			// The parent runs the left side of the pipe:
			//	Set 'pipe_child' to the child env ID.
			//	dup() the write end of the pipe onto
			//	file descriptor 1 (standard output).
			//	Then close the pipe.
			//	Then 'goto runit', to execute this piece of
			//	the pipeline.

			// LAB 5: Your code here.
#endif
			panic("| not implemented");
			break;

#endif	// LAB >= 99
		case 0:		// String is complete
			// Run the current command!
			goto runit;
			
		default:
			panic("bad return %d from gettoken", c);
			break;
			
		}
	}

runit:
	// Return immediately if command line was empty.
	if(argc == 0) {
		if (debug)
			cprintf("EMPTY COMMAND\n");
		exit(EXIT_SUCCESS);
	}

	// Clean up command line.
	// Read all commands from the filesystem: add an initial '/' to
	// the command name.
	// This essentially acts like 'PATH=/'.
	if (argv[0][0] != '/') {
		argv0buf[0] = '/';
		strcpy(argv0buf + 1, argv[0]);
		argv[0] = argv0buf;
	}
	argv[argc] = 0;

	// Print the command.
	if (debug) {
		cprintf("execv:");
		for (i = 0; argv[i]; i++)
			cprintf(" %s", argv[i]);
		cprintf("\n");
	}

	// Run the command!
	if (execv(argv[0], argv) < 0)
		cprintf("exec %s: %s\n", argv[0], strerror(errno));
	exit(EXIT_FAILURE);
}


// Get the next token from string s.
// Set *p1 to the beginning of the token and *p2 just past the token.
// Returns
//	0 for end-of-string;
//	< for <;
//	> for >;
//	| for |;
//	w for a word.
//
// Eventually (once we parse the space where the \0 will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug > 1)
			cprintf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1)
		cprintf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while (*s && strchr(WHITESPACE, *s))
		*s++ = 0;
	if (*s == 0) {
		if (debug > 1)
			cprintf("EOL\n");
		return 0;
	}
	if (strchr(SYMBOLS, *s)) {
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1)
			cprintf("TOK %c\n", t);
		return t;
	}
	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;
		**p2 = 0;
		cprintf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char* np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}


void
usage(void)
{
	cprintf("usage: sh [-dix] [command-file]\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int r, interactive, echocmds;
	cputs("[USER MODE]\n");

	interactive = '?';
	echocmds = 0;
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}ARGEND

	if (argc > 1)
		usage();
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0)
			panic("open %s: %s", argv[0], strerror(errno));
		assert(r == 0);
	}
	if (interactive == '?')
		interactive = isatty(0);

	while (1) {
		char *buf;

		buf = readline(interactive ? "$ " : NULL);
		if (buf == NULL) {
			if (debug)
				cprintf("EXITING\n");
			exit(EXIT_SUCCESS);	// end of file
		}
		if (debug)
			cprintf("LINE: %s\n", buf);
		if (buf[0] == '#')
			continue;
		if (echocmds)
			fprintf(stdout, "# %s\n", buf);
		if (strcmp(buf, "exit") == 0)	// built-in command
			exit(0);
		if (strcmp(buf, "cwd") == 0) {
			printf("%s\n", files->fi[files->cwd].de.d_name);
			continue;
		}
		if (debug)
			cprintf("BEFORE FORK\n");
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (debug)
			cprintf("FORK: %d\n", r);
		if (r == 0) {
			runcmd(buf);
			exit(EXIT_SUCCESS);
		} else
			waitpid(r, NULL, 0);
	}
}

#endif	// LAB >= 4
