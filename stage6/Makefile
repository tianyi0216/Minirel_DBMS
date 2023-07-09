#
# Makefile for Minirel
#

.SUFFIXES: .o .C

#
# Compiler and loader definitions
#

LD =		ld
LDFLAGS =	

CXX =	         g++

CXXFLAGS =	-g -Wall -DDEBUG #-DDEBUGIND -DDEBUGBUF

MAKEFILE =	Makefile

# Comment out if purify not desired

#PURIFY =	purify -collector=/usr/sup/purify/rld/ld -g++ -inuse-at-exit=yes

PURIFY =        purify -collector=/usr/ccs/bin/ld -g++

#
# list of all object and source files
#

OBJS =		buf.o bufHash.o db.o heapfile.o error.o page.o \
		catalog.o create.o destroy.o \
		help.o load.o print.o quit.o insert.o delete.o \
		select.o join.o sort.o partition.o joinHT.o

DBOBJS =	catalog.o buf.o bufHash.o db.o heapfile.o error.o page.o

NONCATOBJS =	buf.o db.o heapfile.o error.o page.o sort.o 

SRCS =		buf.C  bufHash.C db.C heapfile.C error.C page.C \
		sort.C catalog.C \
		create.C destroy.C help.C load.C print.C \
		quit.C insert.C delete.C select.C join.C minirel.C \
		dbcreate.C dbdestroy.C partition.C joinHT.C

LIBS =		parser.o

all:		minirel dbcreate dbdestroy

minirel:	minirel.o $(OBJS) $(LIBS)
		$(CXX) -o $@ $@.o $(OBJS) $(LIBS) $(LDFLAGS) -lm

parser.o:
		(cd parser; make)

dbcreate:	dbcreate.o $(DBOBJS)
		$(CXX) -o $@ $@.o $(DBOBJS) $(LDFLAGS) -lm

dbdestroy:	dbdestroy.o
		$(CXX) -o $@ $@.o

minirel.pure:	minirel.o $(OBJS) $(LIBS)
		$(PURIFY) $(CXX) -o $@ minirel.o $(OBJS) $(LIBS) $(LDFLAGS) -lm

dbcreate.pure:	dbcreate.o $(DBOBJS) $(LIBS)
		$(PURIFY) $(CXX) -o $@ dbcreate.o $(DBOBJS) $(LDFLAGS) -lm

.C.o:
		$(CXX) $(CXXFLAGS) -c $<

clean:
		(rm -f core *.bak *~ *.o minirel dbcreate dbdestroy *.pure;cd parser;make clean)

depend:
		makedepend -I /s/gcc/include/g++ -f$(MAKEFILE) \
		$(SRCS)


# DO NOT DELETE THIS LINE -- make depend depends on it.
