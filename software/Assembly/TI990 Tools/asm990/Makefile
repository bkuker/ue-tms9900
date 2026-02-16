
CC = gcc
TARGETS = asm990
INSTDIR = /usr/local/bin

ASMOBJS = asm990.o asmpass0.o asmpass1.o asmpass2.o asmoptab.o asmsupt.o \
	parser.o ibmstrtod.o ibmfloat.o

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
		echo "   aix | hpux | linux | mingw | nt | solaris | uss" ;\
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



asm990: $(ASMOBJS)
	$(CC) -o asm990 $(ASMOBJS)

asm990.tok asm990.err asm990.ptb asm990.sem : asm990.bnf
	chat -l asm990.bnf
	@mv asm990.err t.t
	@sed -e 's/fprintf/strcpy/;s/stderr/errorstring/' <t.t >asm990.err
	@rm t.t

clean:
	rm -f $(ASMOBJS) asm990
	rm -f *.obj *.bin *.lst
	rm -f tags cscope.*

tags: *.c *.h
	@cscope -u -b -k -q *.c *.h
	@ctags *.c *.h

install:
	cp $(TARGETS) $(INSTDIR)

asm990.o: asm990.c asmdef.h asm990.tok asm990.err
asmpass0.o: asmpass0.c asmdef.h asm990.tok
asmpass1.o: asmpass1.c asmdef.h asm990.tok
asmpass2.o: asmpass2.c asmdef.h asm990.tok
asmoptab.o: asmoptab.c asmdef.h asm990.tok
asmsupt.o: asmsupt.c asmdef.h asm990.tok
parser.o: parser.c errors.h stables.h scanner.h asmdef.h asm990.ptb asm990.sem
ibmstrtod.o: ibmstrtod.c ibmfloat.h
ibmfloat.o: ibmfloat.c ibmfloat.h
