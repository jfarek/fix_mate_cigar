CC=gcc
CPPFLAGS=-I/users/farek/.local/include
LDFLAGS=
#LIBS=-lhts -lpthread -lz -lm -llzma -lbz2 -lcurl -ldl
LIBHTS_A=/hgsc_software/htslib/htslib-1.9/lib/libhts.a
LIBS=$(LIBHTS_A) -lpthread -lz -lm -l:liblzma.so.0 -l:libbz2.so.1 -lcurl -ldl
CFLAGS=-std=c99 -g -O2 -Wall -pedantic

PROG=fix_mate_cigar
OBJS=fix_mate_cigar.o

.PHONY: all install cleanobjs clean distclean


all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ $(CPPFLAGS)

cleanobjs:
	$(RM) $(OBJS)

clean: cleanobjs
	$(RM) $(PROG)

distclean: clean
