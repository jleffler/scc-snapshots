# @(#)$Id: makefile,v 1.6 2016/06/11 22:26:40 jleffler Exp $
#
# Makefile for SCC (Strip C/C++ Comments)

PROGRAM = scc
SOURCE  = errhelp.c filter.c filterio.c stderr.c scc.c
OBJECT  = errhelp.o filter.o filterio.o stderr.o scc.o
DEBRIS  = a.out core *~
OFLAGS  = -g
WFLAGS  = # -Wall -Wmissing-prototypes -Wstrict-prototypes -std=c11 -pedantic
UFLAGS  = # Set on command line only
IFLAGS  = # -I directory options
DFLAGS  = # -D define options
CFLAGS  = ${OFLAGS} ${UFLAGS} ${WFLAGS} ${IFLAGS} ${DFLAGS}

BASH = bash

.PHONEY: all test dev-test clean realclean depend

TEST_TOOLS = \
	rcskwreduce

TEST_SCRIPTS = \
	scc.test-01.sh \
	scc.test-02.sh \
	scc.test-03.sh \
	scc.test-04.sh \
	scc.test-05.sh \
	scc.test-06.sh \
	scc.test-07.sh \

all: ${PROGRAM} ${TESTTOOLS}

${PROGRAM}: ${OBJECT}
	${CC} -o $@ ${CFLAGS} ${OBJECT} ${LDFLAGS} ${LDLIBES}

test:	${PROGRAM} dev-test ${TESTTOOLS}

dev-test: ${TEST_SCRIPTS}
	for test in ${TEST_SCRIPTS}; \
	do echo $$test; ${BASH} $$test; \
	done

clean:
	rm -f ${OBJECT} ${DEBRIS}

realclean: clean
	rm -f ${PROGRAM} ${SCRIPT}

depend: ${SOURCE}
	mkdep --makefile=scc.mk ${SOURCE}

# DO NOT DELETE THIS LINE or the blank line after it -- make depend uses them.

errhelp.o: errhelp.c
errhelp.o: stderr.h
filter.o: filter.c
filter.o: filter.h
filter.o: stderr.h
scc.o: filter.h
scc.o: posixver.h
scc.o: scc.c
scc.o: stderr.h
stderr.o: stderr.c
stderr.o: stderr.h
