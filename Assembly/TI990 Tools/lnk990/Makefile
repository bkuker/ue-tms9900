
CC = gcc
TARGETS = lnk990
INSTDIR = /usr/local/bin

LNKOBJS = lnk990.o lnkloader.o lnksupt.o lnkpunch.o lnklibrary.o

all: 
	@if [ "`uname -s`" = "Linux" ] ; then \
		echo "Making Linux on a `uname -m`" ;\
		make linux ;\
	elif [ "`uname -s`" = "Darwin" ] ; then \
		echo "Making Darwin" ;\
		make darwin ;\
	elif [ "`uname -s`" = "AIX" ] ; then \
		echo "Making AIX" ;\
		make aix ;\
	elif [ "`uname -s`" = "HP-UX" ] ; then \
		echo "Making HP-UX" ;\
		make hpux ;\
	elif [ "`uname -s`" = "SunOS" ] ; then \
		echo "Making Solaris" ;\
		make solaris ;\
	elif [ "`uname -s`" = "OS/390" ] ; then \
		echo "Making OS/390 USS" ;\
		make uss ;\
	elif [ "`uname -s | cut -c1-7`" = "MINGW32" ] ; then \
		echo "Making `uname -s` on MSYS" ;\
		make mingw ;\
	else \
		echo "OS type `uname -s` is unknown" ;\
		echo "You must enter an OS type. OS types are:" ;\
		echo "   aix | hpux | linux | mingw | nt | solaris | uss " ;\
		echo " " ;\
	fi

aix:
	@make $(TARGETS) "CFLAGS = -DAIX $(DEBUG)"

darwin:
	@make $(TARGETS) "CFLAGS = -DDARWIN $(DEBUG)"

hpux:
	@make $(TARGETS) "CFLAGS = -DHPUX $(DEBUG)"

linux:
	@make $(TARGETS) "CFLAGS = -DLINUX $(DEBUG)"

mingw:
	@make $(TARGETS) "CFLAGS = -DWIN32 -DMINGW $(DEBUG)"

nt:
	@nmake/nologo -f makefile.nt

solaris:
	@make $(TARGETS) "CFLAGS = -DSOLARIS $(DEBUG)"

uss:
	@make $(TARGETS) "CC = cc" "CFLAGS = -DUSS $(DEBUG)"



lnk990: $(LNKOBJS)
	$(CC) -o lnk990 $(LNKOBJS)

clean:
	rm -f $(LNKOBJS) lnk990
	rm -f *.bin *.lst

install:
	cp $(TARGETS) $(INSTDIR)

lnk990.o: lnk990.c lnkdef.h
lnkloader.o: lnkloader.c lnkdef.h
lnklibrary.o: lnklibrary.c lnkdef.h
lnksupt.o: lnksupt.c lnkdef.h
lnkpunch.o: lnkpunch.c lnkdef.h
