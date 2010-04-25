#ifndef PIOS_INC_CTYPE_H
#define PIOS_INC_CTYPE_H

#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define islower(c)	((c) >= 'a' && (c) <= 'z')
#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define isalpha(c)	(islower(c) || isupper(c))
#define isalnum(c)	(isalpha(c) || isdigit(c))
#define iscntrl(c)	((c) < ' ')
#define isblank(c)	((c) == ' ' || (c) == '\t')
#define isspace(c)	((c) == ' ' || ((c >= '\t') && (c <= '\r')))
#define isprint(c)	((c) >= ' ' && (c) <= '~')
#define ispunct(c)	(!isspace(c) && !isalnum(c))
#define isascii(c)	(!((c) & 0x80))
#define isxdigit(c)	(isdigit(c) || ((c) >= 'a' && (c) <= 'f') \
				    || ((c) >= 'A' && (c) <= 'F'))

#define tolower(c)	(isupper(c) ? (c) + 0x20 : (c))
#define toupper(c)	(islower(c) ? (c) - 0x20 : (c))

#endif /* not PIOS_INC_CTYPE_H */
