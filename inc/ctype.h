#ifndef PIOS_INC_CTYPE_H
#define PIOS_INC_CTYPE_H

#include <cdefs.h>

static gcc_inline int isdigit(int c)	{ return c >= '0' && c <= '9'; }
static gcc_inline int islower(int c)	{ return c >= 'a' && c <= 'z'; }
static gcc_inline int isupper(int c)	{ return c >= 'A' && c <= 'Z'; }
static gcc_inline int isalpha(int c)	{ return islower(c) || isupper(c); }
static gcc_inline int isalnum(int c)	{ return isalpha(c) || isdigit(c); }
static gcc_inline int iscntrl(int c)	{ return c < ' '; }
static gcc_inline int isblank(int c)	{ return c == ' ' || c == '\t'; }
static gcc_inline int isspace(int c)	{ return c == ' '
						|| (c >= '\t' && c <= '\r'); }
static gcc_inline int isprint(int c)	{ return c >= ' ' && c <= '~'; }
static gcc_inline int ispunct(int c)	{ return !isspace(c) && !isalnum(c); }
static gcc_inline int isascii(int c)	{ return c >= 0 && c <= 0x7f; }
static gcc_inline int isxdigit(int c)	{ return isdigit(c)
						|| (c >= 'a' && c <= 'f')
						|| (c >= 'A' && c <= 'F'); }

static gcc_inline int tolower(int c)	{ return isupper(c) ? c + 0x20 : c; }
static gcc_inline int toupper(int c)	{ return islower(c) ? c - 0x20 : c; }

#endif /* not PIOS_INC_CTYPE_H */
