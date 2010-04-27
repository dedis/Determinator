PIOSDIR := ../../../../..
BASEDIR := ../../..
MACROS := $(BASEDIR)/null_macros/c.m4.null.PIOS
M4 := m4 -s -Ulen -Uindex

all:  lu.c

clean:
	rm -rf *.c *.h *.o $(TARGET)

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h:
	$(M4) $(MACROS) $*.H > $*.h

.C.c:
	rm -f $(PIOSDIR)/user/$*_cont.c
	$(M4) $(MACROS) $*.C > $(PIOSDIR)/user/$*_cont.c

