#///BEGIN 1
TOP = .

SUBDIRS += tools 
SUBDIRS += user
SUBDIRS += kern

include $(TOP)/GNUmakefile.global

handin:
	-gmake clean
	-rm handin.tgz
	gtar czvf handin5.tgz .

#///END
