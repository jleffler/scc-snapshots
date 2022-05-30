# @(#)$Id: makefile,v 1.14 2022/05/30 21:13:00 jonathanleffler Exp $
#
# Release Makefile for SCC (Strip C/C++ Comments)
#
# No access to JLSS libraries - use scc.mk for that.

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

BASH    = bash
LN      = ln

TEST_FLAGS = # Nothing by default; -g to generate new results, etc.

TEST_TOOLS = \
	rcskwcmp \
	rcskwreduce \

TEST_SCRIPTS = \
	scc.test-03.sh \
	scc.test-04.sh \
	scc.test-05.sh \
	scc.test-06.sh \
	scc.test-07.sh \
	scc.test-08.sh \
	scc.test-09.sh \
	scc.test-10.sh \

LICENCE = COPYING
GPL_3_0 = gpl-3.0.txt

VERSION_HDR = ${PROGRAM}-version.h

all: ${LICENCE} ${PROGRAM} ${TEST_TOOLS}

# The make on AIX 7.2 interprets this as the default target if it appears before all
.PHONEY: all test dev-test clean realclean depend

${LICENCE}: ${GPL_3_0}
	${LN} $< $@

${PROGRAM}: ${OBJECT} ${VERSION_HDR}
	${CC} -o $@ ${CFLAGS} ${OBJECT} ${LDFLAGS} ${LDLIBES}

test:	${PROGRAM} ${TEST_TOOLS} dev-test

dev-test: ${TEST_SCRIPTS}
	for test in ${TEST_SCRIPTS}; \
	do echo $$test; ${BASH} $$test ${TEST_FLAGS}; \
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
