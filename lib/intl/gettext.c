#if LAB >= 9
// Trivial "null implementation" of a gettext facility,
// which simply doesn't know how to translate anything.

char * gettext(const char * msgid)
{
	return (char*)msgid;
}

char * dgettext(const char * domainname, const char * msgid)
{
	return (char*)msgid;
}

char * dcgettext(const char * domainname, const char * msgid, int category)
{
	return (char*)msgid;
}

char * ngettext(const char * msgid, const char * msgid_plural,
		unsigned long int n)
{
	return (char*)msgid;
}

char * dngettext(const char * domainname,
		 const char * msgid, const char * msgid_plural,
		 unsigned long int n)
{
	return (char*)(n == 1 ? msgid : msgid_plural);
}

char * dcngettext(const char * domainname,
		  const char * msgid, const char * msgid_plural,
		  unsigned long int n, int category)
{
	return (char*)(n == 1 ? msgid : msgid_plural);
}

#endif	// LAB >= 9
