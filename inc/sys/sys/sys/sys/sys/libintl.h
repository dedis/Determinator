#if LAB >= 9
#ifndef PIOS_INC_LIBINTL_H
#define PIOS_INC_LIBINTL_H

char * gettext(const char * msgid);
char * dgettext(const char * domainname, const char * msgid);
char * dcgettext(const char * domainname, const char * msgid,
		 int category);

char * ngettext(const char * msgid, const char * msgid_plural,
		unsigned long int n);
char * dngettext(const char * domainname,
		 const char * msgid, const char * msgid_plural,
		 unsigned long int n);
char * dcngettext(const char * domainname,
		  const char * msgid, const char * msgid_plural,
		  unsigned long int n, int category);

#endif	// PIOS_INC_LIBINTL_H
#endif	// LAB >= 9
